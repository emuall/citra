list(APPEND CMAKE_MODULE_PATH "${SRC_DIR}/CMakeModules")
include(GenerateBuildInfo)

# The variable SRC_DIR must be passed into the script (since it uses the current build directory for all values of CMAKE_*_DIR)
set(VIDEO_CORE "${SRC_DIR}/src/video_core")
set(HASH_FILES
    "${VIDEO_CORE}/renderer_opengl/gl_shader_decompiler.cpp"
    "${VIDEO_CORE}/renderer_opengl/gl_shader_decompiler.h"
    "${VIDEO_CORE}/renderer_opengl/gl_shader_disk_cache.cpp"
    "${VIDEO_CORE}/renderer_opengl/gl_shader_disk_cache.h"
    "${VIDEO_CORE}/renderer_opengl/gl_shader_gen.cpp"
    "${VIDEO_CORE}/renderer_opengl/gl_shader_gen.h"
    "${VIDEO_CORE}/renderer_opengl/gl_shader_util.cpp"
    "${VIDEO_CORE}/renderer_opengl/gl_shader_util.h"
    "${VIDEO_CORE}/shader/shader.cpp"
    "${VIDEO_CORE}/shader/shader.h"
    "${VIDEO_CORE}/pica.cpp"
    "${VIDEO_CORE}/pica.h"
    "${VIDEO_CORE}/regs_framebuffer.h"
    "${VIDEO_CORE}/regs_lighting.h"
    "${VIDEO_CORE}/regs_pipeline.h"
    "${VIDEO_CORE}/regs_rasterizer.h"
    "${VIDEO_CORE}/regs_shader.h"
    "${VIDEO_CORE}/regs_texturing.h"
    "${VIDEO_CORE}/regs.cpp"
    "${VIDEO_CORE}/regs.h"
)
set(COMBINED "")
foreach (F IN LISTS HASH_FILES)
    file(READ ${F} TMP)
    set(COMBINED "${COMBINED}${TMP}")
endforeach()
string(MD5 SHADER_CACHE_VERSION "${COMBINED}")
configure_file("${SRC_DIR}/src/common/scm_rev.cpp.in" "scm_rev.cpp" @ONLY)
