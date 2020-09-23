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
#include "screen_capturer.h"
#include <assert.h>
#include <string.h>
#include <X11/Xutil.h>
#include <sys/shm.h>
#include <X11/extensions/Xinerama.h>

const std::string SCREEN_PREFIX = "Display_";

ScreenCapturer::ScreenCapturer()
{

}

ScreenCapturer::~ScreenCapturer()
{

}

const std::vector<DeviceInfo> ScreenCapturer::enum_devices()
{
  std::vector<DeviceInfo> dev_list;
  Display* display = XOpenDisplay(NULL);
  if(!display) {
    return dev_list;
  }
  int nmonitors = 0;
  XineramaScreenInfo* screen = XineramaQueryScreens(display, &nmonitors);
  if(!screen) {
    XCloseDisplay(display);
    return dev_list;
  }

  dev_list.reserve(nmonitors);
  for(auto i = 0; i < nmonitors; i++) {
    auto name = SCREEN_PREFIX + std::to_string(i);
    DeviceInfo dev;
    dev.format_ = PIXEL_FORMAT_RGBA;
    dev.pos_x_ = (int)screen[i].x_org;
    dev.pos_y_ = (int)screen[i].y_org;
    dev.width_ = (int)screen[i].width;
    dev.height_ = (int)screen[i].height;
    dev.name_ = name;
    dev.dev_id_ = (int)screen[i].screen_number;
    dev.ext_data_ = nullptr;
    dev_list.push_back(dev);
  }
  XFree(screen);
  XCloseDisplay(display);
  return dev_list;
}

int ScreenCapturer::bind_device(DeviceInfo& dev)
{
  unbind_device();

  cur_display_ = XOpenDisplay(NULL);
  if(!cur_display_) {
    return -1;
  }
  cur_dev_ = dev;

  int scr_id = cur_dev_.dev_id_ ? cur_dev_.dev_id_ : XDefaultScreen(cur_display_);
  Screen* screen = XScreenOfDisplay(cur_display_, scr_id);
  cur_dev_.width_ = XWidthOfScreen(screen);
  cur_dev_.height_ = XHeightOfScreen(screen);
  
  shm_info_ = new XShmSegmentInfo();
  cur_image_ = XShmCreateImage(cur_display_, screen->root_visual, screen->root_depth,
                ZPixmap, NULL, shm_info_, cur_dev_.width_, cur_dev_.height_);
  shm_info_->shmid = shmget(IPC_PRIVATE, cur_image_->bytes_per_line * cur_image_->height, IPC_CREAT | 0777);
  shm_info_->readOnly = false;
  shm_info_->shmaddr = cur_image_->data = (char *)shmat(shm_info_->shmid, 0, 0);
  XShmAttach(cur_display_, shm_info_);

  return 0;
}

int ScreenCapturer::unbind_device()
{
  if(shm_info_) {
    shmdt(shm_info_->shmaddr);
    shmctl(shm_info_->shmid, IPC_RMID, 0);
    XShmDetach(cur_display_, shm_info_);
    delete shm_info_;
    shm_info_ = nullptr;
  }
  if(cur_image_) {
    XDestroyImage(cur_image_);
    cur_image_ = nullptr;
  }
  if(cur_display_) {
    XCloseDisplay(cur_display_);
    cur_display_ = 0;
  }
  return 0;
}

int ScreenCapturer::grab_frame(unsigned char *&buffer)
{
  if (!cur_display_) return 0;
  if(!XShmGetImage(cur_display_, RootWindow(cur_display_, DefaultScreen(cur_display_)),
           cur_image_, cur_dev_.pos_x_, cur_dev_.pos_y_, AllPlanes)) {
    return -1;
  }
  // TODO:: check if content was changed
  buffer = (unsigned char*) cur_image_->data;
  return cur_dev_.width_ * cur_dev_.height_ * sizeof(int);
}
