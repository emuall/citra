// Copyright 2022 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hw/gpu.h"
#include "video_core/rasterizer_accelerated.h"
#include "video_core/rasterizer_cache/rasterizer_cache.h"
#include "video_core/rasterizer_interface.h"
#include "video_core/regs_texturing.h"
#include "video_core/renderer_opengl/gl_shader_manager.h"
#include "video_core/renderer_opengl/gl_state.h"
#include "video_core/renderer_opengl/gl_stream_buffer.h"
#include "video_core/renderer_opengl/gl_texture_runtime.h"

namespace VideoCore {
class RendererBase;
}

namespace VideoCore {
class CustomTexManager;
}

namespace OpenGL {

class Driver;
class ShaderProgramManager;

class RasterizerOpenGL : public VideoCore::RasterizerAccelerated {
public:
    explicit RasterizerOpenGL(Memory::MemorySystem& memory,
                              VideoCore::CustomTexManager& custom_tex_manager,
                              VideoCore::RendererBase& renderer, Driver& driver);
    ~RasterizerOpenGL() override;

    void TickFrame();
    void LoadDiskResources(const std::atomic_bool& stop_loading,
                           const VideoCore::DiskResourceLoadCallback& callback) override;

    void DrawTriangles() override;
    void FlushAll() override;
    void FlushRegion(PAddr addr, u32 size) override;
    void InvalidateRegion(PAddr addr, u32 size) override;
    void FlushAndInvalidateRegion(PAddr addr, u32 size) override;
    void ClearAll(bool flush) override;
    bool AccelerateDisplayTransfer(const GPU::Regs::DisplayTransferConfig& config) override;
    bool AccelerateTextureCopy(const GPU::Regs::DisplayTransferConfig& config) override;
    bool AccelerateFill(const GPU::Regs::MemoryFillConfig& config) override;
    bool AccelerateDisplay(const GPU::Regs::FramebufferConfig& config, PAddr framebuffer_addr,
                           u32 pixel_stride, ScreenInfo& screen_info) override;
    bool AccelerateDrawBatch(bool is_indexed) override;

private:
    void SyncFixedState() override;
    void NotifyFixedFunctionPicaRegisterChanged(u32 id) override;

    /// Syncs the clip enabled status to match the PICA register
    void SyncClipEnabled();

    /// Syncs the cull mode to match the PICA register
    void SyncCullMode();

    /// Syncs the blend enabled status to match the PICA register
    void SyncBlendEnabled();

    /// Syncs the blend functions to match the PICA register
    void SyncBlendFuncs();

    /// Syncs the blend color to match the PICA register
    void SyncBlendColor();

    /// Syncs the logic op states to match the PICA register
    void SyncLogicOp();

    /// Syncs the color write mask to match the PICA register state
    void SyncColorWriteMask();

    /// Syncs the stencil write mask to match the PICA register state
    void SyncStencilWriteMask();

    /// Syncs the depth write mask to match the PICA register state
    void SyncDepthWriteMask();

    /// Syncs the stencil test states to match the PICA register
    void SyncStencilTest();

    /// Syncs the depth test states to match the PICA register
    void SyncDepthTest();

    /// Syncs and uploads the lighting, fog and proctex LUTs
    void SyncAndUploadLUTs();
    void SyncAndUploadLUTsLF();

    /// Syncs all enabled PICA texture units
    void SyncTextureUnits(const Framebuffer& framebuffer);

    /// Binds the PICA shadow cube required for shadow mapping
    void BindShadowCube(const Pica::TexturingRegs::FullTextureConfig& texture);

    /// Binds a texture cube to texture unit 0
    void BindTextureCube(const Pica::TexturingRegs::FullTextureConfig& texture);

    /// Makes a temporary copy of the framebuffer if a feedback loop is detected
    bool IsFeedbackLoop(u32 texture_index, const Framebuffer& framebuffer, Surface& surface);

    /// Unbinds all special texture unit 0 texture configurations
    void UnbindSpecial();

    /// Binds the custom material referenced by surface if it exists.
    void BindMaterial(u32 texture_index, Surface& surface);

    /// Upload the uniform blocks to the uniform buffer object
    void UploadUniforms(bool accelerate_draw);

    /// Generic draw function for DrawTriangles and AccelerateDrawBatch
    bool Draw(bool accelerate, bool is_indexed);

    /// Internal implementation for AccelerateDrawBatch
    bool AccelerateDrawBatchInternal(bool is_indexed);

    /// Setup vertex array for AccelerateDrawBatch
    void SetupVertexArray(u8* array_ptr, GLintptr buffer_offset, GLuint vs_input_index_min,
                          GLuint vs_input_index_max);

    /// Setup vertex shader for AccelerateDrawBatch
    bool SetupVertexShader();

    /// Setup geometry shader for AccelerateDrawBatch
    bool SetupGeometryShader();

private:
    Driver& driver;
    OpenGLState state;
    GLuint default_texture;
    TextureRuntime runtime;
    RasterizerCache res_cache;
    std::unique_ptr<ShaderProgramManager> shader_program_manager;

    OGLVertexArray sw_vao; // VAO for software shader draw
    OGLVertexArray hw_vao; // VAO for hardware shader / accelerate draw
    std::array<bool, 16> hw_vao_enabled_attributes{};

    GLsizeiptr texture_buffer_size;
    OGLStreamBuffer vertex_buffer;
    OGLStreamBuffer uniform_buffer;
    OGLStreamBuffer index_buffer;
    OGLStreamBuffer texture_buffer;
    OGLStreamBuffer texture_lf_buffer;
    OGLFramebuffer framebuffer;
    GLint uniform_buffer_alignment;
    std::size_t uniform_size_aligned_vs;
    std::size_t uniform_size_aligned_fs;

    OGLTexture texture_buffer_lut_lf;
    OGLTexture texture_buffer_lut_rg;
    OGLTexture texture_buffer_lut_rgba;
    bool use_custom_normal{};
};

} // namespace OpenGL
