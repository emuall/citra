// Copyright 2022 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string_view>
#include "common/common_types.h"

namespace Core {
class TelemetrySession;
}

namespace VideoCore {
enum class CustomPixelFormat : u32;
}

namespace OpenGL {

enum class Vendor {
    Unknown = 0,
    AMD = 1,
    Nvidia = 2,
    Intel = 3,
    ARM = 4,
    Qualcomm = 5,
    Samsung = 6,
    Generic = 7,
};

enum class DriverBug {
    // AMD drivers sometimes freezes when one shader stage is changed but not the others.
    ShaderStageChangeFreeze = 1 << 0,
    // On AMD drivers there is a strange crash in indexed drawing. The crash happens when the buffer
    // read position is near the end and is an out-of-bound access to the vertex buffer. This is
    // probably a bug in the driver and is related to the usage of vec3<byte> attributes in the
    // vertex array. Doubling the allocation size for the vertex buffer seems to avoid the crash.
    VertexArrayOutOfBound = 1 << 1,
    // On AMD and Intel drivers on Windows glTextureView produces incorrect results
    BrokenTextureView = 1 << 2,
};

/**
 * Utility class that loads the OpenGL function pointers and reports
 * information about the graphics device and driver used
 */
class Driver {
public:
    Driver(Core::TelemetrySession& telemetry_session);
    ~Driver();

    /// Returns true of the driver has a particular bug stated in the DriverBug enum
    bool HasBug(DriverBug bug) const;

    /// Returns true if any debug tool is attached
    bool HasDebugTool();

    /// Returns true if the driver supports the provided custom format
    bool IsCustomFormatSupported(VideoCore::CustomPixelFormat format) const;

    /// Returns the vendor of the currently selected physical device
    Vendor GetVendor() const {
        return vendor;
    }

    /// Returns the gpu vendor string returned by the driver
    std::string_view GetVendorString() const {
        return gpu_vendor;
    }

    /// Returns true if the an OpenGLES context is used
    bool IsOpenGLES() const noexcept {
        return is_gles;
    }

    /// Returns true if the implementation is suitable for emulation
    bool IsSuitable() const {
        return is_suitable;
    }

    /// Returns true if the implementation supports ARB_buffer_storage
    bool HasArbBufferStorage() const {
        return arb_buffer_storage;
    }

    /// Returns true if the implementation supports EXT_buffer_storage
    bool HasExtBufferStorage() const {
        return ext_buffer_storage;
    }

    /// Returns true if the implementation supports ARB_clear_texture
    bool HasArbClearTexture() const {
        return arb_clear_texture;
    }

    /// Returns true if the implementation supports ARB_get_texture_sub_image
    bool HasArbGetTextureSubImage() const {
        return arb_get_texture_sub_image;
    }

    /// Returns true if the implementation supports EXT_clip_cull_distance
    bool HasExtClipCullDistance() const {
        return ext_clip_cull_distance;
    }

private:
    void ReportDriverInfo();
    void DeduceVendor();
    void CheckExtensionSupport();
    void FindBugs();

private:
    Core::TelemetrySession& telemetry_session;
    Vendor vendor = Vendor::Unknown;
    DriverBug bugs{};
    bool is_suitable{};
    bool is_gles{};

    bool ext_buffer_storage{};
    bool arb_buffer_storage{};
    bool arb_clear_texture{};
    bool arb_get_texture_sub_image{};
    bool ext_clip_cull_distance{};
    bool ext_texture_compression_s3tc{};
    bool arb_texture_compression_bptc{};

    std::string_view gl_version{};
    std::string_view gpu_vendor{};
    std::string_view gpu_model{};
};

} // namespace OpenGL
