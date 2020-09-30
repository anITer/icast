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
#ifndef IMAGE_PROC_PROGRAM_H
#define IMAGE_PROC_PROGRAM_H

#include <GLES3/gl3.h>
#include <string>

class GLProgram {
public:
  GLProgram();
  ~GLProgram();

  static GLProgram* create_by_shader_string(const std::string& vertex_shader_source,
                                            const std::string& fragment_shader_source);

  void use();
  static void check_fb_fetch();
  static const std::string& get_fb_fetch_header();
  static const std::string& get_fb_fetch_color();

  GLuint get_id() const { return program_; }

  GLuint get_attrib_location(const std::string& attribute_name);
  GLuint get_uniform_location(const std::string& uniform_name);

private:
  GLuint program_ = -1;

  bool init_with_shader_string(const std::string& vertex_shader_source,
                               const std::string& fragment_shader_source);
};

#endif /* IMAGE_PROC_PROGRAM_H */
