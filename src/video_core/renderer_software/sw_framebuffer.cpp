// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include "common/color.h"
#include "common/logging/log.h"
#include "core/hw/gpu.h"
#include "core/memory.h"
#include "video_core/pica_types.h"
#include "video_core/regs_framebuffer.h"
#include "video_core/renderer_software/sw_framebuffer.h"
#include "video_core/utils.h"

namespace SwRenderer {

using Pica::f16;
using Pica::FramebufferRegs;

namespace {

/// Decode/Encode for shadow map format. It is similar to D24S8 format,
/// but the depth field is in big-endian.
const Common::Vec2<u32> DecodeD24S8Shadow(const u8* bytes) {
    return {static_cast<u32>((bytes[0] << 16) | (bytes[1] << 8) | bytes[2]), bytes[3]};
}

void EncodeD24X8Shadow(u32 depth, u8* bytes) {
    bytes[2] = depth & 0xFF;
    bytes[1] = (depth >> 8) & 0xFF;
    bytes[0] = (depth >> 16) & 0xFF;
}

void EncodeX24S8Shadow(u8 stencil, u8* bytes) {
    bytes[3] = stencil;
}
} // Anonymous namespace

Framebuffer::Framebuffer(Memory::MemorySystem& memory_, const Pica::FramebufferRegs& regs_)
    : memory{memory_}, regs{regs_} {}

Framebuffer::~Framebuffer() = default;

void Framebuffer::DrawPixel(int x, int y, const Common::Vec4<u8>& color) const {
    const auto& framebuffer = regs.framebuffer;
    const PAddr addr = framebuffer.GetColorBufferPhysicalAddress();

    // Similarly to textures, the render framebuffer is laid out from bottom to top, too.
    // NOTE: The framebuffer height register contains the actual FB height minus one.
    y = framebuffer.height - y;

    const u32 coarse_y = y & ~7;
    const u32 bytes_per_pixel =
        GPU::Regs::BytesPerPixel(GPU::Regs::PixelFormat(framebuffer.color_format.Value()));
    const u32 dst_offset = VideoCore::GetMortonOffset(x, y, bytes_per_pixel) +
                           coarse_y * framebuffer.width * bytes_per_pixel;
    u8* depth_buffer = memory.GetPhysicalPointer(addr);
    u8* dst_pixel = depth_buffer + dst_offset;

    switch (framebuffer.color_format) {
    case FramebufferRegs::ColorFormat::RGBA8:
        Common::Color::EncodeRGBA8(color, dst_pixel);
        break;
    case FramebufferRegs::ColorFormat::RGB8:
        Common::Color::EncodeRGB8(color, dst_pixel);
        break;
    case FramebufferRegs::ColorFormat::RGB5A1:
        Common::Color::EncodeRGB5A1(color, dst_pixel);
        break;
    case FramebufferRegs::ColorFormat::RGB565:
        Common::Color::EncodeRGB565(color, dst_pixel);
        break;
    case FramebufferRegs::ColorFormat::RGBA4:
        Common::Color::EncodeRGBA4(color, dst_pixel);
        break;
    default:
        LOG_CRITICAL(Render_Software, "Unknown framebuffer color format {:x}",
                     static_cast<u32>(framebuffer.color_format.Value()));
        UNIMPLEMENTED();
    }
}

const Common::Vec4<u8> Framebuffer::GetPixel(int x, int y) const {
    const auto& framebuffer = regs.framebuffer;
    const PAddr addr = framebuffer.GetColorBufferPhysicalAddress();

    y = framebuffer.height - y;

    const u32 coarse_y = y & ~7;
    const u32 bytes_per_pixel =
        GPU::Regs::BytesPerPixel(GPU::Regs::PixelFormat(framebuffer.color_format.Value()));
    const u32 src_offset = VideoCore::GetMortonOffset(x, y, bytes_per_pixel) +
                           coarse_y * framebuffer.width * bytes_per_pixel;
    const u8* color_buffer = memory.GetPhysicalPointer(addr);
    const u8* src_pixel = color_buffer + src_offset;

    switch (framebuffer.color_format) {
    case FramebufferRegs::ColorFormat::RGBA8:
        return Common::Color::DecodeRGBA8(src_pixel);
    case FramebufferRegs::ColorFormat::RGB8:
        return Common::Color::DecodeRGB8(src_pixel);
    case FramebufferRegs::ColorFormat::RGB5A1:
        return Common::Color::DecodeRGB5A1(src_pixel);
    case FramebufferRegs::ColorFormat::RGB565:
        return Common::Color::DecodeRGB565(src_pixel);
    case FramebufferRegs::ColorFormat::RGBA4:
        return Common::Color::DecodeRGBA4(src_pixel);
    default:
        LOG_CRITICAL(Render_Software, "Unknown framebuffer color format {:x}",
                     static_cast<u32>(framebuffer.color_format.Value()));
        UNIMPLEMENTED();
    }

    return {0, 0, 0, 0};
}

u32 Framebuffer::GetDepth(int x, int y) const {
    const auto& framebuffer = regs.framebuffer;
    const PAddr addr = framebuffer.GetDepthBufferPhysicalAddress();

    y = framebuffer.height - y;

    const u32 coarse_y = y & ~7;
    const u32 bytes_per_pixel = FramebufferRegs::BytesPerDepthPixel(framebuffer.depth_format);
    const u32 stride = framebuffer.width * bytes_per_pixel;

    const u32 src_offset = VideoCore::GetMortonOffset(x, y, bytes_per_pixel) + coarse_y * stride;
    const u8* depth_buffer = memory.GetPhysicalPointer(addr);
    const u8* src_pixel = depth_buffer + src_offset;

    switch (framebuffer.depth_format) {
    case FramebufferRegs::DepthFormat::D16:
        return Common::Color::DecodeD16(src_pixel);
    case FramebufferRegs::DepthFormat::D24:
        return Common::Color::DecodeD24(src_pixel);
    case FramebufferRegs::DepthFormat::D24S8:
        return Common::Color::DecodeD24S8(src_pixel).x;
    default:
        LOG_CRITICAL(HW_GPU, "Unimplemented depth format {}",
                     static_cast<u32>(framebuffer.depth_format.Value()));
        UNIMPLEMENTED();
        return 0;
    }
}

u8 Framebuffer::GetStencil(int x, int y) const {
    const auto& framebuffer = regs.framebuffer;
    const PAddr addr = framebuffer.GetDepthBufferPhysicalAddress();

    y = framebuffer.height - y;

    const u32 coarse_y = y & ~7;
    const u32 bytes_per_pixel = Pica::FramebufferRegs::BytesPerDepthPixel(framebuffer.depth_format);
    const u32 stride = framebuffer.width * bytes_per_pixel;

    const u32 src_offset = VideoCore::GetMortonOffset(x, y, bytes_per_pixel) + coarse_y * stride;
    const u8* depth_buffer = memory.GetPhysicalPointer(addr);
    const u8* src_pixel = depth_buffer + src_offset;

    switch (framebuffer.depth_format) {
    case FramebufferRegs::DepthFormat::D24S8:
        return Common::Color::DecodeD24S8(src_pixel).y;
    default:
        LOG_WARNING(
            HW_GPU,
            "GetStencil called for function which doesn't have a stencil component (format {})",
            static_cast<u32>(framebuffer.depth_format.Value()));
        return 0;
    }
}

void Framebuffer::SetDepth(int x, int y, u32 value) const {
    const auto& framebuffer = regs.framebuffer;
    const PAddr addr = framebuffer.GetDepthBufferPhysicalAddress();

    y = framebuffer.height - y;

    const u32 coarse_y = y & ~7;
    const u32 bytes_per_pixel = FramebufferRegs::BytesPerDepthPixel(framebuffer.depth_format);
    const u32 stride = framebuffer.width * bytes_per_pixel;

    const u32 dst_offset = VideoCore::GetMortonOffset(x, y, bytes_per_pixel) + coarse_y * stride;
    u8* depth_buffer = memory.GetPhysicalPointer(addr);
    u8* dst_pixel = depth_buffer + dst_offset;

    switch (framebuffer.depth_format) {
    case FramebufferRegs::DepthFormat::D16:
        Common::Color::EncodeD16(value, dst_pixel);
        break;
    case FramebufferRegs::DepthFormat::D24:
        Common::Color::EncodeD24(value, dst_pixel);
        break;
    case FramebufferRegs::DepthFormat::D24S8:
        Common::Color::EncodeD24X8(value, dst_pixel);
        break;
    default:
        LOG_CRITICAL(HW_GPU, "Unimplemented depth format {}",
                     static_cast<u32>(framebuffer.depth_format.Value()));
        UNIMPLEMENTED();
        break;
    }
}

void Framebuffer::SetStencil(int x, int y, u8 value) const {
    const auto& framebuffer = regs.framebuffer;
    const PAddr addr = framebuffer.GetDepthBufferPhysicalAddress();

    y = framebuffer.height - y;

    const u32 coarse_y = y & ~7;
    const u32 bytes_per_pixel = Pica::FramebufferRegs::BytesPerDepthPixel(framebuffer.depth_format);
    const u32 stride = framebuffer.width * bytes_per_pixel;

    const u32 dst_offset = VideoCore::GetMortonOffset(x, y, bytes_per_pixel) + coarse_y * stride;
    u8* depth_buffer = memory.GetPhysicalPointer(addr);
    u8* dst_pixel = depth_buffer + dst_offset;

    switch (framebuffer.depth_format) {
    case Pica::FramebufferRegs::DepthFormat::D16:
    case Pica::FramebufferRegs::DepthFormat::D24:
        // Nothing to do
        break;
    case Pica::FramebufferRegs::DepthFormat::D24S8:
        Common::Color::EncodeX24S8(value, dst_pixel);
        break;
    default:
        LOG_CRITICAL(HW_GPU, "Unimplemented depth format {}",
                     static_cast<u32>(framebuffer.depth_format.Value()));
        UNIMPLEMENTED();
        break;
    }
}

void Framebuffer::DrawShadowMapPixel(int x, int y, u32 depth, u8 stencil) const {
    const auto& framebuffer = regs.framebuffer;
    const auto& shadow = regs.shadow;
    const PAddr addr = framebuffer.GetColorBufferPhysicalAddress();

    y = framebuffer.height - y;

    const u32 coarse_y = y & ~7;
    u32 bytes_per_pixel = 4;
    u32 dst_offset = VideoCore::GetMortonOffset(x, y, bytes_per_pixel) +
                     coarse_y * framebuffer.width * bytes_per_pixel;
    u8* shadow_buffer = memory.GetPhysicalPointer(addr);
    u8* dst_pixel = shadow_buffer + dst_offset;

    const auto ref = DecodeD24S8Shadow(dst_pixel);
    const u32 ref_z = ref.x;
    const u32 ref_s = ref.y;

    if (depth >= ref_z) {
        return;
    }

    if (stencil == 0) {
        EncodeD24X8Shadow(depth, dst_pixel);
    } else {
        const f16 constant = f16::FromRaw(shadow.constant);
        const f16 linear = f16::FromRaw(shadow.linear);
        const f16 x_ = f16::FromFloat32(static_cast<float>(depth) / ref_z);
        const f16 stencil_new = f16::FromFloat32(stencil) / (constant + linear * x_);
        stencil = static_cast<u8>(std::clamp(stencil_new.ToFloat32(), 0.0f, 255.0f));

        if (stencil < ref_s) {
            EncodeX24S8Shadow(stencil, dst_pixel);
        }
    }
}

u8 PerformStencilAction(FramebufferRegs::StencilAction action, u8 old_stencil, u8 ref) {
    switch (action) {
    case FramebufferRegs::StencilAction::Keep:
        return old_stencil;
    case FramebufferRegs::StencilAction::Zero:
        return 0;
    case FramebufferRegs::StencilAction::Replace:
        return ref;
    case FramebufferRegs::StencilAction::Increment:
        // Saturated increment
        return std::min<u8>(old_stencil, 254) + 1;
    case FramebufferRegs::StencilAction::Decrement:
        // Saturated decrement
        return std::max<u8>(old_stencil, 1) - 1;
    case FramebufferRegs::StencilAction::Invert:
        return ~old_stencil;
    case FramebufferRegs::StencilAction::IncrementWrap:
        return old_stencil + 1;
    case FramebufferRegs::StencilAction::DecrementWrap:
        return old_stencil - 1;
    default:
        LOG_CRITICAL(HW_GPU, "Unknown stencil action {:x}", static_cast<int>(action));
        UNIMPLEMENTED();
        return 0;
    }
}

Common::Vec4<u8> EvaluateBlendEquation(const Common::Vec4<u8>& src,
                                       const Common::Vec4<u8>& srcfactor,
                                       const Common::Vec4<u8>& dest,
                                       const Common::Vec4<u8>& destfactor,
                                       FramebufferRegs::BlendEquation equation) {
    Common::Vec4i result;

    const auto src_result = (src * srcfactor).Cast<s32>();
    const auto dst_result = (dest * destfactor).Cast<s32>();

    switch (equation) {
    case FramebufferRegs::BlendEquation::Add:
        result = (src_result + dst_result) / 255;
        break;
    case FramebufferRegs::BlendEquation::Subtract:
        result = (src_result - dst_result) / 255;
        break;
    case FramebufferRegs::BlendEquation::ReverseSubtract:
        result = (dst_result - src_result) / 255;
        break;
    // TODO: How do these two actually work?  OpenGL doesn't include the blend factors in the
    //       min/max computations, but is this what the 3DS actually does?
    case FramebufferRegs::BlendEquation::Min:
        result.r() = std::min(src.r(), dest.r());
        result.g() = std::min(src.g(), dest.g());
        result.b() = std::min(src.b(), dest.b());
        result.a() = std::min(src.a(), dest.a());
        break;
    case FramebufferRegs::BlendEquation::Max:
        result.r() = std::max(src.r(), dest.r());
        result.g() = std::max(src.g(), dest.g());
        result.b() = std::max(src.b(), dest.b());
        result.a() = std::max(src.a(), dest.a());
        break;
    default:
        LOG_CRITICAL(HW_GPU, "Unknown RGB blend equation 0x{:x}", equation);
        UNIMPLEMENTED();
    }

    return Common::Vec4<u8>(std::clamp(result.r(), 0, 255), std::clamp(result.g(), 0, 255),
                            std::clamp(result.b(), 0, 255), std::clamp(result.a(), 0, 255));
};

u8 LogicOp(u8 src, u8 dest, FramebufferRegs::LogicOp op) {
    switch (op) {
    case FramebufferRegs::LogicOp::Clear:
        return 0;
    case FramebufferRegs::LogicOp::And:
        return src & dest;
    case FramebufferRegs::LogicOp::AndReverse:
        return src & ~dest;
    case FramebufferRegs::LogicOp::Copy:
        return src;
    case FramebufferRegs::LogicOp::Set:
        return 255;
    case FramebufferRegs::LogicOp::CopyInverted:
        return ~src;
    case FramebufferRegs::LogicOp::NoOp:
        return dest;
    case FramebufferRegs::LogicOp::Invert:
        return ~dest;
    case FramebufferRegs::LogicOp::Nand:
        return ~(src & dest);
    case FramebufferRegs::LogicOp::Or:
        return src | dest;
    case FramebufferRegs::LogicOp::Nor:
        return ~(src | dest);
    case FramebufferRegs::LogicOp::Xor:
        return src ^ dest;
    case FramebufferRegs::LogicOp::Equiv:
        return ~(src ^ dest);
    case FramebufferRegs::LogicOp::AndInverted:
        return ~src & dest;
    case FramebufferRegs::LogicOp::OrReverse:
        return src | ~dest;
    case FramebufferRegs::LogicOp::OrInverted:
        return ~src | dest;
    }
    UNREACHABLE();
};

} // namespace SwRenderer
