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

#include "screen_capturer.h"
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

class WindowCapturer : public ScreenCapturer
{
public:
struct XCPoint {
    int _x;
    int _y;
};
struct XCWindow {
    size_t _handle;
    XCPoint _position;
    XCPoint _size;
    // Name will always be lower case. It is converted to lower case
    // internally by the library for comparisons
    char _name[128] = {0};
};
public:
    WindowCapturer();
    ~WindowCapturer() override;

    std::vector<DeviceInfo>& enum_devices() override;
    int bind_device(DeviceInfo info) override;
    int unbind_device() override;
    int grab_frame(unsigned char* &buffer) override;

private:
    std::vector<DeviceInfo> _dev_list;
    std::vector<XCWindow> _window_list;
    XCWindow _cur_window;
    Display* _cur_display = nullptr;
    XImage* _cur_image = nullptr;
    XShmSegmentInfo* _shm_info = nullptr;
};

#endif // WINDOW_CAPTURER_H
