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
#ifndef COMPOSITE_CAPTURER_H
#define COMPOSITE_CAPTURER_H

#include "cursor_capturer.h"

class CompositeCapturer : public ICaptureDevice
{
public:
  CompositeCapturer(ICaptureDevice* window_cap);
  ~CompositeCapturer();

  const std::vector<DeviceInfo> enum_devices() override;
  int bind_device(DeviceInfo dev) override;
  int unbind_device() override;
  DeviceInfo& get_cur_device() override;
  int grab_frame(unsigned char* &buffer) override;
  void set_enable_cursor(bool is_enable) { is_cursor_enabled_ = is_enable; }

private:
  ICaptureDevice* window_cap_device_ = nullptr;
  CursorCapturer* cursor_cap_device_ = nullptr;
  bool      is_cursor_enabled_ = true;
};

#endif // COMPOSITE_CAPTURER_H
