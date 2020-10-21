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
#ifndef RENDER_CTRL_H
#define RENDER_CTRL_H

#include <pthread.h>
#include <map>
#include "egl_core.h"
#include "texture.h"
#include "object_cacher.h"

class GLRenderer;

class RenderCtrl
{
public:
  RenderCtrl();
  virtual ~RenderCtrl();

  void start();
  void stop();

  void set_fps(float fps);

  void add_renderer(GLRenderer* renderer);
  void remove_renderer(GLRenderer* renderer);
  void clear_renderers();

  EGLSurface create_surface(Window &window);
  void release_surface(EGLSurface& surface);
  void swap_buffer(EGLSurface& surface);
  void make_current(EGLSurface& surface);

  Texture* fetch_texture(int width, int height,
                         Cacheable::Attributes* attribute = Texture::s_default_texture_attributes_);
  void return_texture(Texture* texture);

private:
  void setup_egl();
  void release_egl();
  void setup_renderers();
  void do_rendering();
  void release_renderers();
  static void* render_loop(void* data);

  pthread_t  render_thread_;
  volatile bool is_running_ = false;

  volatile int interval_us_ = 1000000 / 30;

  EglCore   *egl_core_ = nullptr;
  EGLSurface cur_background_surface_ = 0;
  ObjectCacher<Texture, Texture::Attributes> texture_cache_;

  pthread_mutex_t render_lock_;
  std::map<GLRenderer*, int> renderer_list_;
};

#endif // RENDER_CTRL_H
