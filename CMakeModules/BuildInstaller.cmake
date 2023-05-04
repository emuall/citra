# To use this as a script, make sure you pass in the variables BASE_DIR, SRC_DIR, BUILD_DIR, and TARGET_FILE

if(WIN32)
    set(PLATFORM "windows")
elseif(APPLE)
    set(PLATFORM "mac")
elseif(UNIX)
    set(PLATFORM "linux")
else()
    message(FATAL_ERROR "Cannot build installer for this unsupported platform")
endif()

list(APPEND CMAKE_MODULE_PATH "${BASE_DIR}/CMakeModules")
include(DownloadExternals)
download_qt_external(tools_ifw QT_PREFIX)

file(GLOB_RECURSE INSTALLER_BASE "${QT_PREFIX}/**/installerbase*")
file(GLOB_RECURSE BINARY_CREATOR "${QT_PREFIX}/**/binarycreator*")

set(CONFIG_FILE "${SRC_DIR}/config/config_${PLATFORM}.xml")
set(PACKAGES_DIR "${BUILD_DIR}/packages")
file(MAKE_DIRECTORY ${PACKAGES_DIR})

execute_process(COMMAND ${BINARY_CREATOR} -t ${INSTALLER_BASE} -n -c ${CONFIG_FILE} -p ${PACKAGES_DIR} ${TARGET_FILE})
