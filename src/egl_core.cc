/*
 * MIT License
 *
 * Copyright (c) 2020 Andy Young
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "egl_core.h"
#include <iostream>
#include <cassert>

EglCore::EglCore() : egl_display_(EGL_NO_DISPLAY)
                   , egl_config_(nullptr)
                   , egl_context_(EGL_NO_CONTEXT)
                   , gl_version_(-1) {
  init(nullptr, 0);
}

EglCore::~EglCore() {
  destroy();
}

/**
 * 构造方法
 * @param shared_context
 * @param flags
 */
EglCore::EglCore(EGLContext shared_context, int flags) {
  init(shared_context, flags);
}

/**
 * 初始化
 * @param shared_context
 * @param flags
 * @return
 */
bool EglCore::init(EGLContext shared_context, int flags) {
  assert(egl_display_ == EGL_NO_DISPLAY);
  if (egl_display_ != EGL_NO_DISPLAY) {
    //ALOGE(TAG, "EGL already set up");
    return false;
  }
  if (shared_context == nullptr) {
    shared_context = EGL_NO_CONTEXT;
  }

  egl_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  assert(egl_display_ != EGL_NO_DISPLAY);
  if (egl_display_ == EGL_NO_DISPLAY) {
    //ALOGE(TAG, "unable to get EGL14 display.\n");
    return false;
  }

  if (!eglInitialize(egl_display_, 0, 0)) {
    egl_display_ = EGL_NO_DISPLAY;
    //ALOGE(TAG, "unable to initialize EGL14");
    return false;
  }

  // 尝试使用GLES3
  if ((flags & FLAG_TRY_GLES3) != 0) {
    EGLConfig config = get_config(flags, 3);
    if (config != nullptr) {
      int attrib3_list[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
      };
      EGLContext context = eglCreateContext(egl_display_, config,
                                            shared_context, attrib3_list);
      if (eglGetError() == EGL_SUCCESS) {
        egl_config_ = config;
        egl_context_ = context;
        gl_version_ = 3;
      }
    }
  }
  // 如果GLES3没有获取到，则尝试使用GLES2
  if (egl_context_ == EGL_NO_CONTEXT) {
    EGLConfig config = get_config(flags, 2);
    assert(config != nullptr);
    int attrib2_list[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLContext context = eglCreateContext(egl_display_, config,
                                          shared_context, attrib2_list);
    if (eglGetError() == EGL_SUCCESS) {
      egl_config_ = config;
      egl_context_ = context;
      gl_version_ = 2;
    }
  }

  int values[1] = {0};
  eglQueryContext(egl_display_, egl_context_, EGL_CONTEXT_CLIENT_VERSION, values);
  // ALOGD(TAG, "EGLContext created, client version %d", values[0]);

  return true;
}


/**
 * 获取合适的EGLConfig
 * @param flags
 * @param version
 * @return
 */
EGLConfig EglCore::get_config(int flags, int version) {
  int attrib_list[] = {
    EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
    EGL_BUFFER_SIZE,       32,
    EGL_RED_SIZE,          8,
    EGL_GREEN_SIZE,        8,
    EGL_BLUE_SIZE,         8,
    EGL_ALPHA_SIZE,        8,
//    EGL_DEPTH_SIZE,      24,
//    EGL_STENCIL_SIZE,    8,

    EGL_SAMPLE_BUFFERS,    0,
    EGL_SAMPLES,           0,

    EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE,   EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };
  EGLConfig configs = nullptr;
  int num_configs = 0;
  if (!eglChooseConfig(egl_display_, attrib_list, &configs, 1, &num_configs)) {
    //ALOGW(TAG, "unable to find RGB8888 / %d  EGLConfig", version);
    return nullptr;
  }
  return configs;
}

/**
 * 释放资源
 */
void EglCore::destroy() {
  if (egl_display_ != EGL_NO_DISPLAY) {
    eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(egl_display_, egl_context_);
    eglReleaseThread();
    eglTerminate(egl_display_);
  }

  egl_display_ = EGL_NO_DISPLAY;
  egl_context_ = EGL_NO_CONTEXT;
  egl_config_ = nullptr;
}

/**
 * 获取EGLContext
 * @return
 */
EGLContext EglCore::get_egl_context() {
  return egl_context_;
}

/**
 * 销毁EGLSurface
 * @param egl_surface
 */
void EglCore::release_surface(EGLSurface egl_surface) {
  eglDestroySurface(egl_display_, egl_surface);
}

/**
 * 创建EGLSurface
 * @param surface
 * @return
 */
EGLSurface EglCore::create_window_surface(Window win_id) {
  if (win_id == 0) {
    //ALOGE(TAG, "ANativeWindow is nullptr!");
    return EGL_NO_SURFACE;
  }
  int surface_attribs[] = {
      EGL_NONE
  };
  //ALOGD(TAG, "eglCreateWindowSurface start");
  EGLSurface egl_surface = eglCreateWindowSurface(egl_display_, egl_config_, win_id, surface_attribs);
  if (egl_surface == EGL_NO_SURFACE) {
    //ALOGE(TAG, "eglCreateWindowSurface returned EGL_NO_SURFACE");
  }
  return egl_surface;
}

/**
 * 创建离屏渲染的EGLSurface
 * @param width
 * @param height
 * @return
 */
EGLSurface EglCore::create_offscreen_surface(int width, int height) {
  int surface_attribs[] = {
    EGL_WIDTH, width,
    EGL_HEIGHT, height,
    EGL_NONE
  };
  EGLSurface egl_surface = eglCreatePbufferSurface(egl_display_, egl_config_, surface_attribs);
  if (egl_surface == EGL_NO_SURFACE) {
    //ALOGE(TAG, "eglCreatePbufferSurface returned EGL_NO_SURFACE");
  }
  return egl_surface;
}

/**
 * 切换到当前的上下文
 * @param egl_surface
 */
void EglCore::make_current(EGLSurface egl_surface) {
  if (egl_display_ == EGL_NO_DISPLAY) {
    //ALOGD(TAG, "Note: make_current w/o display.\n");
  }
  if (!eglMakeCurrent(egl_display_, egl_surface, egl_surface, egl_context_)) {
    //ALOGE(TAG, "ERROR: make_current surface[%p] failed", egl_surface);
    // TODO 抛出异常
  }
}

/**
 * 切换到某个上下文
 * @param draw_surface
 * @param read_surface
 */
void EglCore::make_current(EGLSurface draw_surface, EGLSurface read_surface) {
  if (egl_display_ == EGL_NO_DISPLAY) {
    //ALOGD(TAG, "Note: make_current w/o display.\n");
  }
  if (!eglMakeCurrent(egl_display_, draw_surface, read_surface, egl_context_)) {
    //ALOGE(TAG, "ERROR: make_current surface[%p|%p] failed", draw_surface, read_surface);
    // TODO 抛出异常
  }
}

/**
 *
 */
void EglCore::make_nothing_current() {
  if (!eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
    //ALOGE(TAG, "ERROR: make_nothing_current failed");
    // TODO 抛出异常
  }
}

/**
 * 交换显示
 * @param egl_surface
 * @return
 */
bool EglCore::swap_buffers(EGLSurface egl_surface) {
  return eglSwapBuffers(egl_display_, egl_surface);
}

/**
 * 是否处于当前上下文
 * @param egl_surface
 * @return
 */
bool EglCore::is_current(EGLSurface egl_surface) {
  return egl_context_ == eglGetCurrentContext() &&
         egl_surface == eglGetCurrentSurface(EGL_DRAW);
}

/**
 * 查询surface
 * @param egl_surface
 * @param what
 * @return
 */
int EglCore::query_surface(EGLSurface egl_surface, int what) {
  int value = 0;
  eglQuerySurface(egl_context_, egl_surface, what, &value);
  return value;
}

/**
 * 查询字符串
 * @param what
 * @return
 */
const char* EglCore::query_string(int what) {
  return eglQueryString(egl_display_, what);
}

/**
 * 获取GLES版本号
 * @return
 */
int EglCore::get_gl_version() {
  return gl_version_;
}
