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
#ifndef SCREEN_CAPTURER_H
#define SCREEN_CAPTURER_H

#include "capture_interface.h"
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

class ScreenCapturer : public ICaptureDevice
{
public:
struct XCScreen {
    int _id = INT32_MAX;
    int _index = INT32_MAX;
    int _adapter = INT32_MAX;
    int _width = 0;
    int _height = 0;
    int _original_width = 0;
    int _original_height = 0;
    // Offsets are the number of pixels that a monitor can be from the origin.
    // For example, users can shuffle their monitors around so this affects their offset.
    int _offset_x = 0;
    int _offset_y = 0;
    int _original_offset_x = 0;
    int _original_offset_y = 0;
    char _name[128] = {0};
    float _scaling = 1.0f;
};

public:
    ScreenCapturer();
    virtual ~ScreenCapturer();

    virtual std::vector<DeviceInfo>& enum_devices() override;
    int bind_device(DeviceInfo info) override;
    int unbind_device() override;
    int start_device() override;
    int stop_device() override;
    int grab_frame(unsigned char* &buffer) override;

protected:
    std::vector<XCScreen> _screen_list;

private:
    std::vector<DeviceInfo> _dev_list;
    XCScreen _cur_screen;
    Display* _cur_display = nullptr;
    XImage* _cur_image = nullptr;
    XShmSegmentInfo* _shm_info = nullptr;
};

#endif // SCREEN_CAPTURER_H
