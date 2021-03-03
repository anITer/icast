cmake_minimum_required(VERSION 3.13)

project(icast LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)

INCLUDE_DIRECTORIES()
LINK_DIRECTORIES()
include_directories(
        include
        ${X11_INCLUDE_DIR}
        ${OPENGL_INCLUDE_DIRS}
)
add_library(icast SHARED
  include/x_window_env.h
  include/v4l2.h
  include/capture_interface.h
  include/camera_device.h
  include/screen_capturer.h
  include/window_capturer.h
  include/cursor_capturer.h
  include/composite_capturer.h
  include/gl_renderer.h
  include/render_ctrl.h
  include/egl_core.h
  include/program.h
  include/texture.h
  include/cacheable.h
  include/object_cacher.h
  src/v4l2.cc
  src/x_window_env.cc
  src/camera_device.cc
  src/screen_capturer.cc
  src/window_capturer.cc
  src/cursor_capturer.cc
  src/composite_capturer.cc
  src/gl_renderer.cc
  src/render_ctrl.cc
  src/egl_core.cc
  src/program.cc
  src/texture.cc
)
target_link_libraries(icast ${X11_LIBRARIES}       # libx11-dev
                            Xfixes                 # libxfixes-dev
                            Xdamage                # libxdamage-dev
                            Xinerama               # libxinerama-dev
                            Xcomposite             # libxcomposite-dev
                            ${OPENGL_LIBRARY_DIRS} # libgles2-mesa-dev
                            EGL                    # libegl1-mesa-dev
                        )

# Next lines needed for building all Qt projects
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5Gui)
include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})

# using Qt meta-system (precompiler)
QT5_ADD_RESOURCES( RES_SOURCES demo/res/shader.qrc )
QT5_WRAP_UI( UI_HEADERS demo/widget.ui )

add_executable(sample
        demo/video_widget.h
        demo/video_widget.cpp
        demo/main.cpp
        ${RES_SOURCES}
        ${UI_HEADERS}
)
target_link_libraries(sample icast Qt5::Widgets wayland-egl)
