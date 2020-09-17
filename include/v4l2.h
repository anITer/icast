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
#ifndef V4L2_H
#define V4L2_H

#include <stdlib.h>
#include <linux/videodev2.h>

#ifdef __cplusplus
extern "C"{
#endif

#define V4L2_STATUS_OK 1
#define V4L2_STATUS_ERROR -1

typedef struct {
    void *_start;
    size_t _length;
    v4l2_memory _type = V4L2_MEMORY_MMAP;
} v4l2_buffer_t;

typedef struct {
    unsigned int _width[32];
    unsigned int _height[32];
    unsigned int _pixel_format;
    unsigned int _reslution_num;
} v4l2_format_t;

typedef struct {
    char _name[32];
    int _fd;
    unsigned int _num_buffers;
    v4l2_buffer_t* _buffers;
    v4l2_format_t _format;
    unsigned char* _data;
} v4l2_device_t;

v4l2_device_t* v4l2_create_device(const char* device_name);
void v4l2_destroy_device(v4l2_device_t *device);

int v4l2_open_device(v4l2_device_t *device);
int v4l2_close_device(v4l2_device_t *device);

size_t v4l2_get_buffer_size(v4l2_device_t* device);

int v4l2_set_format(v4l2_device_t* device, v4l2_format_t* format, int res_index);
int v4l2_get_format(v4l2_device_t* device, v4l2_format_t* format);
int v4l2_enum_format(v4l2_device_t* device, v4l2_format_t* format);

int v4l2_start_capture(v4l2_device_t *device);
int v4l2_stop_capture(v4l2_device_t *device);

int v4l2_grab_frame(v4l2_device_t *device);

void v4l2_copy_frame(v4l2_device_t *device, unsigned char* dest);

#ifdef __cplusplus
}
#endif

#endif // V4L2_H
