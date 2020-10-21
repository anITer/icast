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
#ifndef FILTER_TEXTURE_OBJECT_H
#define FILTER_TEXTURE_OBJECT_H

#include <GLES3/gl3.h>
#include <mutex>
#include <vector>
#include "cacheable.h"

class Texture : public Cacheable {
public:
  struct Attributes : Cacheable::Attributes {
    GLenum min_filter_ = GL_LINEAR;
    GLenum mag_filter_ = GL_LINEAR;
    GLenum wrap_s_ = GL_CLAMP_TO_EDGE;
    GLenum wrap_t_ = GL_CLAMP_TO_EDGE;
    GLenum internal_format_ = GL_RGBA;
    GLenum format_ = GL_RGBA;
    GLenum type_ = GL_UNSIGNED_BYTE;
    GLenum target_ = GL_TEXTURE_2D;

    inline std::string get_hash() const override {
      return str_format("%d:%d:%d:%d:%d:%d:%d:%d",
                        min_filter_, mag_filter_, wrap_s_, wrap_t_,
                        internal_format_, format_, type_, target_);
    }
  };

  static Texture* create(int width, int height,
      Cacheable::Attributes* attribute = s_default_texture_attributes_);

  Texture(int width, int height, Attributes* texture_attributes = s_default_texture_attributes_);
  virtual ~Texture();

  GLuint get_texture();

  bool need_be_cached() override { return has_gen_tex_; }

  void upload_pixel_from_pbo(int pbo);
  void upload_pixel_from_buffer(unsigned char* pixel_buffer);

  void download_pixels(unsigned char* &pixel_buffer) { /* TODO:: using pbo */ }

  inline Cacheable::Attributes* get_attributes() const override { return (Cacheable::Attributes*) &attributes_; };
  inline const Attributes& get_texture_attributes() const { return attributes_; };

  static Attributes* s_default_texture_attributes_;

private:
  bool has_gen_tex_ = false;
  Attributes attributes_;
  GLuint texture_ = 0;
  unsigned char* pixel_buffer_ = nullptr;
  std::mutex pixel_lock_;
  bool need_reset_texture_ = false;

private:

  void generate_texture();
  void destroy_texture();
};

#endif /* FILTER_TEXTURE_OBJECT_H */
