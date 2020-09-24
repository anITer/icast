cmake_minimum_required(VERSION 3.13)

project(icast LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(X11 REQUIRED)
include_directories(
        include
        ${X11_INCLUDE_DIR}
)
add_library(icast SHARED
  include/v4l2.h
  include/capture_interface.h
  include/camera_device.h
  include/screen_capturer.h
  include/window_capturer.h
  include/cursor_capturer.h
  include/composite_capturer.h
  src/v4l2.cc
  src/camera_device.cc
  src/screen_capturer.cc
  src/window_capturer.cc
  src/cursor_capturer.cc
  src/composite_capturer.cc
)
target_link_libraries(icast ${X11_LIBRARIES}       # libx11-dev
                            ${X11_Xfixes_LIB}      # libxfixes-dev
                            ${X11_Xdamage_LIB}     # libxdamage-dev
                            ${X11_Xinerama_LIB}    # libxinerama-dev
                            ${X11_Xcomposite_LIB}  # libxcomposite-dev
                        )

# Next lines needed for building all Qt projects
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# using Qt meta-system (precompiler)
QT5_ADD_RESOURCES( RES_SOURCES demo/res/shader.qrc )
#QT5_WRAP_UI( UI_HEADERS demo/widget.ui )

add_executable(sample
        demo/glwidget.h
        demo/glwidget.cpp
        demo/main.cpp
        ${RES_SOURCES}
        ${UI_HEADERS}
)
target_link_libraries(sample icast Qt5::Widgets)
