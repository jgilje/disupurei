cmake_minimum_required(VERSION 2.8.11)

project(disupurei)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FindASan)
include(FindMSan)
include(FindTSan)
include(FindUBSan)

find_package(Qt5Network)
find_package(Qt5Widgets)

IF(WIN32)
	SET(GSTREAMER_ROOT "C:/gstreamer/1.0/x86_64")
ENDIF(WIN32)

FIND_LIBRARY(GOBJECT gobject-2.0 HINTS ${GSTREAMER_ROOT} ENV GSTREAMER_ROOT PATH_SUFFIXES lib)
FIND_LIBRARY(GLIB glib-2.0 HINTS ${GSTREAMER_ROOT} ENV GSTREAMER_ROOT PATH_SUFFIXES lib)
FIND_LIBRARY(GSTREAMER gstreamer-1.0 HINTS ${GSTREAMER_ROOT} ENV GSTREAMER_ROOT PATH_SUFFIXES lib)
FIND_LIBRARY(GSTREAMER_BASE gstbase-1.0 HINTS ${GSTREAMER_ROOT} ENV GSTREAMER_ROOT PATH_SUFFIXES lib)
FIND_LIBRARY(GSTREAMER_VIDEO gstvideo-1.0 HINTS ${GSTREAMER_ROOT} ENV GSTREAMER_ROOT PATH_SUFFIXES lib)
FIND_LIBRARY(GSTREAMER_GL gstgl-1.0 HINTS ${GSTREAMER_ROOT} ENV GSTREAMER_ROOT PATH_SUFFIXES lib)

get_filename_component(GLIB_LIB_DIR ${GLIB} DIRECTORY)
get_filename_component(GSTREAMER_LIB_DIR ${GSTREAMER} DIRECTORY)

find_path(GLIB_INCLUDE glib.h HINTS ${GSTREAMER_ROOT} ENV GSTREAMER_ROOT PATH_SUFFIXES include/glib-2.0)
find_path(GLIB_CONFIG_INCLUDE glibconfig.h HINTS ${GLIB_LIB_DIR} PATH_SUFFIXES glib-2.0/include)
find_path(GSTREAMER_INCLUDE gst/gst.h HINTS ${GSTREAMER_ROOT} ENV GSTREAMER_ROOT PATH_SUFFIXES include/gstreamer-1.0)
find_path(GSTREAMER_CONFIG_INCLUDE gst/gstconfig.h HINTS ${GSTREAMER_LIB_DIR} PATH_SUFFIXES gstreamer-1.0/include)

LIST(APPEND GLIB_INCLUDE_DIRS ${GLIB_INCLUDE})
LIST(APPEND GLIB_INCLUDE_DIRS ${GLIB_CONFIG_INCLUDE})
LIST(APPEND GSTREAMER_INCLUDE_DIRS ${GSTREAMER_INCLUDE})
LIST(APPEND GSTREAMER_INCLUDE_DIRS ${GSTREAMER_CONFIG_INCLUDE})

IF(WIN32)
	# Need the gl.h and glext.h from GSTREAMER install on Windows
	LIST(APPEND GSTREAMER_INCLUDE_DIRS ${GSTREAMER_ROOT}/include)
ENDIF(WIN32)

IF ("${PLATFORM}" STREQUAL "raspberry-pi")
	FIND_LIBRARY(GLESV2 GLESv2 PATHS "/opt/vc/lib")
	set(PLATFORM_INCLUDES "${CMAKE_SYSROOT}/opt/vc/include/interface/vcos/pthreads")
	LIST(APPEND PLATFORM_LIBRARIES ${GLESV2})
ENDIF()

IF(UNIX)
	find_package(Qt5X11Extras)
	SET(PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} "Qt5::X11Extras")
ENDIF()

add_executable(disupurei
    gstpipeline.cpp
    imageplayer.cpp
    main.cpp
    playlist.cpp
    videoplayer.cpp
    window.cpp
)

target_include_directories(disupurei PRIVATE "${GSTREAMER_INCLUDE_DIRS}")
target_include_directories(disupurei PRIVATE "${GLIB_INCLUDE_DIRS}")
target_include_directories(disupurei SYSTEM PRIVATE "${PLATFORM_INCLUDES}")

target_link_libraries(disupurei
    Qt5::Network
    Qt5::Widgets
    ${GOBJECT}
    ${GLIB}
    ${GSTREAMER}
    ${GSTREAMER_BASE}
    ${GSTREAMER_VIDEO}
    ${GSTREAMER_GL}
    ${PLATFORM_LIBRARIES}
)

set_property(TARGET disupurei PROPERTY CXX_STANDARD 11)
set_property(TARGET disupurei PROPERTY CXX_STANDARD_REQURED true)
