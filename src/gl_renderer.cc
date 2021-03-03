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
#include <algorithm>
#include <mutex>

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

GLRenderer::GLRenderer(RenderCtrl* render_ctrl)
{
  render_ctrl_ = render_ctrl;
  pthread_mutex_init(&pixel_mutex_, nullptr);
  memcpy(mvp_matrix_, IDENTITY_MATRIX, 16 * sizeof(float));
}

GLRenderer::~GLRenderer()
{
  pthread_mutex_destroy(&pixel_mutex_);
}

void GLRenderer::bind_window_for_source(void* win, std::string& src_id)
{
  source_id_ = src_id;
  is_window_changed = win != cur_window_;
  cur_window_ = win;
}

int GLRenderer::setup()
{
  return setup_program();
}

int GLRenderer::release()
{
  if (input_texture_) {
    delete input_texture_;
    input_texture_ = nullptr;
  }
  if (input_texture_uv_) {
    delete input_texture_uv_;
    input_texture_uv_ = nullptr;
  }
  if (pixel_buffer_object_) {
    glDeleteBuffers(1, &pixel_buffer_object_);
    pixel_buffer_object_ = 0;
  }
  if (program_) {
    delete program_;
    program_ = nullptr;
  }
  if (cur_window_surface_) render_ctrl_->release_surface(cur_window_surface_);
  cur_window_surface_ = 0;
  return 0;
}

int GLRenderer::upload_texture(uint8_t **data, int num_channel, int width, int height)
{
  if (!data || !*data) return -1;

  pthread_mutex_lock(&pixel_mutex_);
  check_texture_size(width, height);
  pixel_buffer_ = *data;
  is_pixel_updated = true;
//  if (input_texture_) input_texture_->upload_pixels(pixel_buffer_);
//  if (input_texture_uv_) input_texture_uv_->upload_pixels(pixel_buffer_);
//  is_pixel_updated = true;
  pthread_mutex_unlock(&pixel_mutex_);
  return 0;
}

int GLRenderer::upload_texture_internal()
{
  if (is_pixel_updated) {
    pthread_mutex_lock(&pixel_mutex_);
    setup_pixel_buffer();
    glBindBuffer(GL_ARRAY_BUFFER, pixel_buffer_object_);
    if (tex_format_ == PIXEL_FORMAT_RGBA) {
      glBufferSubData(GL_ARRAY_BUFFER, 0, tex_width_ * tex_height_ * 4, pixel_buffer_);
    } else {
      glBufferSubData(GL_ARRAY_BUFFER, 0, tex_width_ * tex_height_ * 2, pixel_buffer_);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    is_pixel_updated = false;
    pthread_mutex_unlock(&pixel_mutex_);

    if (input_texture_) input_texture_->upload_pixel_from_pbo(pixel_buffer_object_);
    if (input_texture_uv_) input_texture_uv_->upload_pixel_from_pbo(pixel_buffer_object_);
    return 1;
  }
  return 0;
}

int GLRenderer::pre_draw()
{
  if (is_window_changed) {
    if (cur_window_surface_) {
      render_ctrl_->release_surface(cur_window_surface_);
      cur_window_surface_ = EGL_NO_SURFACE;
    }
    if (cur_window_) {
      cur_window_surface_ = render_ctrl_->create_surface(cur_window_);
    }
    is_window_changed = false;
  }
  render_ctrl_->make_current(cur_window_surface_);
  return 0;
}

int GLRenderer::draw()
{
  if (!cur_window_) {
    return 0;
  }

  if (!upload_texture_internal() && !is_force_refresh_) {
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
  glBindTexture(GL_TEXTURE_2D, input_texture_->get_texture());
  glUniform1i(color_map_handle_, 0);
  if (tex_format_ == PIXEL_FORMAT_YUYV && input_texture_uv_) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, input_texture_uv_->get_texture());
    glUniform1i(uv_color_map_handle_, 1);
  }

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisableVertexAttribArray(vertices_handle_);
  glDisableVertexAttribArray(tex_coord_handle_);
  glUseProgram(0);
  render_ctrl_->swap_buffer(cur_window_surface_);

  post_draw();
  int error = glGetError();
  return -error;
}

int GLRenderer::post_draw()
{
  return 0;
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
  if (width == tex_width_ && height == tex_height_) {
    return 0;
  }
  need_reset_pbo_ = true;
  tex_width_ = width;
  tex_height_ = height;
  if (input_texture_) render_ctrl_->return_texture(input_texture_);
  if (input_texture_uv_) render_ctrl_->return_texture(input_texture_uv_);
  if (tex_format_ == PIXEL_FORMAT_RGBA) {
    input_texture_ = render_ctrl_->fetch_texture(tex_width_, tex_height_);
  } else { // YUYV
    Texture::Attributes attr = *Texture::s_default_texture_attributes_;
    attr.format_ = GL_LUMINANCE_ALPHA;
    attr.internal_format_ = GL_LUMINANCE_ALPHA;
    input_texture_ = render_ctrl_->fetch_texture(tex_width_, tex_height_, &attr);
    input_texture_uv_ = render_ctrl_->fetch_texture(tex_width_ >> 1, tex_height_);
  }
  reset_mvp_matrix();
  return 1;
}

int GLRenderer::setup_pixel_buffer()
{
  if (!need_reset_pbo_ && pixel_buffer_object_) return 0;
  if (pixel_buffer_object_) glDeleteBuffers(1, &pixel_buffer_object_);

  glGenBuffers(1, &pixel_buffer_object_);
  glBindBuffer(GL_ARRAY_BUFFER, pixel_buffer_object_);
  if (tex_format_ == PIXEL_FORMAT_RGBA) {
    glBufferData(GL_ARRAY_BUFFER, tex_width_ * tex_height_ * 4, nullptr, GL_DYNAMIC_DRAW);
  } else {
    glBufferData(GL_ARRAY_BUFFER, tex_width_ * tex_height_ * 2, nullptr, GL_DYNAMIC_DRAW);
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  need_reset_pbo_ = false;

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
  color_map_handle_ = glGetUniformLocation(program_id, "color_map");
  uv_color_map_handle_ = glGetUniformLocation(program_id, "uv_color_map");
  vertices_handle_ = glGetAttribLocation(program_id, "model_coords");
  tex_coord_handle_ = glGetAttribLocation(program_id, "tex_coords");
  error = glGetError();
  if (error != GL_NO_ERROR) return -error;

  if (tex_format_ == PIXEL_FORMAT_RGBA) {
    input_texture_ = render_ctrl_->fetch_texture(tex_width_, tex_height_);
  } else { // YUYV
    Texture::Attributes attr = *Texture::s_default_texture_attributes_;
    attr.format_ = GL_LUMINANCE_ALPHA;
    attr.internal_format_ = GL_LUMINANCE_ALPHA;
    input_texture_ = render_ctrl_->fetch_texture(tex_width_, tex_height_, &attr);
    input_texture_uv_ = render_ctrl_->fetch_texture(tex_width_ >> 1, tex_height_);
  }
  error = glGetError();
  return -error;
}
