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

#ifndef IMAGE_PROC_CACHEABLE_H
#define IMAGE_PROC_CACHEABLE_H

#include <string>
#include <cstring>
#include <cstdarg>

std::string inline str_format(const char *fmt, ...) {
  std::string str_result = "";
  if (nullptr != fmt) {
    va_list marker;
    va_start(marker, fmt);

    char *buf = nullptr;
    int result = vasprintf(&buf, fmt, marker);

    if (nullptr == buf) {
      va_end(marker);
      return str_result;
    }

    if (result < 0) {
      free(buf);
      va_end(marker);
      return str_result;
    }

    result = (int)strlen(buf);
    str_result.append(buf, result);
    free(buf);
    va_end(marker);
  }
  return str_result;
}

class Cacheable {
public:
  struct Attributes {
    virtual std::string get_hash() const { return ""; }
  };

  static Cacheable* create(int width, int height, Attributes* attribute) { return nullptr; }

  static inline std::string get_hash(int width, int height, const Attributes* attributes) {
    return str_format("%.1dx%.1d-%s", width, height, attributes->get_hash().c_str());
  }

  virtual bool need_be_cached() { return true; };
  virtual Attributes* get_attributes() const = 0;
  int get_width() const { return width_; }
  int get_height() const { return height_; }

protected:
  int width_ = 0;
  int height_ = 0;
};

#endif // IMAGE_PROC_CACHEABLE_H
