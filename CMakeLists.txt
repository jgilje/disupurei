cmake_minimum_required(VERSION 2.8.11)

project(disupurei)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

find_package(Qt5Network)
find_package(Qt5Widgets)
IF(UNIX)
	find_package(Qt5X11Extras)

	include(FindPkgConfig)
	pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-video-1.0 gstreamer-gl-1.0)

	SET(PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} "Qt5::X11Extras")
ENDIF(UNIX)
IF(WIN32)
	# Using the paths from the installer at http://gstreamer.freedesktop.org/data/pkg/windows/
	SET(GSTREAMER_LIBRARIES ${GSTREAMER_LIBRARIES} "C:/gstreamer/1.0/x86_64/lib/glib-2.0.lib")
	SET(GSTREAMER_LIBRARIES ${GSTREAMER_LIBRARIES} "C:/gstreamer/1.0/x86_64/lib/gobject-2.0.lib")
	SET(GSTREAMER_LIBRARIES ${GSTREAMER_LIBRARIES} "C:/gstreamer/1.0/x86_64/lib/gstreamer-1.0.lib")
	SET(GSTREAMER_LIBRARIES ${GSTREAMER_LIBRARIES} "C:/gstreamer/1.0/x86_64/lib/gstvideo-1.0.lib")
	SET(GSTREAMER_LIBRARIES ${GSTREAMER_LIBRARIES} "C:/gstreamer/1.0/x86_64/lib/gstgl-1.0.lib")
	SET(GSTREAMER_INCLUDE_DIRS ${GSTREAMER_INCLUDE_DIRS} "C:/gstreamer/1.0/x86_64/include")
	SET(GSTREAMER_INCLUDE_DIRS ${GSTREAMER_INCLUDE_DIRS} "C:/gstreamer/1.0/x86_64/include/gstreamer-1.0")
	SET(GSTREAMER_INCLUDE_DIRS ${GSTREAMER_INCLUDE_DIRS} "C:/gstreamer/1.0/x86_64/include/glib-2.0")
	SET(GSTREAMER_INCLUDE_DIRS ${GSTREAMER_INCLUDE_DIRS} "C:/gstreamer/1.0/x86_64/lib/gstreamer-1.0/include")
	SET(GSTREAMER_INCLUDE_DIRS ${GSTREAMER_INCLUDE_DIRS} "C:/gstreamer/1.0/x86_64/lib/glib-2.0/include")
ENDIF(WIN32)

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
    ${GSTREAMER_LIBRARIES}
    ${PLATFORM_LIBRARIES}
)

set_property(TARGET disupurei PROPERTY CXX_STANDARD 11)
set_property(TARGET disupurei PROPERTY CXX_STANDARD_REQURED true)
