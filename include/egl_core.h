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
#ifndef FILTER_EGL_CORE_H
#define FILTER_EGL_CORE_H

#include <EGL/egl.h>

/**
 * Constructor flag: surface must be recordable.  This discourages EGL from using a
 * pixel format that cannot be converted efficiently to something usable by the video
 * encoder.
 */
const int FLAG_RECORDABLE = 0x01;

/**
 * Constructor flag: ask for GLES3, fall back to GLES2 if not available.  Without this
 * flag, GLES2 is used.
 */
const int FLAG_TRY_GLES3 = 002;

class EglCore {

public:
  EglCore();

  EglCore(EGLContext shared_context, int flags);

  ~EglCore();

  // 获取EglContext
  EGLContext get_egl_context();

  // 销毁Surface
  void release_surface(EGLSurface egl_surface);

  //  创建EGLSurface
  EGLSurface create_window_surface(Window win_id);

  // 创建离屏EGLSurface
  EGLSurface create_offscreen_surface(int width, int height);

  // 切换到当前上下文
  void make_current(EGLSurface egl_surface);

  // 切换到某个上下文
  void make_current(EGLSurface draw_surface, EGLSurface read_surface);

  // 没有上下文
  void make_nothing_current();

  // 交换显示
  bool swap_buffers(EGLSurface egl_surface);

  // 判断是否属于当前上下文
  bool is_current(EGLSurface egl_surface);

  // 执行查询
  int query_surface(EGLSurface egl_surface, int what);

  // 查询字符串
  const char* query_string(int what);

  // 获取当前的GLES 版本号
  int get_gl_version();

private:
  EGLDisplay egl_display_ = EGL_NO_DISPLAY;
  EGLConfig egl_config_ = NULL;
  EGLContext egl_context_ = EGL_NO_CONTEXT;
  int gl_version_ = -1;

  // 查找合适的EGLConfig
  EGLConfig get_config(int flags, int version);

  bool init(EGLContext shared_context, int flags);
  void destroy();

};
#endif /* FILTER_EGL_CORE_H */
