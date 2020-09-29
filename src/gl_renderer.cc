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
#include "gl_renderer.h"
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/time.h>
#include <algorithm>
#include <mutex>

static const int DEFAULT_FRAME_TIME_US = 1000000 / 30;
static const int GL_WIDTH_ALIGN_SIZE = 2;
static const float IDENTITY_MATRIX[16] = {
          1.0, 0.0, 0.0, 0.0,
          0.0, 1.0, 0.0, 0.0,
          0.0, 0.0, 1.0, 0.0,
          0.0, 0.0, 0.0, 1.0
};

static const float VERTEX_COORDS[8] = {
  -1, -1, 1, -1, -1, 1, 1, 1,
};

static const float TEXTURE_COORDS[8] = {
  0, 0, 1, 0, 0, 1, 1, 1,
};

static char* read_string(const char* path)
{
  if (!path) return nullptr;

  FILE *fp = fopen(path, "r");
  if (!fp) return nullptr;

  fseek(fp, 0L, SEEK_END);
  long int len = ftell(fp);
  char* res = (char *)malloc(len + 1);
  memset(res, 0x0, len + 1);
  if (!res) {
    fclose(fp);
    return nullptr; // TODO::log error oom
  }

  fseek(fp, 0L, SEEK_SET);
  char *tmp = nullptr;
  char *cur = res;
  while ((tmp = fgets(cur, len, fp)) != NULL) {
    cur += strlen(tmp);
  }
  fclose(fp);

  return res;
}

static int get_gl_width(int width) {
  return (width + GL_WIDTH_ALIGN_SIZE - 1) / GL_WIDTH_ALIGN_SIZE * GL_WIDTH_ALIGN_SIZE;
}

void* GLRenderer::render_loop(void* data)
{
  GLRenderer* renderer = (GLRenderer *)data;
  renderer->setup();

  long start = 0;
  long end = DEFAULT_FRAME_TIME_US;
  timeval tmp_time;
  while (renderer->is_running_) {
    usleep(std::max(0L, DEFAULT_FRAME_TIME_US - (end - start)));
    if (!renderer->is_running_) break;

    gettimeofday(&tmp_time, nullptr);
    start = tmp_time.tv_sec * 1000000 + tmp_time.tv_usec;
    if (renderer->upload_texture_internal()
     || renderer->is_force_refresh_) {
      renderer->bind_window_internal();
      renderer->draw();
      renderer->is_force_refresh_ = false;
    }
    gettimeofday(&tmp_time, nullptr);
    end = tmp_time.tv_sec * 1000000 + tmp_time.tv_usec;
  }

  renderer->destroy();
  return nullptr;
}

GLRenderer::GLRenderer()
{
  pthread_mutex_init(&pixel_mutex_, nullptr);
  memcpy(mvp_matrix_, IDENTITY_MATRIX, 16 * sizeof(float));
}

GLRenderer::~GLRenderer()
{
  stop();
  pthread_mutex_destroy(&pixel_mutex_);
}

void GLRenderer::start()
{
  if (is_running_) return;
  is_running_ = true;
  pthread_create(&render_thread, nullptr, render_loop, this);
}

void GLRenderer::stop()
{
  if (!is_running_) return;
  is_running_ = false;
  pthread_join(render_thread, nullptr);
}

int GLRenderer::setup()
{
  if (cur_eglcore_) return 0;

  cur_eglcore_ = new EglCore();
  cur_background_surface_ = cur_eglcore_->create_offscreen_surface(1, 1);
  cur_eglcore_->make_current(cur_background_surface_);
  return setup_program();
}

int GLRenderer::destroy()
{
  if (input_texture_) {
    glDeleteTextures(1, &input_texture_);
    input_texture_ = 0;
  }
  if (program_) {
    delete program_;
    program_ = nullptr;
  }
  if (cur_eglcore_) {
    if (cur_background_surface_) cur_eglcore_->release_surface(cur_background_surface_);
    cur_background_surface_ = 0;
    if (cur_window_surface_) cur_eglcore_->release_surface(cur_window_surface_);
    cur_window_surface_ = 0;
    delete cur_eglcore_;
  }
  cur_eglcore_ = nullptr;
  return 0;
}

int GLRenderer::upload_texture(uint8_t **data, int num_channel, int width, int height)
{
  if (!data || !*data) return -1;

  pthread_mutex_lock(&pixel_mutex_);
  check_texture_size(width, height);
  pixel_buffer_ = *data;
  pthread_mutex_unlock(&pixel_mutex_);
  is_pixel_updated = true;
  return 0;
}

int GLRenderer::upload_texture_internal()
{
  if (is_pixel_updated) {
    pthread_mutex_lock(&pixel_mutex_);
    setup_texture();
    glBindTexture(GL_TEXTURE_2D, input_texture_);
    if (tex_format_ == PIXEL_FORMAT_RGBA) {
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width_, tex_height_, GL_RGBA, GL_UNSIGNED_BYTE, pixel_buffer_);
    } else { // YUYV for camera
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width_ >> 1, tex_height_, GL_RGBA, GL_UNSIGNED_BYTE, pixel_buffer_);
    } // support other formats
    pthread_mutex_unlock(&pixel_mutex_);
    glBindTexture(GL_TEXTURE_2D, 0);
    is_pixel_updated = false;
    return 1;
  }
  return 0;
}

int GLRenderer::pre_draw()
{
  return 0;
}

