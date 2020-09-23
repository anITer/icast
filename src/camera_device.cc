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
#include "camera_device.h"
#include <algorithm>

const std::string DEVICE_PREFIX = "/dev/video";

CameraDevice::CameraDevice()
{

}

CameraDevice::~CameraDevice()
{

}

const std::vector<DeviceInfo> CameraDevice::enum_devices()
{
  std::vector<DeviceInfo> dev_list;
  v4l2_device_t* tmp_cam = nullptr;
  for (int i = 0; i < 8; i++) {
    std::string tmp_name = DEVICE_PREFIX + std::to_string(i);
    tmp_cam = v4l2_create_device(tmp_name.c_str());
    if (!tmp_cam) continue;
    if (v4l2_open_device(tmp_cam) != V4L2_STATUS_OK) {
      v4l2_destroy_device(tmp_cam);
      continue;
    }
    v4l2_get_format(tmp_cam, &tmp_cam->format_);
    // v4l2_enum_format(tmp_cam, &tmp_cam->format_);

    DeviceInfo dev;
    dev.format_ = get_pixel_format(tmp_cam->format_);
    dev.pos_x_ = 0;
    dev.pos_y_ = 0;
    dev.width_ = (int) tmp_cam->format_.width_[0];
    dev.height_ = (int) tmp_cam->format_.height_[0];
    dev.name_ = tmp_name;
    dev.dev_id_ = i;
    dev.ext_data_ = (unsigned char *)tmp_cam;
    dev_list.push_back(dev);
    v4l2_close_device(tmp_cam);
    v4l2_destroy_device(tmp_cam);
  }
  return dev_list;
}

int CameraDevice::bind_device(DeviceInfo& dev)
{
  unbind_device();

  v4l2_cam_ = v4l2_create_device(dev.name_.c_str());
  v4l2_get_format(v4l2_cam_, &v4l2_cam_->format_);
  _width = dev.width_;
  _height = dev.height_;
  cur_dev_ = dev;
  return 0;
}

int CameraDevice::unbind_device()
{
  if (!v4l2_cam_) return 0;
  v4l2_destroy_device(v4l2_cam_);
  v4l2_cam_ = nullptr;
  return 0;
}

int CameraDevice::start_device()
{
  if (!v4l2_cam_) return -1;
  if (v4l2_open_device(v4l2_cam_) != V4L2_STATUS_OK) {
    return -1;
  }
  if (v4l2_start_capture(v4l2_cam_) != V4L2_STATUS_OK) {
    v4l2_close_device(v4l2_cam_);
    return -1;
  }
  return 0;
}

int CameraDevice::stop_device()
{
  if (!v4l2_cam_) return -1;
  if (v4l2_stop_capture(v4l2_cam_) != V4L2_STATUS_OK) {
    return -1;
  }
  if (v4l2_close_device(v4l2_cam_) != V4L2_STATUS_OK) {
    return -1;
  }
  return 0;
}

/**
 * @brief CameraDevice::grab_frame
 * @param buffer
 * @return the length of captured data
 */
int CameraDevice::grab_frame(unsigned char *&buffer)
{
  if (!v4l2_cam_) {
    return 0;
  }
  if (v4l2_grab_frame(v4l2_cam_) != V4L2_STATUS_OK) {
    return -1;
  }
  buffer = v4l2_cam_->data_;
  return _width * _height * 2;
}

int CameraDevice::set_fps(int fps)
{
  struct v4l2_streamparm param;
  param.type = V4L2_BUF_TYPE_META_CAPTURE;
  param.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;
  param.parm.capture.timeperframe.numerator = 1;
  param.parm.capture.timeperframe.denominator = fps;
  return v4l2_set_param(v4l2_cam_, &param);
}

PixelFormat CameraDevice::get_pixel_format(v4l2_format_t& format)
{
  switch(format.pixel_format_) {
  case V4L2_PIX_FMT_RGBA32:
  case V4L2_PIX_FMT_RGB32:
    return PIXEL_FORMAT_RGBA;
  case V4L2_PIX_FMT_RGB24:
    return PIXEL_FORMAT_RGB;
  case V4L2_PIX_FMT_YUV420:
    return PIXEL_FORMAT_I420;
  case V4L2_PIX_FMT_YUYV:
    return PIXEL_FORMAT_YUYV;
  }
  return PIXEL_FORMAT_YUYV;
}
