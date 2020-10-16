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

static int get_gl_width(int width) {
  return (width + GL_WIDTH_ALIGN_SIZE - 1) / GL_WIDTH_ALIGN_SIZE * GL_WIDTH_ALIGN_SIZE;
}

Texture::Texture(int width, int height, int format)
:texture_(-1) {
    width_ = width;
    height_ = height;
    pixel_format_ = format;
    pixel_buffer_ = nullptr;
}

Texture::~Texture() {
    destroy_texture();

    if (pixel_buffer_) {
        pixel_lock_.lock();
        free(pixel_buffer_);
        pixel_buffer_ = nullptr;
        pixel_lock_.unlock();
    }
}

void Texture::set_size(int width, int height)
{
    if (width_ == width && height_ == height) return;
    width_ = width;
    height_ = height;
    need_reset_texture_ = true;
}

void Texture::upload_pixel_from_pbo(int pbo)
{
    if (texture_ == -1) generate_texture();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, pixel_format_, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void Texture::upload_pixels(unsigned char *pixel_buffer) {
    if (!pixel_buffer) {
        return;
    }

    pixel_lock_.lock();
    int length = 0;
    if (pixel_format_ == GL_RGBA) {
        length = width_ * height_ * 4;
    } else if (pixel_format_ == GL_LUMINANCE || pixel_format_ == GL_ALPHA) {
        length = width_ * height_;
    } else if (pixel_format_ == GL_LUMINANCE_ALPHA) {
        length = width_ * height_ * 2;
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
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, pixel_format_, get_gl_width(width_), height_, 0, pixel_format_, GL_UNSIGNED_BYTE, 0);
    // TODO: Handle mipmaps
    glBindTexture(GL_TEXTURE_2D, 0);
    has_gen_tex_ = true;
}

void Texture::destroy_texture() {
    if (texture_ != -1 && has_gen_tex_) {
        glDeleteTextures(1, &texture_);
        has_gen_tex_ = false;
        texture_ = -1;
    }
}

GLuint Texture::get_texture() {
    if (texture_ == -1 || need_reset_texture_) {
        destroy_texture();
        generate_texture();
        need_reset_texture_ = false;
    }
    if (pixel_buffer_) {
        pixel_lock_.lock();

        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, pixel_format_, GL_UNSIGNED_BYTE, pixel_buffer_);
        glBindTexture(GL_TEXTURE_2D, 0);
        if (pixel_buffer_) free(pixel_buffer_);
        pixel_buffer_ = nullptr;
        pixel_lock_.unlock();
    }
    return texture_;
}
