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
#include "window_capturer.h"
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include <algorithm>
#include <X11/Xutil.h>
#include <sys/shm.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xcomposite.h>

const std::string SCREEN_PREFIX = "Display_";

static DeviceInfo create_device(Display* display, XID& window)
{
  XTextProperty property;
  XGetWMName(display, window, &property);
  int cnt = 0;
  char** list = nullptr;
  Xutf8TextPropertyToTextList(display, &property, &list, &cnt);
  std::string name(cnt ? (char *)list[0] : "");
  DeviceInfo dev;
  dev.dev_id_ = window;
  dev.name_ = name;
  dev.format_ = PIXEL_FORMAT_RGBA;

  XWindowAttributes wndattr;
  XGetWindowAttributes(display, window, &wndattr);
  dev.pos_x_ = wndattr.x;
  dev.pos_y_ = wndattr.y;
  dev.width_ = wndattr.width;
  dev.height_ = wndattr.height;
  dev.ext_data_ = nullptr;

  return dev;
}

WindowCapturer::WindowCapturer()
{
  cur_dev_.dev_id_ = 0;
}

WindowCapturer::~WindowCapturer()
{

}

const std::vector<DeviceInfo> WindowCapturer::enum_devices()
{
  std::vector<DeviceInfo> dev_list;
  Display* display = XOpenDisplay(NULL);
  if(!display) return dev_list;
  
  Atom atom = XInternAtom(display, "_NET_CLIENT_LIST", true);
  Atom actual_type;
  int format;
  unsigned long num_items, bytes_after;
  unsigned char* data = 0;
  int status = XGetWindowProperty(display, XDefaultRootWindow(display), atom,
                                  0L, (~0L), false, AnyPropertyType, &actual_type,
                                  &format, &num_items, &bytes_after, &data);
  if(status >= Success && num_items) {
    XID* array = (XID *)data;
    for(unsigned long i = 0; i < num_items; i++) {
      XID& w = array[i];
      const DeviceInfo& dev = create_device(display, w);
      if (dev.name_ == "") continue; // all window should have a name
      dev_list.push_back(dev);
    }
    XFree(data);
  }
  XCloseDisplay(display);
  return dev_list;
}

int WindowCapturer::bind_device(DeviceInfo dev)
{
  unbind_device();

  cur_display_ = XOpenDisplay(NULL);
  if(!cur_display_) {
    return -1;
  }
  cur_dev_ = dev;
  cur_dev_.format_ = PIXEL_FORMAT_RGBA;

  Window tmp_wnd;
  int x, y;
  unsigned int width, height, border, depth = 0;
  XGetGeometry(cur_display_, (Window)(cur_dev_.dev_id_),
              &tmp_wnd, &x, &y, &width, &height, &border, &depth);
  cur_dev_.width_ = width;
  cur_dev_.height_ = height;

  int scr = XDefaultScreen(cur_display_);
  shm_info_ = new XShmSegmentInfo();
  cur_image_ = XShmCreateImage(cur_display_, DefaultVisual(cur_display_, scr),
                                DefaultDepth(cur_display_, scr), ZPixmap, NULL,
                                shm_info_, cur_dev_.width_, cur_dev_.height_);
  shm_info_->shmid = shmget(IPC_PRIVATE, cur_image_->bytes_per_line * cur_image_->height, IPC_CREAT | 0777);
  shm_info_->readOnly = false;
  shm_info_->shmaddr = cur_image_->data = (char*) shmat(shm_info_->shmid, 0, 0);
  XShmAttach(cur_display_, shm_info_);

  // force preserve an off-screen storage for window even if it's in the background
  XCompositeRedirectWindow(cur_display_, cur_dev_.dev_id_, CompositeRedirectAutomatic);

  if (!XDamageQueryExtension(cur_display_, &damage_event_base_, &damage_error_base_)) {
    damage_event_base_ = 0;
    return 0;
  }
  damage_handle_ = XDamageCreate(cur_display_, cur_dev_.dev_id_, XDamageReportRawRectangles);

  return 0;
}

int WindowCapturer::unbind_device()
{
  if (damage_handle_) {
    XDamageDestroy(cur_display_, damage_handle_);
    damage_handle_ = 0;
  }
  if (cur_dev_.dev_id_) {
    XCompositeUnredirectWindow(cur_display_, cur_dev_.dev_id_, CompositeRedirectAutomatic);
  }
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
  cur_dev_.dev_id_ = 0;
  return 0;
}

bool WindowCapturer::is_window_redrawed()
{
  if (!damage_event_base_) return true;
  int events_to_process = XPending(cur_display_);
  XEvent e;
  for (int i = 0; i < events_to_process; i++) {
    XNextEvent(cur_display_, &e);
    if (e.type == damage_event_base_ + XDamageNotify) {
      if (((XDamageNotifyEvent *)&e)->damage == damage_handle_) {
        struct timeval tp;
        gettimeofday(&tp, NULL);
//        fprintf(stderr, "[time:%lu] is_window_redrawed: true\n", tp.tv_sec * 1000 + tp.tv_usec / 1000);
        return true;
      }
    }
  }
//  fprintf(stderr, "is_window_redrawed: false\n");
  return false;
}

int WindowCapturer::resize_window_internal(int x, int y, int width, int height)
{
  bool is_size_changed = width != cur_dev_.width_|| height != cur_dev_.height_;
  cur_dev_.pos_x_ = x;
  cur_dev_.pos_y_ = y;
  cur_dev_.width_ = width;
  cur_dev_.height_ = height;

  if (is_size_changed) {
    // avoid calling extended class function
    // this should be internal behavior
    WindowCapturer::bind_device(cur_dev_);
  }

  return is_size_changed;
}

int WindowCapturer::grab_frame(unsigned char *&buffer)
{
  if (!cur_display_) return 0;
  Window tmp_wnd;
  int x, y, pos_x, pos_y = 0;
  unsigned int width, height, border, depth = 0;
  do {
    if(XGetGeometry(cur_display_, (Window)(cur_dev_.dev_id_),
                    &tmp_wnd, &x, &y, &width, &height, &border, &depth) == 0) {
      return -1; // window might not be valid any more
    }
    XTranslateCoordinates(cur_display_, (Window)(cur_dev_.dev_id_),
                        XDefaultRootWindow(cur_display_), x, y, &pos_x, &pos_y, &tmp_wnd);
    // if window size changed, rebuild memory mapping staffs
    resize_window_internal(pos_x, pos_y, width, height);
    if(!XShmGetImage(cur_display_, (Window)(cur_dev_.dev_id_), cur_image_, 0, 0, AllPlanes)) {
      // TODO:: log error fetch buffer failed
      return -1;
    }
    break;
  } while (true);
  buffer = (unsigned char *)cur_image_->data;
  return is_window_redrawed() ? cur_dev_.width_ * cur_dev_.height_ * sizeof(int) : 0;
}
