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
#include "cursor_capturer.h"
#include <cassert>
#include <cstring>

CursorCapturer::CursorCapturer() : _cur_window()
{

}

CursorCapturer::~CursorCapturer()
{
    stop_device();
    unbind_device();
}

const std::vector<DeviceInfo>& CursorCapturer::enum_devices()
{
    clear_devices();

    Display* display = XOpenDisplay(NULL);
    if (!display) {
        return _dev_list;
    }
    XFixesCursorImage* image = XFixesGetCursorImage(display);
    if (!image) {
        return _dev_list;
    }
    _dev_list.push_back({ PIXEL_FORMAT_RGBA, image->x, image->y, image->width, image->height, image->name, 0, &_cur_image });
    XFree(image);
    XCloseDisplay(display);
    return _dev_list;
}

int CursorCapturer::bind_device(int index)
{
    assert(index == 0);
    unbind_device();

    _cur_dev_index = index;
    _cur_display = XOpenDisplay(NULL);
    if (!_cur_display) {
        return -1;
    }
    _cur_window = DefaultRootWindow(_cur_display);
    if (!_cur_window) {
        return -1;
    }
    _cur_image = XFixesGetCursorImage(_cur_display);
    if (!_cur_image) {
        return -1;
    }

    _dev_list[_cur_dev_index]._thumbnail = (uint8_t*) malloc(_cur_image->width * _cur_image->height * sizeof(int));
    _dev_list[_cur_dev_index]._width = _cur_image->width;
    _dev_list[_cur_dev_index]._height = _cur_image->height;
    XFree(_cur_image);

    return 0;
}

int CursorCapturer::unbind_device()
{
    if(_cur_image) {
        free(_dev_list[_cur_dev_index]._thumbnail);
        _dev_list[_cur_dev_index]._thumbnail = nullptr;
        _cur_image = nullptr;
    }
    if(_cur_display) {
        XCloseDisplay(_cur_display);
        _cur_display = 0;
    }
    _cur_dev_index = -1;
    return 0;
}

int CursorCapturer::start_device()
{
    return 0;
}

int CursorCapturer::stop_device()
{
    return 0;
}

int CursorCapturer::grab_frame(unsigned char *&buffer)
{
    _cur_image = XFixesGetCursorImage(_cur_display);
    if (!_cur_image) return -1;

    _dev_list[_cur_dev_index]._pos_x = _cur_image->x;
    _dev_list[_cur_dev_index]._pos_y = _cur_image->y;

    // if the pixelstride is 64 bits.. scale down to 32bits
    if (last_state != _cur_image->cursor_serial) {
        int* pixels = (int *) _dev_list[_cur_dev_index]._thumbnail;
        if (sizeof(_cur_image->pixels[0]) == 8) {
            long* original = (long *) _cur_image->pixels;
            for (int i = 0; i < _cur_image->width * _cur_image->height; ++i) {
                pixels[i] = (int) (original[i]);
            }
        } else {
            memcpy(pixels, _cur_image->pixels, _cur_image->width * _cur_image->height * 4);
        }
    }
    last_state = _cur_image->cursor_serial;

    buffer = (unsigned char*) (_dev_list[_cur_dev_index]._thumbnail);
    XFree(_cur_image);
    return _cur_image->width * _cur_image->height * 4;
}

