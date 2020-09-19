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
    // place holder
    _dev_list.push_back({ PIXEL_FORMAT_RGBA, 0, 0, 0, 0, "cursor", 0, &_cur_image });
    return _dev_list;
}

void CursorCapturer::set_parent_window(const XID& window)
{
    _cur_window = window;
}

int CursorCapturer::bind_device(int index)
{
    unbind_device();

    _cur_dev_index = index;
    _cur_display = XOpenDisplay(NULL);
    if (!_cur_display) {
        return -1;
    }
    if (!_cur_window) {
        _cur_window = DefaultRootWindow(_cur_display);
    }
    if (!_cur_window) {
        return -1;
    }
    _cur_image = XFixesGetCursorImage(_cur_display);
    if (!_cur_image) {
        return -1;
    }

    if (sizeof(_cur_image->pixels[0]) == 8) { // if the pixelstride is 64 bits.. scale down to 32bits
        int* pixels = (int *)_cur_image->pixels;
        for (int i = 0; i < _cur_image->width * _cur_image->height; ++i) {
            pixels[i] = pixels[i * 2];
        }
    }

    _dev_list[_cur_dev_index]._thumbnail = (uint8_t*) _cur_image->pixels;
    _dev_list[_cur_dev_index]._width = _cur_image->width;
    _dev_list[_cur_dev_index]._height = _cur_image->height;

    return 0;
}

int CursorCapturer::unbind_device()
{
    if(_cur_image) {
        _dev_list[_cur_dev_index]._thumbnail = nullptr;
        XFree(_cur_image);
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
    // Get the mouse cursor position
    int x, y = 0; // coordinate to parent window
    int root_x, root_y = 0; // coordinate to root window
    unsigned int mask = 0; // 0 for button_up, 256 for left_button_down, 1024 for right button_up
    XID child_win, root_win;
    XQueryPointer(_cur_display, _cur_window, &child_win, &root_win, &root_x, &root_y, &x, &y, &mask);
    _dev_list[_cur_dev_index]._pos_x = x;
    _dev_list[_cur_dev_index]._pos_y = y;
    fprintf(stderr, "current pos of cursor is [%d, %d | %d, %d | %u]\n", root_x, root_y, x, y, mask);

    XFree(_cur_image);
    // TODO:: just update area
    _cur_image = XFixesGetCursorImage(_cur_display);
    if (!_cur_image) {
        return -1;
    }
    if (sizeof(_cur_image->pixels[0]) == 8) { // if the pixelstride is 64 bits.. scale down to 32bits
        int* pixels = (int *)_cur_image->pixels;
        for (int i = 0; i < _cur_image->width * _cur_image->height; ++i) {
            pixels[i] = pixels[i * 2];
        }
    }

    buffer = (unsigned char*) _cur_image->pixels;
    return _cur_image->width * _cur_image->height * 4;
}

