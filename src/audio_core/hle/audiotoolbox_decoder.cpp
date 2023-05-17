// Copyright 2023 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <AudioToolbox/AudioToolbox.h>
#include "audio_core/audio_types.h"
#include "audio_core/hle/adts.h"
#include "audio_core/hle/audiotoolbox_decoder.h"

namespace AudioCore::HLE {

static constexpr auto bytes_per_sample = sizeof(s16);
static constexpr auto aac_frames_per_packet = 1024;
static constexpr auto error_out_of_data = -1932;

class AudioToolboxDecoder::Impl {
public:
    explicit Impl(Memory::MemorySystem& memory);
    ~Impl();
    std::optional<BinaryResponse> ProcessRequest(const BinaryRequest& request);

private:
    std::optional<BinaryResponse> Initalize(const BinaryRequest& request);
    std::optional<BinaryResponse> Decode(const BinaryRequest& request);

    void Clear();
    bool InitializeDecoder(ADTSData& adts_header);

    static OSStatus DataFunc(AudioConverterRef in_audio_converter, u32* io_number_data_packets,
                             AudioBufferList* io_data,
                             AudioStreamPacketDescription** out_data_packet_description,
                             void* in_user_data);

    Memory::MemorySystem& memory;

    ADTSData adts_config;
    AudioStreamBasicDescription output_format = {};
    AudioConverterRef converter = nullptr;

    u8* curr_data = nullptr;
    u32 curr_data_len = 0;

