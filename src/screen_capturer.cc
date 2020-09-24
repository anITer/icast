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
#include <string.h>
#include <X11/extensions/Xinerama.h>

const std::string SCREEN_PREFIX = "Display_";

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

int ScreenCapturer::bind_device(DeviceInfo dev)
{
  Display* display = XOpenDisplay(NULL);
  if(!display) return -1;

  int scr_id = dev.dev_id_ ? dev.dev_id_ : XDefaultScreen(display);
  dev.dev_id_ = RootWindow(display, scr_id);
  XCloseDisplay(display);

  return WindowCapturer::bind_device(dev);
}
