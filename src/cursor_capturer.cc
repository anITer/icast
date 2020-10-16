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
#include "cursor_capturer.h"
#include <cstring>

CursorCapturer::CursorCapturer()
{
  cur_dev_.dev_id_ = 0;
}

CursorCapturer::~CursorCapturer()
{

}

const std::vector<DeviceInfo> CursorCapturer::enum_devices()
{
  std::vector<DeviceInfo> dev_list;
  Display* display = XOpenDisplay(NULL);
  if (!display) {
    return dev_list;
  }
  XFixesCursorImage* image = XFixesGetCursorImage(display);
  if (!image) {
    return dev_list;
  }

  DeviceInfo dev;
  dev.format_ = PIXEL_FORMAT_RGBA;
  dev.pos_x_ = 0;
  dev.pos_y_ = 0;
  dev.width_ = (int)image->width;
  dev.height_ = (int)image->height;
  dev.name_ = "cursor";
  dev.dev_id_ = 0UL;
  dev.ext_data_ = nullptr;
  dev_list.push_back(dev);

  XFree(image);
  XCloseDisplay(display);
  return dev_list;
}

int CursorCapturer::bind_device(DeviceInfo dev)
{
  unbind_device();

  cur_display_ = XOpenDisplay(NULL);
  if (!cur_display_) {
    return -1;
  }
  cur_dev_ = dev; // only dev_id_ matters something

  if (!cur_dev_.dev_id_) {
    cur_dev_.dev_id_ = DefaultRootWindow(cur_display_);
  }
  if (!cur_dev_.dev_id_) {
    return -1;
  }

  cur_dev_.ext_data_ = (uint8_t *)malloc(grab_frame(cur_dev_.ext_data_));

  return 0;
}

int CursorCapturer::unbind_device()
{
  if(cur_image_) {
    free(cur_dev_.ext_data_);
    cur_dev_.ext_data_ = nullptr;
    cur_image_ = nullptr;
  }
  if(cur_display_) {
    XCloseDisplay(cur_display_);
    cur_display_ = nullptr;
  }
  cur_dev_.dev_id_ = 0;
  return 0;
}

int CursorCapturer::grab_frame(unsigned char *&buffer)
{
  if (!cur_display_) return 0;
  // Get the mouse cursor position
  cur_image_ = XFixesGetCursorImage(cur_display_);
  if (!cur_image_) return 0;

  bool is_pos_changed = cur_dev_.pos_x_ != cur_image_->x
                      || cur_dev_.pos_y_ != cur_image_->y
                      || cur_dev_.width_ != cur_image_->width
                      || cur_dev_.height_ != cur_image_->height;
  cur_dev_.pos_x_ = cur_image_->x - cur_image_->xhot;
  cur_dev_.pos_y_ = cur_image_->y - cur_image_->yhot;
  cur_dev_.width_ = cur_image_->width;
  cur_dev_.height_ = cur_image_->height;

  bool is_state_changed = false;
  int pixel_size = cur_image_->width * cur_image_->height;
  if (last_cursor_state_ != cur_image_->cursor_serial && cur_dev_.ext_data_) {
    int* pixels = (int *) cur_dev_.ext_data_;
    // if the pixelstride is 64 bits, scale down to 32bits
    if (sizeof(cur_image_->pixels[0]) == sizeof(long)) {
      long* original = (long *) cur_image_->pixels;
      for (int i = 0; i < pixel_size; ++i) {
        pixels[i] = (int)original[i];
      }
    } else {
      memcpy(pixels, cur_image_->pixels, pixel_size * sizeof(int));
    }
    is_state_changed = true;
  }

  last_cursor_state_ = cur_image_->cursor_serial;
  buffer = (unsigned char *)cur_dev_.ext_data_;
  XFree(cur_image_);
  return (is_pos_changed || is_state_changed) ? pixel_size * sizeof(int) : 0;
}

int CursorCapturer::get_hot_spot(int &x, int &y)
{
  if (!cur_image_) return -1;
  x = cur_image_->xhot;
  y = cur_image_->yhot;
  return 0;
}
