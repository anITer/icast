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

static ScreenCapturer::XCScreen create_screen(int index, int id, int h, int w, int ox, int oy, const std::string &n, float scaling)
{
    ScreenCapturer::XCScreen ret = {};
    ret._index = index;

    ret._id = id;
    assert(n.size() + 1 < sizeof(ret._name));
    memcpy(ret._name, n.c_str(), n.size() + 1);
    ret._original_offset_x = ret._offset_x = ox;
    ret._original_offset_y = ret._offset_y = oy;
    ret._original_width = ret._width = w;
    ret._original_height = ret._height = h;
    ret._scaling = scaling;
    return ret;
}

ScreenCapturer::ScreenCapturer() : _screen_list()
{

}

ScreenCapturer::~ScreenCapturer()
{
    stop_device();
    unbind_device();

    _screen_list.clear();
}

const std::vector<DeviceInfo>& ScreenCapturer::enum_devices()
{
    clear_devices();
    if (_screen_list.size()) {
        _screen_list.clear();
    }

    Display* display = XOpenDisplay(NULL);
    if(!display) {
        return _dev_list;
    }
    int nmonitors = 0;
    XineramaScreenInfo* screen = XineramaQueryScreens(display, &nmonitors);
    if(!screen) {
        XCloseDisplay(display);
        return _dev_list;
    }

    _screen_list.reserve(nmonitors);
    _dev_list.reserve(nmonitors);
    // TODO:: support multi-monitors, add tag in XCScreen struct
    for(auto i = 0; i < nmonitors; i++) {
        auto name = SCREEN_PREFIX + std::to_string(i);
        _screen_list.push_back(create_screen(i, screen[i].screen_number, screen[i].height,
                                            screen[i].width, screen[i].x_org, screen[i].y_org, name, 1.0f));
        _dev_list.push_back({ PIXEL_FORMAT_RGBA, 0, 0, screen[i].width, screen[i].height, name, i, &_screen_list[i] });
    }
    XFree(screen);
    XCloseDisplay(display);
    return _dev_list;
}

int ScreenCapturer::bind_device(int index)
{
    unbind_device();

    _cur_dev_index = index;
    DeviceInfo& info = _dev_list[index];
    XCScreen& screen = *((XCScreen*) info._ext_data);
    _cur_display = XOpenDisplay(NULL);
    if(!_cur_display) {
        return -1;
    }
    int scr = XDefaultScreen(_cur_display);
    _shm_info = new XShmSegmentInfo();

    _cur_image = XShmCreateImage(_cur_display, DefaultVisual(_cur_display, scr),
                                DefaultDepth(_cur_display, scr), ZPixmap, NULL,
                                _shm_info, screen._width, screen._height);
    _shm_info->shmid = shmget(IPC_PRIVATE, _cur_image->bytes_per_line * _cur_image->height, IPC_CREAT | 0777);
    _shm_info->readOnly = false;
    _shm_info->shmaddr = _cur_image->data = (char*) shmat(_shm_info->shmid, 0, 0);

    XShmAttach(_cur_display, _shm_info);

    return 0;
}

int ScreenCapturer::unbind_device()
{
    if(_shm_info) {
        shmdt(_shm_info->shmaddr);
        shmctl(_shm_info->shmid, IPC_RMID, 0);
        XShmDetach(_cur_display, _shm_info);
        _shm_info = nullptr;
    }
    if(_cur_image) {
        XDestroyImage(_cur_image);
        _cur_image = nullptr;
    }
    if(_cur_display) {
        XCloseDisplay(_cur_display);
        _cur_display = 0;
    }
    _cur_dev_index = -1;
    return 0;
}

int ScreenCapturer::start_device()
{
    return 0;
}

int ScreenCapturer::stop_device()
{
    return 0;
}

int ScreenCapturer::grab_frame(unsigned char *&buffer)
{
    XCScreen& screen = _screen_list[_cur_dev_index];
    if(!XShmGetImage(_cur_display, RootWindow(_cur_display, DefaultScreen(_cur_display)),
                     _cur_image, screen._offset_x, screen._offset_y, AllPlanes)) {
        if (_update_event_callback) {
            _update_event_callback->on_device_updated();
        }
        return -1;
    }
    buffer = (unsigned char*) _cur_image->data;
    return screen._width * screen._height * 4;
}
