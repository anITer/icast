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
#ifndef CAPTURE_DEVICE_H
#define CAPTURE_DEVICE_H

#include <vector>
#include <string>

enum PixelFormat {
  PIXEL_FORMAT_RGBA = 0,
  PIXEL_FORMAT_RGB,
  PIXEL_FORMAT_I420,
  PIXEL_FORMAT_NV21,
  PIXEL_FORMAT_YUYV
};

struct DeviceInfo {
  PixelFormat format_;
  int pos_x_ = 0;
  int pos_y_ = 0;
  int width_ = 0;
  int height_ = 0;
  unsigned long dev_id_;
  std::string name_;
  u_int8_t* ext_data_ = nullptr;
};

class ICaptureDevice
{
public:
  ICaptureDevice() { }
  virtual const std::vector<DeviceInfo> enum_devices() = 0;
  virtual int bind_device(DeviceInfo dev) = 0;
  virtual int unbind_device() { return 0; }
  virtual int start_device() { return 0; }
  virtual int stop_device() { return 0; }
  /**
   * @brief grab_frame, get buffer of the updated frame
   * @param buffer, pointer will be set to the pixel buffer
   * @return value equals 0 stands for nothing changed (no need to render),
   *         value bigger than 0 means things changed (result is the length of updated buffer),
   *         value smaller than 0 means some exception occurred
   */
  virtual int grab_frame(unsigned char* &buffer) = 0;
  virtual DeviceInfo& get_cur_device() { return cur_dev_; }
  virtual ~ICaptureDevice() { unbind_device(); }

protected:
  DeviceInfo cur_dev_;
};

#endif // CAPTURE_DEVICE_H
