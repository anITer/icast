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
#ifndef WINDOW_CAPTURER_H
#define WINDOW_CAPTURER_H

#include "capture_interface.h"
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xdamage.h>

class WindowCapturer : public ICaptureDevice
{
public:
  WindowCapturer();
  ~WindowCapturer() override;

  const std::vector<DeviceInfo> enum_devices() override;
  int bind_device(DeviceInfo dev) override;
  int unbind_device() override;
  int grab_frame(unsigned char* &buffer) override;

private:
  bool is_window_redrawed();
  int resize_window_internal(int x, int y, int width, int height);
  Display* cur_display_ = nullptr;
  XImage* cur_image_ = nullptr;
  XShmSegmentInfo* shm_info_ = nullptr;

  // adaptive fps related
  Damage damage_handle_ = 0;
  int damage_event_base_ = 0;
  int damage_error_base_ = 0;
  XserverRegion damage_region_ = 0;
};

#endif // WINDOW_CAPTURER_H
