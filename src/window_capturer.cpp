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
#include <algorithm>
#include <X11/Xutil.h>
#include <sys/shm.h>
#include <X11/extensions/Xinerama.h>

const std::string SCREEN_PREFIX = "Display_";

static std::vector<std::string> property_to_string(Display* dpy, const XTextProperty& prop)
{
    char **list;
    int n_strings = 0;
    std::vector<std::string> result = std::vector<std::string>{};
    int status = XmbTextPropertyToTextList(dpy, &prop, &list, &n_strings);

    if (status < Success || n_strings == 0 || *list == nullptr) {
        return result;
    }

    for (int i = 0; i < n_strings; ++i) {
        result.emplace_back(list[i]);
    }

    XFreeStringList(list);
    return result;
}

static WindowCapturer::XCWindow create_window(Display* display, XID& window)
{
    using namespace std::string_literals;

    XTextProperty property;
    XGetWMName(display, window, &property);
    std::vector<std::string> candidates = property_to_string(display, property);
    WindowCapturer::XCWindow w = {};
    w._handle = reinterpret_cast<size_t>(window);

    XWindowAttributes wndattr;
    XGetWindowAttributes(display, window, &wndattr);

    w._position = WindowCapturer::XCPoint{ wndattr.x, wndattr.y };
    w._size = WindowCapturer::XCPoint{ wndattr.width, wndattr.height };

    auto name = candidates.empty() ? ""s : std::move(candidates.front());
    std::transform(name.begin(), name.end(), std::begin(w._name), ::tolower);
    return w;
}

WindowCapturer::WindowCapturer() : _dev_list(), _window_list()
{

}

WindowCapturer::~WindowCapturer()
{
    stop_device();
    unbind_device();

    _dev_list.clear();
    _window_list.clear();
}

std::vector<DeviceInfo>& WindowCapturer::enum_devices()
{
    if (_dev_list.size() > 0) {
        _dev_list.clear();
        _window_list.clear();
    }

    // TODO:: using detected display??
    ScreenCapturer::enum_devices();
    Display* display = XOpenDisplay(NULL);
    if(!display) {
        return _dev_list;
    }
    Atom atom = XInternAtom(display, "_NET_CLIENT_LIST", true);
    Atom actual_type;
    int format;
    unsigned long num_items, bytes_after;
    unsigned char* data = 0;
    int status = XGetWindowProperty(display, XDefaultRootWindow(display), atom,
                                    0L, (~0L), false, AnyPropertyType, &actual_type,
                                    &format, &num_items, &bytes_after, &data);
    if(status >= Success && num_items) {
        _window_list.reserve(num_items);
        _dev_list.reserve(num_items);
        auto array = (XID*)data;
        for(unsigned long i = 0; i < num_items; i++) {
            auto w = array[i];
            _window_list.push_back(create_window(display, w));
            _dev_list.push_back({ PIXEL_FORMAT_RGBA, _window_list[i]._size._x, _window_list[i]._size._y,
                                 _window_list[i]._name, (int) i, &_window_list[i] });
        }
        XFree(data);
    }
    XCloseDisplay(display);
    return _dev_list;
}

int WindowCapturer::bind_device(DeviceInfo info)
{
    unbind_device();

    _cur_window = *((XCWindow*) info._ext_data);
    // TODO:: using detected display??
    _cur_display = XOpenDisplay(NULL);
    if(!_cur_display) {
        return -1;
    }
    int scr = XDefaultScreen(_cur_display);
    _shm_info = new XShmSegmentInfo();
    _cur_image = XShmCreateImage(_cur_display, DefaultVisual(_cur_display, scr),
                                DefaultDepth(_cur_display, scr), ZPixmap, NULL,
                                _shm_info, _cur_window._size._x, _cur_window._size._y);
    _shm_info->shmid = shmget(IPC_PRIVATE, _cur_image->bytes_per_line * _cur_image->height, IPC_CREAT | 0777);
    _shm_info->readOnly = false;
    _shm_info->shmaddr = _cur_image->data = (char*) shmat(_shm_info->shmid, 0, 0);

    XShmAttach(_cur_display, _shm_info);

    return 0;
}

int WindowCapturer::unbind_device()
{
    _cur_window = {};
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
    return 0;
}

int WindowCapturer::grab_frame(unsigned char *&buffer)
{
    XWindowAttributes wndattr;
    if(XGetWindowAttributes(_cur_display, _cur_window._handle, &wndattr) == 0){
        if (_update_event_callback) {
            _update_event_callback->on_device_updated();
        }
        return -1; // window might not be valid any more
    }
    if(wndattr.width != _cur_window._size._x || wndattr.height != _cur_window._size._y){
        if (_update_event_callback) {
            _update_event_callback->on_device_updated();
        }
        return -2; // window size changed. This will rebuild everything
    }

    if(!XShmGetImage(_cur_display,
                     _cur_window._handle,
                     _cur_image,
                     0,
                     0,
                     AllPlanes)) {
        if (_update_event_callback) {
            _update_event_callback->on_device_updated();
        }
        return -3;
    }
    buffer = (unsigned char*) _cur_image->data;
    return _cur_window._size._x * _cur_window._size._y * 4;
}
