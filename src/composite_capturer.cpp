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
#include "composite_capturer.h"
#include <cassert>
#include <iostream>

CompositeCapturer::CompositeCapturer(ICaptureDevice* window_cap) : _window_cap_device(window_cap)
{
    assert(window_cap != nullptr);
    _cursor_cap_device = new CursorCapturer();
}

CompositeCapturer::~CompositeCapturer()
{
    _window_cap_device = nullptr;
    if (_cursor_cap_device) {
        delete _cursor_cap_device;
    }
    _cursor_cap_device = nullptr;
}

const std::vector<DeviceInfo>& CompositeCapturer::enum_devices()
{
    if (_cursor_cap_device) _cursor_cap_device->enum_devices();
    return _window_cap_device->enum_devices();
}

DeviceInfo* CompositeCapturer::get_cur_device()
{
    return _window_cap_device->get_cur_device();
}

int CompositeCapturer::bind_device(int index)
{
    if (_cursor_cap_device) _cursor_cap_device->bind_device(0);
    return _window_cap_device->bind_device(index);
}

int CompositeCapturer::unbind_device()
{
    if (_cursor_cap_device) _cursor_cap_device->unbind_device();
    return _window_cap_device->unbind_device();
}

int CompositeCapturer::start_device()
{
    if (_cursor_cap_device) _cursor_cap_device->start_device();
    return _window_cap_device->start_device();
}

int CompositeCapturer::stop_device()
{
    if (_cursor_cap_device) _cursor_cap_device->stop_device();
    return _window_cap_device->stop_device();
}

int CompositeCapturer::grab_frame(unsigned char *&buffer)
{
    int len = _window_cap_device->grab_frame(buffer);
    if (len < 0) return len;
    unsigned char* cursor_buffer = nullptr;
    _cursor_cap_device->grab_frame(cursor_buffer);

    // blend cursor icon with window
    int wnd_pos_x = _window_cap_device->get_cur_device()->_pos_x;
    int wnd_pos_y = _window_cap_device->get_cur_device()->_pos_y;
    int wnd_width = _window_cap_device->get_cur_device()->_width;
    int wnd_height = _window_cap_device->get_cur_device()->_height;
    int curs_pos_x = _cursor_cap_device->get_cur_device()->_pos_x;
    int curs_pos_y = _cursor_cap_device->get_cur_device()->_pos_y;
    int curs_width = _cursor_cap_device->get_cur_device()->_width;
    int curs_height = _cursor_cap_device->get_cur_device()->_height;

    int offset_x = curs_pos_x - wnd_pos_x;
    int offset_y = curs_pos_y - wnd_pos_y;
    int start_x = std::max(0, -offset_x);
    int start_y = std::max(0, -offset_y);
    int end_x = std::min(curs_width, wnd_width - offset_x);
    int end_y = std::min(curs_height, wnd_height - offset_y);
    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            int& wnd_pixel = ((int*) buffer)[(y + offset_y) * wnd_width + (x + offset_x)];
            int& curs_pixel = ((int*) cursor_buffer)[y * curs_width + x];
            uint8_t curs_alpha = (uint8_t)(curs_pixel >> 24);
            if (curs_alpha == 0) {
                continue;
            } else if (curs_alpha == 255) {
                wnd_pixel = curs_pixel;
            } else { // do blending
                /* pixel values from XFixesGetCursorImage come premultiplied by alpha */
                uint8_t r = (uint8_t) (curs_pixel >> 0) + ((uint8_t) (wnd_pixel >> 0) * (255 - curs_alpha) + 255/2) / 255;
                uint8_t g = (uint8_t) (curs_pixel >> 8) + ((uint8_t) (wnd_pixel >> 8) * (255 - curs_alpha) + 255/2) / 255;
                uint8_t b = (uint8_t) (curs_pixel >> 16) + ((uint8_t) (wnd_pixel >> 16) * (255 - curs_alpha) + 255/2) / 255;
                wnd_pixel = (int) r | ((int) g << 8) | ((int) b << 16) | ((int) 0xff << 24);
            }
        }
    }
    return len;
}

void CompositeCapturer::clear_devices()
{
    if (_cursor_cap_device) _cursor_cap_device->clear_devices();
    _window_cap_device->clear_devices();
}

void CompositeCapturer::set_update_callback(OnDeviceUpdateCallback *callback)
{
    if (_cursor_cap_device) _cursor_cap_device->set_update_callback(callback);
    _window_cap_device->set_update_callback(callback);
}

void CompositeCapturer::remove_update_callback()
{
    if (_cursor_cap_device) _cursor_cap_device->remove_update_callback();
    _cursor_cap_device->remove_update_callback();
}