    AudioStreamPacketDescription packet_description;
};

AudioToolboxDecoder::Impl::Impl(Memory::MemorySystem& memory) : memory(memory) {}

std::optional<BinaryResponse> AudioToolboxDecoder::Impl::Initalize(const BinaryRequest& request) {
    BinaryResponse response;
    std::memcpy(&response, &request, sizeof(response));
    response.unknown1 = 0x0;

    Clear();
    return response;
}

AudioToolboxDecoder::Impl::~Impl() {
    Clear();
}

void AudioToolboxDecoder::Impl::Clear() {
    curr_data = nullptr;
    curr_data_len = 0;

    adts_config = {};
    output_format = {};

    if (converter) {
        AudioConverterDispose(converter);
        converter = nullptr;
    }
}

std::optional<BinaryResponse> AudioToolboxDecoder::Impl::ProcessRequest(
    const BinaryRequest& request) {
    if (request.codec != DecoderCodec::AAC) {
        LOG_ERROR(Audio_DSP, "AudioToolbox AAC Decoder cannot handle such codec: {}",
                  static_cast<u16>(request.codec));
        return {};
    }

    switch (request.cmd) {
    case DecoderCommand::Init: {
        return Initalize(request);
    }
    case DecoderCommand::Decode: {
        return Decode(request);
    }
    case DecoderCommand::Unknown: {
        BinaryResponse response;
        std::memcpy(&response, &request, sizeof(response));
        response.unknown1 = 0x0;
        return response;
    }
    default:
        LOG_ERROR(Audio_DSP, "Got unknown binary request: {}", static_cast<u16>(request.cmd));
        return {};
    }
}

bool AudioToolboxDecoder::Impl::InitializeDecoder(ADTSData& adts_header) {
    if (converter) {
        if (adts_config.channels == adts_header.channels &&
            adts_config.samplerate == adts_header.samplerate) {
            return true;
        } else {
            Clear();
        }
    }

    AudioStreamBasicDescription input_format = {
        .mSampleRate = static_cast<Float64>(adts_header.samplerate),
        .mFormatID = kAudioFormatMPEG4AAC,
        .mFramesPerPacket = aac_frames_per_packet,
        .mChannelsPerFrame = adts_header.channels,
    };

    u32 bytes_per_frame = input_format.mChannelsPerFrame * bytes_per_sample;
    output_format = {
        .mSampleRate = input_format.mSampleRate,
        .mFormatID = kAudioFormatLinearPCM,
        .mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked,
        .mBytesPerPacket = bytes_per_frame,
        .mFramesPerPacket = 1,
        .mBytesPerFrame = bytes_per_frame,
        .mChannelsPerFrame = input_format.mChannelsPerFrame,
        .mBitsPerChannel = bytes_per_sample * 8,
    };

    auto status = AudioConverterNew(&input_format, &output_format, &converter);
    if (status != noErr) {
        LOG_ERROR(Audio_DSP, "Could not create AAC audio converter: {}", status);
        Clear();
        return false;
    }

    adts_config = adts_header;
    return true;
}

OSStatus AudioToolboxDecoder::Impl::DataFunc(
    AudioConverterRef in_audio_converter, u32* io_number_data_packets, AudioBufferList* io_data,
    AudioStreamPacketDescription** out_data_packet_description, void* in_user_data) {
    auto impl = reinterpret_cast<Impl*>(in_user_data);
    if (!impl || !impl->curr_data || impl->curr_data_len == 0) {
        *io_number_data_packets = 0;
        return error_out_of_data;
    }

    io_data->mNumberBuffers = 1;
    io_data->mBuffers[0].mNumberChannels = 0;
    io_data->mBuffers[0].mDataByteSize = impl->curr_data_len;
    io_data->mBuffers[0].mData = impl->curr_data;
    *io_number_data_packets = 1;

    if (out_data_packet_description != nullptr) {
        impl->packet_description.mStartOffset = 0;
        impl->packet_description.mVariableFramesInPacket = 0;
        impl->packet_description.mDataByteSize = impl->curr_data_len;
        *out_data_packet_description = &impl->packet_description;
    }

    impl->curr_data = nullptr;
    impl->curr_data_len = 0;

    return noErr;
}

std::optional<BinaryResponse> AudioToolboxDecoder::Impl::Decode(const BinaryRequest& request) {
    BinaryResponse response;
    response.codec = request.codec;
    response.cmd = request.cmd;
    response.size = request.size;

    if (request.src_addr < Memory::FCRAM_PADDR ||
        request.src_addr + request.size > Memory::FCRAM_PADDR + Memory::FCRAM_SIZE) {
        LOG_ERROR(Audio_DSP, "Got out of bounds src_addr {:08x}", request.src_addr);
        return {};
    }

    auto data = memory.GetFCRAMPointer(request.src_addr - Memory::FCRAM_PADDR);
    auto adts_header = ParseADTS(reinterpret_cast<const char*>(data));
    curr_data = data + adts_header.header_length;
    curr_data_len = request.size - adts_header.header_length;

    if (!InitializeDecoder(adts_header)) {
        return std::nullopt;
    }

    // Up to 2048 samples, up to 2 channels each
    s16 decoder_output[4096];
    AudioBufferList out_buffer{1,
                               {{
                                   output_format.mChannelsPerFrame,
                                   sizeof(decoder_output),
                                   decoder_output,
                               }}};

    u32 num_packets = sizeof(decoder_output) / output_format.mBytesPerPacket;
    auto status = AudioConverterFillComplexBuffer(converter, DataFunc, this, &num_packets,
                                                  &out_buffer, nullptr);
    if (status != noErr && status != error_out_of_data) {
        LOG_ERROR(Audio_DSP, "Could not decode AAC data: {}", status);
        Clear();
        return std::nullopt;
    }

    // De-interleave samples.
    std::array<std::vector<s16>, 2> out_streams;
    auto num_frames = num_packets * output_format.mFramesPerPacket;
    for (u32 frame = 0; frame < num_frames; frame++) {
        for (u32 ch = 0; ch < output_format.mChannelsPerFrame; ch++) {
            out_streams[ch].push_back(
                decoder_output[(frame * output_format.mChannelsPerFrame) + ch]);
        }
    }

    curr_data = nullptr;
    curr_data_len = 0;

    response.sample_rate = GetSampleRateEnum(static_cast<u32>(output_format.mSampleRate));
    response.num_channels = output_format.mChannelsPerFrame;
    response.num_samples = num_frames;

    // transfer the decoded buffer from vector to the FCRAM
    for (std::size_t ch = 0; ch < out_streams.size(); ch++) {
        if (!out_streams[ch].empty()) {
            auto byte_size = out_streams[ch].size() * bytes_per_sample;
            auto dst = ch == 0 ? request.dst_addr_ch0 : request.dst_addr_ch1;
            if (dst < Memory::FCRAM_PADDR ||
                dst + byte_size > Memory::FCRAM_PADDR + Memory::FCRAM_SIZE) {
                LOG_ERROR(Audio_DSP, "Got out of bounds dst_addr_ch{} {:08x}", ch, dst);
                return {};
            }
            std::memcpy(memory.GetFCRAMPointer(dst - Memory::FCRAM_PADDR), out_streams[ch].data(),
                        byte_size);
        }
    }

    return response;
}

AudioToolboxDecoder::AudioToolboxDecoder(Memory::MemorySystem& memory)
    : impl(std::make_unique<Impl>(memory)) {}

AudioToolboxDecoder::~AudioToolboxDecoder() = default;

std::optional<BinaryResponse> AudioToolboxDecoder::ProcessRequest(const BinaryRequest& request) {
    return impl->ProcessRequest(request);
}

bool AudioToolboxDecoder::IsValid() const {
    return true;
}

} // namespace AudioCore::HLE
