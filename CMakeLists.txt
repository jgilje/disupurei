cmake_minimum_required(VERSION 2.8.11)

project(disupurei)

include(FindPkgConfig)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

find_package(Qt5Network)
find_package(Qt5Widgets)
find_package(Qt5X11Extras)

pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-video-1.0 gstreamer-gl-1.0)

#include_directories(GSTREAMER_gstreamer-1.0_INCLUDEDIR)
#include_directories(GSTREAMER_gstreamer-video-1.0_INCLUDEDIR)
#include_directories(GSTREAMER_gstreamer-gl-1.0_INCLUDEDIR)

# Tell CMake to create the helloworld executable
add_executable(disupurei
    gstpipeline.cpp
    imageplayer.cpp
    main.cpp
    playlist.cpp
    videoplayer.cpp
    window.cpp
)

target_include_directories(disupurei PRIVATE "${GSTREAMER_INCLUDE_DIRS}")

# Use the Widgets module from Qt 5.
target_link_libraries(disupurei
    Qt5::Network
    Qt5::Widgets
    Qt5::X11Extras
    ${GSTREAMER_LIBRARIES}
)

set_property(TARGET disupurei PROPERTY CXX_STANDARD 11)
set_property(TARGET disupurei PROPERTY CXX_STANDARD_REQURED true)