int GLRenderer::draw()
{
  if (!cur_window_surface_) {
    return 0;
  }

  pre_draw();

  glViewport(0, 0, output_width_, output_height_);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(program_->get_id());

  //顶点
  glEnableVertexAttribArray(vertices_handle_);
  glVertexAttribPointer(vertices_handle_, 2, GL_FLOAT, 0, 0, VERTEX_COORDS);
  //纹理坐标
  glEnableVertexAttribArray(tex_coord_handle_);
  glVertexAttribPointer(tex_coord_handle_, 2, GL_FLOAT, 0, 0, TEXTURE_COORDS);
  //MVP矩阵
  glUniformMatrix4fv(mvp_matrix_handle_, 1, GL_FALSE, mvp_matrix_);

  //纹理
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, input_texture_);
  glUniform1i(color_map_handle_, 0);

  if (tex_width_handle_ >= 0) {
    glUniform1f(tex_width_handle_, (float) (1.0 / output_width_));
  }

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisableVertexAttribArray(vertices_handle_);
  glDisableVertexAttribArray(tex_coord_handle_);
  glUseProgram(0);
  cur_eglcore_->swap_buffers(cur_window_surface_);

  post_draw();
  int error = glGetError();
  return -error;
}

int GLRenderer::post_draw()
{
  return 0;
}

int GLRenderer::bind_window(XID win_id)
{
  if (cur_window_id_ && cur_window_id_ == win_id) return 0;

  cur_window_id_ = win_id;
  is_window_id_changed_ = true;
  return 0;
}

int GLRenderer::bind_window_internal()
{
  if (!is_window_id_changed_) return 0;
  if (cur_window_surface_) {
    cur_eglcore_->release_surface(cur_window_surface_);
    cur_window_surface_ = 0;
  }
  if (cur_window_id_ ) {
    cur_window_surface_ = cur_eglcore_->create_window_surface(cur_window_id_);
    cur_eglcore_->make_current(cur_window_surface_);
  } else {
    cur_eglcore_->make_current(cur_background_surface_);
  }
  is_window_id_changed_ = false;
  return 1;
}

void GLRenderer::set_output_size(int width, int height)
{
  if (width == output_width_ && height == output_height_) {
    return;
  }
  is_force_refresh_ = true;
  output_width_ = width;
  output_height_ = height;
  reset_mvp_matrix();
}

void GLRenderer::reset_mvp_matrix()
{
  mvp_matrix_[5] = -1.0; // flip virtically
  mvp_matrix_[0] = 1.0; // flip horizontally
  if (tex_scale_type_ == SCALE_TYPE_STRETCH || output_height_ * tex_height_ == 0) return;

  float scale = (float) output_width_ / output_height_ / ((float) tex_width_ / tex_height_);

  switch(tex_scale_type_) {
  case SCALE_TYPE_SCALE_FIT:
    if (scale < 1.0) {
      mvp_matrix_[5] *= scale;
    } else {
      mvp_matrix_[0] /= scale;
    }
    break;
  case SCALE_TYPE_CROP_FIT:
    if (scale > 1.0) {
      mvp_matrix_[5] *= scale;
    } else {
      mvp_matrix_[0] /= scale;
    }
    break;
  default:
    break;
  }
}

void GLRenderer::set_texture_format(PixelFormat format)
{
  tex_format_ = format;
}

void GLRenderer::set_scale_type(ScaleType type)
{
  tex_scale_type_ = type;
}

int GLRenderer::check_texture_size(int width, int height)
{
  if (width != tex_width_ || height != tex_height_) {
    is_tex_size_changed_ = true;
  }
  tex_width_ = width;
  tex_height_ = height;
  reset_mvp_matrix();
  return is_tex_size_changed_;
}

int GLRenderer::setup_texture()
{
  if (input_texture_) {
    if (is_tex_size_changed_) {
      glDeleteTextures(1, &input_texture_);
    } else {
      return 0;
    }
  }
  glGenTextures(1, &input_texture_);
  glBindTexture(GL_TEXTURE_2D, input_texture_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  if (tex_format_ == PIXEL_FORMAT_RGBA) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, get_gl_width(tex_width_), tex_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  } else { // YUYV for camera
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, get_gl_width(tex_width_) >> 1, tex_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  }
  is_tex_size_changed_ = false;
  int error = glGetError();
  return -error;
}

int GLRenderer::setup_program()
{
  char* vert_str = read_string("../icast/demo/res/shader/vertex.vsh");
  char* frag_str = tex_format_ == PIXEL_FORMAT_RGBA
        ? read_string("../icast/demo/res/shader/rgba_fragment.fsh")
        : read_string("../icast/demo/res/shader/yuyv_fragment.fsh");
  program_ = GLProgram::create_by_shader_string(vert_str, frag_str);
  free(vert_str);
  free(frag_str);
  int error = glGetError();
  if (error != GL_NO_ERROR) return -error;

  program_->use();
  error = glGetError();
  if (error != GL_NO_ERROR) return -error;

  GLuint program_id = program_->get_id();
  mvp_matrix_handle_ = glGetUniformLocation(program_id, "mvp_matrix");
  tex_width_handle_ = glGetUniformLocation(program_id, "tex_width");
  color_map_handle_ = glGetUniformLocation(program_id, "color_map");
  vertices_handle_ = glGetAttribLocation(program_id, "model_coords");
  tex_coord_handle_ = glGetAttribLocation(program_id, "tex_coords");
  error = glGetError();
  return -error;
}
