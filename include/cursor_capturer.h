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
#ifndef CURSOR_UTIL_H
#define CURSOR_UTIL_H

#include "capture_interface.h"
#include <X11/X.h>
#include <X11/extensions/Xfixes.h>

class CursorCapturer : public ICaptureDevice
{
public:
  CursorCapturer();
  ~CursorCapturer();
  const std::vector<DeviceInfo> enum_devices() override;
  int bind_device(DeviceInfo dev) override;
  int unbind_device() override;
  int grab_frame(unsigned char* &buffer) override;
  int get_hot_spot(int &x, int &y);
private:
  XFixesCursorImage* cur_image_ = nullptr;
  Display* cur_display_ = nullptr;
  unsigned long last_cursor_state_ = 0;
};

#endif // CURSOR_UTIL_H
