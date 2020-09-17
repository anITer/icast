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
#include <v4l2.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C"{
#endif

#define CLEAR(x) memset(&(x), 0, sizeof(x))

static int xioctl(int fd, int request, void *arg) {
	int r;
	
	do {
		r = ioctl(fd, request, arg);	
	} while (-1 == r && EINTR == errno);
	
	return r;
}

v4l2_device_t* v4l2_create_device(const char* device_name) {
	v4l2_device_t* device = (v4l2_device_t*)malloc(sizeof(v4l2_device_t));
	
	strncpy(device->_name, device_name, sizeof(device->_name));
	
	return device;
}

void v4l2_destroy_device(v4l2_device_t* device) {
	free(device);
}

int v4l2_open_device(v4l2_device_t* device) {
	struct stat st;
	
	//identify device
	if (-1 == stat(device->_name, &st)) {
		fprintf(stderr, "Cannot identify device: %s\n", device->_name);
		return V4L2_STATUS_ERROR;
	}
	
	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "Unrecognized device: %s\n", device->_name);
		return V4L2_STATUS_ERROR;
	}
	
	//open device
	device->_fd = open(device->_name, O_RDWR | O_NONBLOCK, 0);
	
	if (-1 == device->_fd) {
		fprintf(stderr, "Unable to open device: %s\n", device->_name);
		return V4L2_STATUS_ERROR;
	}
	
	struct v4l2_capability cap;
	
	//query capabilities
	if (-1 == xioctl(device->_fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "Error: %s is no V4L2 device\n", device->_name);
		} else {
			fprintf(stderr, "Error querying capabilities on device: %s\n", device->_name);
		}
		
		return V4L2_STATUS_ERROR;
	}
	
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "Error: %s is no video capture device\n", device->_name);
		return V4L2_STATUS_ERROR;
	}
	
	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		fprintf(stderr, "Error: %s does not support streaming\n", device->_name);
		return V4L2_STATUS_ERROR;
	}
	
    //init buffer
	struct v4l2_requestbuffers req;
	
	CLEAR(req);
	
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	
	if (-1 == xioctl(device->_fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "Error: %s does not support memory mapping\n", device->_name);
		} else {
			fprintf(stderr, "Error requesting buffers on device: %s\n", device->_name);
		}
		
        req.memory = V4L2_MEMORY_USERPTR;
        if (-1 == xioctl(device->_fd, VIDIOC_REQBUFS, &req)) {
            fprintf(stderr, "Error: %s does not support memory user_pointer\n", device->_name);
            return V4L2_STATUS_ERROR;
        }
	}
	
	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n", device->_name);
		return V4L2_STATUS_ERROR;
	}
	
	//allocate buffers
	device->_buffers = (v4l2_buffer_t*)calloc(req.count, sizeof(v4l2_buffer_t));
	
	if (!device->_buffers) {
		fprintf(stderr, "Out of memory");
		return V4L2_STATUS_ERROR;
	}
	
	//map buffers
    for (unsigned int i = 0; i < req.count; i++) {
		struct v4l2_buffer buf;
		
		CLEAR(buf);
		
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = req.memory;
		buf.index = i;
		
		if (-1 == xioctl(device->_fd, VIDIOC_QUERYBUF, &buf)) {
			fprintf(stderr, "Unable to query buffers on %s\n", device->_name);
			return V4L2_STATUS_ERROR;
		}
		
		device->_buffers[i]._length = buf.length;
        device->_buffers[i]._type = (v4l2_memory) req.memory;
        if (buf.memory == V4L2_MEMORY_MMAP) {
            device->_buffers[i]._start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, device->_fd, buf.m.offset);
        } else if (buf.memory == V4L2_MEMORY_USERPTR) {
            device->_buffers[i]._start = malloc(buf.length);
        }
		
		if (MAP_FAILED == device->_buffers[i]._start) {
			fprintf(stderr, "Unable to map buffers on %s\n", device->_name);
			return V4L2_STATUS_ERROR;
		}		
	}
	
	device->_num_buffers = req.count;
	
	return V4L2_STATUS_OK;
}

int v4l2_close_device(v4l2_device_t *device) {
    for (unsigned int i = 0; i < device->_num_buffers; i++) {
        if (device->_buffers[i]._type == V4L2_MEMORY_MMAP) {
            if (-1 == munmap(device->_buffers[i]._start, device->_buffers[i]._length)) {
                fprintf(stderr, "Unable to unmap buffers on %s\n", device->_name);
                return V4L2_STATUS_ERROR;
            }
        } else if (device->_buffers[i]._type == V4L2_MEMORY_USERPTR) {
            free(device->_buffers[i]._start);
            device->_buffers[i]._start = nullptr;
        }
	}
	
	if (-1 == close(device->_fd)) {
		fprintf(stderr, "Unable to close device: %s\n", device->_name);
		return V4L2_STATUS_ERROR;
	}
	
	return V4L2_STATUS_OK;
}

size_t v4l2_get_buffer_size(v4l2_device_t* device) {
	if (device->_num_buffers > 0) {
		return device->_buffers[0]._length;
	} else {
		return 0;
	}
}

