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
    PixelFormat _format;
    int _width = 0;
    int _height = 0;
    std::string _name;
    int _dev_id;
    void* _ext_data;
    u_int8_t* _thumbnail = nullptr;
};

class OnDeviceUpdateCallback
{
public:
    virtual int on_device_updated() = 0;
};

class ICaptureDevice
{
public:
    virtual std::vector<DeviceInfo>& enum_devices() = 0;
    virtual int bind_device(DeviceInfo info) = 0;
    virtual int unbind_device() = 0;
    virtual int start_device() = 0;
    virtual int stop_device() = 0;
    virtual int grab_frame(unsigned char* &buffer) = 0;
    virtual void set_update_callback(OnDeviceUpdateCallback* callback)
    { _update_event_callback = callback; }

    virtual void remove_update_callback()
    { _update_event_callback = nullptr; }

protected:
    OnDeviceUpdateCallback* _update_event_callback = nullptr;
};

#endif // CAPTURE_DEVICE_H
