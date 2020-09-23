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
#include "composite_capturer.h"
#include <cassert>
#include <iostream>

CompositeCapturer::CompositeCapturer(ICaptureDevice* window_cap) : window_cap_device_(window_cap)
{
  assert(window_cap != nullptr);
  cursor_cap_device_ = new CursorCapturer();
}

CompositeCapturer::~CompositeCapturer()
{
  window_cap_device_ = nullptr;
  if (cursor_cap_device_) {
    delete cursor_cap_device_;
  }
  cursor_cap_device_ = nullptr;
}

const std::vector<DeviceInfo> CompositeCapturer::enum_devices()
{
  if (cursor_cap_device_) cursor_cap_device_->enum_devices();
  return window_cap_device_->enum_devices();
}

int CompositeCapturer::bind_device(DeviceInfo& dev)
{
  if (cursor_cap_device_) {
    // todo: maybe use window (dev.dev_id_) of dev
    DeviceInfo tmp;
    cursor_cap_device_->bind_device(tmp);
  }
  return window_cap_device_->bind_device(dev);
}

int CompositeCapturer::unbind_device()
{
  if (cursor_cap_device_) cursor_cap_device_->unbind_device();
  return window_cap_device_->unbind_device();
}

DeviceInfo& CompositeCapturer::get_cur_device()
{
    return window_cap_device_->get_cur_device();
}

int CompositeCapturer::grab_frame(unsigned char *&buffer)
{
  int len = window_cap_device_->grab_frame(buffer);
  if (len < 0) return len;

  if (is_cursor_enabled_) {
    unsigned char* cursor_buffer = nullptr;
    int state = cursor_cap_device_->grab_frame(cursor_buffer);
    if (len == 0 && state == 0) return 0; // nothing changed

    // blend cursor icon with window
    int wnd_width = window_cap_device_->get_cur_device().width_;
    int wnd_height = window_cap_device_->get_cur_device().height_;
    int curs_width = cursor_cap_device_->get_cur_device().width_;
    int curs_height = cursor_cap_device_->get_cur_device().height_;
    int offset_x = cursor_cap_device_->get_cur_device().pos_x_ - window_cap_device_->get_cur_device().pos_x_;
    int offset_y = cursor_cap_device_->get_cur_device().pos_y_ - window_cap_device_->get_cur_device().pos_y_;
    int start_x = std::max(0, -offset_x);
    int start_y = std::max(0, -offset_y);
    int end_x = std::min(curs_width, wnd_width - offset_x);
    int end_y = std::min(curs_height, wnd_height - offset_y);
    for (int y = start_y; y < end_y; y++) {
      for (int x = start_x; x < end_x; x++) {
        int& wnd_pixel = ((int*) buffer)[(y + offset_y) * wnd_width + (x + offset_x)];
        int& curs_pixel = ((int*) cursor_buffer)[y * curs_width + x];
        uint8_t curs_alpha = (uint8_t)(curs_pixel >> 24);
        if (curs_alpha == 0) {
          continue;
        } else if (curs_alpha == 255) {
          wnd_pixel = curs_pixel;
        } else { // do blending
          /* pixel values from XFixesGetCursorImage come premultiplied by alpha */
          uint8_t r = (uint8_t) (curs_pixel >> 0) + ((uint8_t) (wnd_pixel >> 0) * (255 - curs_alpha) + 255/2) / 255;
          uint8_t g = (uint8_t) (curs_pixel >> 8) + ((uint8_t) (wnd_pixel >> 8) * (255 - curs_alpha) + 255/2) / 255;
          uint8_t b = (uint8_t) (curs_pixel >> 16) + ((uint8_t) (wnd_pixel >> 16) * (255 - curs_alpha) + 255/2) / 255;
          wnd_pixel = (int) r | ((int) g << 8) | ((int) b << 16) | ((int) 0xff << 24);
        }
      }
    }
  }
  return len;
}
