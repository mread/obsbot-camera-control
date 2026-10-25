cmake_minimum_required(VERSION 3.16)

project(obsbot-control)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets OpenGLWidgets DBus)

# SDK paths
set(SDK_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/sdk/include)
set(SDK_LIB_DIR ${CMAKE_SOURCE_DIR}/sdk/lib)

# GUI Application
add_executable(obsbot-gui
    src/gui/main.cpp
    src/gui/MainWindow.cpp
    src/gui/MainWindow.h
    src/gui/CameraController.cpp
    src/gui/CameraControllerTiny2.cpp
    src/gui/CameraHolders.cpp
    src/gui/CameraHolders.h
    src/gui/ControlService.cpp
    src/gui/FlowLayout.cpp
    src/gui/FlowLayout.h
    src/gui/ControlService.h
    src/gui/ModeBar.cpp
    src/gui/ModeBar.h
    src/gui/AimPad.cpp
    src/gui/AimPad.h
    src/gui/PreviewAimFilter.cpp
    src/gui/PreviewAimFilter.h
    src/gui/Tiny2Panel.h
    src/gui/Tiny2CameraTab.cpp
    src/gui/Tiny2CameraTab.h
    src/gui/Tiny2ImageTab.cpp
    src/gui/Tiny2ImageTab.h
    src/gui/Tiny2DeviceTab.cpp
    src/gui/Tiny2DeviceTab.h
    src/gui/CameraController.h
    src/gui/TrackingControlWidget.cpp
    src/gui/TrackingControlWidget.h
    src/gui/PTZControlWidget.cpp
    src/gui/PTZControlWidget.h
    src/gui/CameraSettingsWidget.cpp
    src/gui/CameraSettingsWidget.h
    src/gui/FilterPreviewWidget.cpp
    src/gui/FilterPreviewWidget.h
    src/gui/CameraPreviewWidget.cpp
    src/gui/CameraPreviewWidget.h
    src/gui/VideoEffectsWidget.cpp
    src/gui/VideoEffectsWidget.h
    src/gui/VirtualCameraStreamer.cpp
    src/gui/VirtualCameraStreamer.h
    src/gui/PreviewWindow.cpp
    src/gui/PreviewWindow.h
    src/gui/V4l2Backend.cpp
    src/gui/V4l2Backend.h
    src/gui/XYPad.cpp
    src/gui/XYPad.h
    src/common/Config.cpp
    src/common/Config.h
    resources/resources.qrc
)

target_include_directories(obsbot-gui PRIVATE
    ${SDK_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_directories(obsbot-gui PRIVATE
    ${SDK_LIB_DIR}
)

target_link_libraries(obsbot-gui PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::OpenGLWidgets
    Qt6::DBus
    dev
)

target_compile_definitions(obsbot-gui PRIVATE
    $<$<CONFIG:Debug>:OBSBOT_DEBUG_GEOMETRY>
)

# CLI Application
add_executable(obsbot-cli
    src/cli/meet2_test.cpp
    src/common/Config.cpp
    src/common/Config.h
)

target_include_directories(obsbot-cli PRIVATE
    ${SDK_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_directories(obsbot-cli PRIVATE
    ${SDK_LIB_DIR}
)

target_link_libraries(obsbot-cli PRIVATE
    dev
)

# Set RPATH for finding libdev.so
# Build uses local SDK, install uses system library path
set_target_properties(obsbot-gui PROPERTIES
    BUILD_RPATH "${SDK_LIB_DIR}"
    INSTALL_RPATH "/usr/lib"
)

set_target_properties(obsbot-cli PROPERTIES
    BUILD_RPATH "${SDK_LIB_DIR}"
    INSTALL_RPATH "/usr/lib"
)