int v4l2_set_format(v4l2_device_t* device, v4l2_format_t* format, int res_index) {
	struct v4l2_format fmt;
	
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = format->_width[res_index];
    fmt.fmt.pix.height = format->_height[res_index];
	fmt.fmt.pix.pixelformat = format->_pixel_format;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	
	if (-1 == xioctl(device->_fd, VIDIOC_S_FMT, &fmt)) {
        fprintf(stderr, "Could not set format[%dx%d] on device: %s\n",
                format->_width[res_index], format->_height[res_index], device->_name);
		return V4L2_STATUS_ERROR;
	} else {
		return V4L2_STATUS_OK;
	}
}

int v4l2_get_format(v4l2_device_t* device, v4l2_format_t* format) {
	struct v4l2_format fmt;
	
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (-1 == xioctl(device->_fd, VIDIOC_G_FMT, &fmt)) {
		fprintf(stderr, "Could not get format on device: %s\n", device->_name);
		return V4L2_STATUS_ERROR;
	}
	
    format->_width[0] = fmt.fmt.pix.width;
    format->_height[0] = fmt.fmt.pix.height;
	format->_pixel_format = fmt.fmt.pix.pixelformat;
    format->_reslution_num = 1;
	
	return V4L2_STATUS_OK;
}

int v4l2_enum_format(v4l2_device_t* device, v4l2_format_t* format) {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_fmtdesc fmt;
    struct v4l2_frmsizeenum frmsize;
    struct v4l2_frmivalenum frmival;

    fmt.index = 0;
    fmt.type = type;
    format->_reslution_num = 0;
    format->_pixel_format = V4L2_PIX_FMT_YUYV;
    for (; ioctl(device->_fd, VIDIOC_ENUM_FMT, &fmt) >= 0 && format->_reslution_num < 32; fmt.index++) {
        printf("find formatdesc [%d, %d, %d, %s, 0x%x]\n",
               fmt.index, fmt.type, fmt.flags, fmt.description, fmt.pixelformat);
        if (fmt.pixelformat != V4L2_PIX_FMT_YUYV) {
            continue;
        }
        frmsize.pixel_format = fmt.pixelformat;
        frmsize.index = 0;
        while (ioctl(device->_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0) {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                printf("V4L2_FRMSIZE_TYPE_DISCRETE: %dx%d\n",
                                  frmsize.discrete.width,
                                  frmsize.discrete.height);
                format->_width[format->_reslution_num] = frmsize.discrete.width;
                format->_height[format->_reslution_num] = frmsize.discrete.height;
            } else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
                printf("V4L2_FRMSIZE_TYPE_STEPWISE: %dx%d\n",
                                  frmsize.stepwise.max_width,
                                  frmsize.stepwise.max_height);
                format->_width[format->_reslution_num] = frmsize.stepwise.max_width;
                format->_height[format->_reslution_num] = frmsize.stepwise.max_height;
            }
            format->_reslution_num++;
            frmsize.index++;
        }
    }

    return V4L2_STATUS_OK;
}

int v4l2_start_capture(v4l2_device_t *device) {
	//allocate data buffer
	device->_data = (unsigned char*)malloc(device->_buffers[0]._length);
	
	//queue buffers
    for (unsigned int i = 0; i < device->_num_buffers; i++) {
		struct v4l2_buffer buf;
		
		CLEAR(buf);
		
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = device->_buffers[0]._type;
		buf.index = i;
		
		if (-1 == xioctl(device->_fd, VIDIOC_QBUF, &buf)) {
			fprintf(stderr, "Unable to queue buffers on device: %s\n", device->_name);
			return V4L2_STATUS_ERROR;
		}
	}
	
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	//turn on stream
	if (-1 == xioctl(device->_fd, VIDIOC_STREAMON, &type)) {
		fprintf(stderr, "Unable to turn on stream on device: %s\n", device->_name);
		return V4L2_STATUS_ERROR;
	}
	
	return V4L2_STATUS_OK;
}

int v4l2_stop_capture(v4l2_device_t *device) {
	//free data buffer
	free(device->_data);
	
	//turn off stream
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (-1 == xioctl(device->_fd, VIDIOC_STREAMOFF, &type)) {
		fprintf(stderr, "Unable to turn off stream on device: %s\n", device->_name);
		return V4L2_STATUS_ERROR;
	}
	
	return V4L2_STATUS_OK;
}

int v4l2_grab_frame(v4l2_device_t *device) {
	struct v4l2_buffer frame_buffer;
	
	//dequeue buffer
	CLEAR(frame_buffer);

	frame_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frame_buffer.memory = device->_buffers[0]._type;
	
	if (-1 == xioctl(device->_fd, VIDIOC_DQBUF, &frame_buffer)) {
		return V4L2_STATUS_ERROR;
	}
	
	memcpy(device->_data, device->_buffers[frame_buffer.index]._start, frame_buffer.bytesused);

	//requeue buffer
	if (-1 == xioctl(device->_fd, VIDIOC_QBUF, &frame_buffer)) {
		fprintf(stderr, "Could not requeue buffer on device: %s\n", device->_name);
		return V4L2_STATUS_ERROR;
	}
	
	return V4L2_STATUS_OK;
}

void v4l2_copy_frame(v4l2_device_t *device, unsigned char* dest) {
	memcpy(dest, device->_data, device->_buffers[0]._length);
}

#ifdef __cplusplus
}
#endif
