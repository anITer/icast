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
#ifndef GL_RENDERER_H
#define GL_RENDERER_H

#include "program.h"
#include "egl_core.h"
#include "texture.h"
#include "capture_interface.h"
#include <pthread.h>

class GLRenderer
{
public:
enum ScaleType
{
  SCALE_TYPE_STRETCH = 0,
  SCALE_TYPE_SCALE_FIT,
  SCALE_TYPE_CROP_FIT,
};

public:
  GLRenderer();
  virtual ~GLRenderer();

  void start();
  void stop();

  int upload_texture(uint8_t** data, int num_channel, int width, int height);
  int bind_window(XID win_id);
  void set_output_size(int width, int height);
  void set_texture_format(PixelFormat format);
  void set_scale_type(ScaleType type = SCALE_TYPE_SCALE_FIT);

protected:
  int setup();
  int destroy();
  int draw();
  virtual int pre_draw();
  virtual int post_draw();
  int upload_texture_internal();
  int bind_window_internal();
  int check_texture_size(int width, int height);
  int setup_pixel_buffer();
  int setup_program();
  static void* render_loop(void* data);
  void reset_mvp_matrix();

  GLProgram *program_ = nullptr;
  int mvp_matrix_handle_ = -1;
  int vertices_handle_ = -1;
  int tex_coord_handle_ = -1;
  int color_map_handle_ = -1;
  int uv_color_map_handle_ = -1;

  float mvp_matrix_[16];
  ScaleType tex_scale_type_ = SCALE_TYPE_SCALE_FIT;
  PixelFormat tex_format_ = PIXEL_FORMAT_YUYV;

  Texture *input_texture_ = nullptr;
  Texture *input_texture_uv_ = nullptr;
  int tex_width_ = 0;
  int tex_height_ = 0;

  GLuint pixel_buffer_object_ = 0;
  volatile bool need_reset_pbo_ = false;
  pthread_mutex_t pixel_mutex_;
  uint8_t* pixel_buffer_ = nullptr;
  volatile bool is_pixel_updated = false;

  int output_width_ = 0;
  int output_height_ = 0;
  volatile bool is_force_refresh_ = false;

  EglCore   *cur_eglcore_ = nullptr;
  EGLSurface cur_background_surface_ = 0;
  EGLSurface cur_window_surface_ = 0;
  Window     cur_window_id_ = 0;
  volatile bool is_window_id_changed_ = false;

  pthread_t  render_thread;
  volatile bool is_running_ = false;
};

#endif // GL_RENDERER_H
