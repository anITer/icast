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
#include "texture.h"
#include <cassert>
#include <cstring>

static const int GL_WIDTH_ALIGN_SIZE = 2;

static int gl_width(int width) {
  return (width + GL_WIDTH_ALIGN_SIZE - 1) / GL_WIDTH_ALIGN_SIZE * GL_WIDTH_ALIGN_SIZE;
}

Texture::Attributes* Texture::s_default_texture_attributes_ = new Texture::Attributes();

Texture* Texture::create(int width, int height, Cacheable::Attributes* attribute) {
  Attributes tmp_attribute(*((Attributes*) attribute));
  return new Texture(width, height, &tmp_attribute);
}

Texture::Texture(int width, int height, Attributes* texture_attributes)
:attributes_(*texture_attributes), texture_(0) {
  width_ = width;
  height_ = height;
  pixel_buffer_ = nullptr;
}

Texture::~Texture() {
  destroy_texture();
  texture_ = 0;

  if (pixel_buffer_) {
    pixel_lock_.lock();
    free(pixel_buffer_);
    pixel_buffer_ = nullptr;
    pixel_lock_.unlock();
  }
}

void Texture::upload_pixel_from_pbo(int pbo)
{
  if (texture_ == 0) generate_texture();
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
  glBindTexture(attributes_.target_, texture_);
  glTexSubImage2D(attributes_.target_, 0, 0, 0, width_, height_,
                  attributes_.format_, attributes_.type_, 0);
  glBindTexture(attributes_.target_, 0);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void Texture::upload_pixel_from_buffer(unsigned char *pixel_buffer) {
  if (!pixel_buffer) {
    return;
  }

  pixel_lock_.lock();
  int length = 0;
  switch(attributes_.format_) {
  case GL_RGBA:
    length = width_ * height_ * 4;
    break;
  case GL_LUMINANCE:
  case GL_ALPHA:
    length = width_ * height_;
    break;
  case GL_LUMINANCE_ALPHA:
    length = width_ * height_ * 2;
    break;
  default:
    length = width_ * height_ * 4;
    break;
  }
  if (pixel_buffer_) {
    free(pixel_buffer_);
  }
  pixel_buffer_ = (unsigned char*) malloc(length * sizeof(unsigned char));
  memcpy(pixel_buffer_, pixel_buffer, length);
  pixel_lock_.unlock();
}

void Texture::generate_texture() {
  glGenTextures(1, &texture_);
  glBindTexture(attributes_.target_, texture_);
  glTexParameteri(attributes_.target_, GL_TEXTURE_MIN_FILTER, attributes_.min_filter_);
  glTexParameteri(attributes_.target_, GL_TEXTURE_MAG_FILTER, attributes_.mag_filter_);
  glTexParameteri(attributes_.target_, GL_TEXTURE_WRAP_S, attributes_.wrap_s_);
  glTexParameteri(attributes_.target_, GL_TEXTURE_WRAP_T, attributes_.wrap_t_);
  if (attributes_.target_ == GL_TEXTURE_2D) { // oes texture should be managed by the SF-service
    glTexImage2D(attributes_.target_, 0, attributes_.internal_format_, gl_width(width_), height_,
                 0, attributes_.format_, attributes_.type_, 0);
  }
  // TODO: Handle mipmaps
  glBindTexture(attributes_.target_, 0);
  has_gen_tex_ = true;
}

void Texture::destroy_texture() {
  if (texture_ != 0 && has_gen_tex_) {
    glDeleteTextures(1, &texture_);
    has_gen_tex_ = false;
    texture_ = -1;
  }
}

GLuint Texture::get_texture() {
  if (texture_ == 0) {
    destroy_texture();
    generate_texture();
  }
  if (pixel_buffer_) {
    pixel_lock_.lock();

    glBindTexture(attributes_.target_, texture_);
    glTexSubImage2D(attributes_.target_, 0, 0, 0, width_, height_,
            attributes_.format_, attributes_.type_, pixel_buffer_);
    glBindTexture(attributes_.target_, 0);
    free(pixel_buffer_);
    pixel_buffer_ = nullptr;
    pixel_lock_.unlock();
  }
  return texture_;
}
