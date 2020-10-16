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

class Texture {
public:
    Texture(int width, int height, int format);
    virtual ~Texture();

    GLuint get_texture();

    void upload_pixel_from_pbo(int pbo);
    void upload_pixels(unsigned char* pixel_buffer);
    // TODO:: using pbo
    void download_pixels(unsigned char* &pixel_buffer) { }

    void set_size(int width, int height);

private:
    bool has_gen_tex_ = false; // has generated texture or not
    GLuint texture_;
    unsigned char* pixel_buffer_ = nullptr;
    std::mutex pixel_lock_;
    bool need_reset_texture_ = false;
    int pixel_format_;
    int width_ = 0;
    int height_ = 0;

private:

    void generate_texture();
    void destroy_texture();
};

#endif /* FILTER_TEXTURE_OBJECT_H */
