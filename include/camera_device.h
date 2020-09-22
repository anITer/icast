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

#ifndef CAMERA_DEVICE_H
#define CAMERA_DEVICE_H

#include "capture_interface.h"
#include "v4l2.h"

class CameraDevice : public ICaptureDevice
{
public:
    CameraDevice();
    ~CameraDevice();

    const std::vector<DeviceInfo>& enum_devices() override;
    int bind_device(int index) override;
    int unbind_device() override;
    int start_device() override;
    int stop_device() override;
    int grab_frame(unsigned char* &buffer) override;
    int set_fps(int fps);

private:
    PixelFormat get_pixel_format(v4l2_format_t& format);

    int _width = 0;
    int _height = 0;
    v4l2_device_t* _v4l2_cam = nullptr;
};

#endif // CAMERA_DEVICE_H
