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
  strncpy(device->name_, device_name, sizeof(device->name_));
  return device;
}

void v4l2_destroy_device(v4l2_device_t* device) {
  free(device);
}

int v4l2_open_device(v4l2_device_t* device) {
  struct stat st;
  if (-1 == stat(device->name_, &st)) {
    fprintf(stderr, "Cannot identify device: %s\n", device->name_);
    return V4L2_STATUS_ERROR;
  }
  
  if (!S_ISCHR(st.st_mode)) {
    fprintf(stderr, "Unrecognized device: %s\n", device->name_);
    return V4L2_STATUS_ERROR;
  }
  
  //open device
  device->fd_ = open(device->name_, O_RDWR | O_NONBLOCK, 0);
  if (-1 == device->fd_) {
    fprintf(stderr, "Unable to open device: %s\n", device->name_);
    return V4L2_STATUS_ERROR;
  }
  
  struct v4l2_capability cap;
  if (-1 == xioctl(device->fd_, VIDIOC_QUERYCAP, &cap)) {
    if (EINVAL == errno) {
      fprintf(stderr, "Error: %s is not V4L2 device\n", device->name_);
    } else {
      fprintf(stderr, "Error querying capabilities on device: %s\n", device->name_);
    }
    return V4L2_STATUS_ERROR;
  }
  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    fprintf(stderr, "Error: %s is no video capture device\n", device->name_);
    return V4L2_STATUS_ERROR;
  }
  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    fprintf(stderr, "Error: %s does not support streaming\n", device->name_);
    return V4L2_STATUS_ERROR;
  }
  strncpy(device->description_, (char *)cap.card, strlen(device->description_));

  struct v4l2_crop crop;
  struct v4l2_cropcap cropcap;
  CLEAR(cropcap);
  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (0 == xioctl(device->fd_, VIDIOC_CROPCAP, &cropcap)) {
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect; /* reset to default */
    xioctl(device->fd_, VIDIOC_S_CROP, &crop);
  }

  struct v4l2_format fmt;
  CLEAR(fmt);
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (device->format_.width_ > 0 && device->format_.height_ > 0) {
    fmt.fmt.pix.width       = device->format_.width_;
    fmt.fmt.pix.height      = device->format_.height_;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_ANY;
    if (-1 == xioctl(device->fd_, VIDIOC_S_FMT, &fmt)) {
      fprintf(stderr, "Error setting format on device: %s\n", device->name_);
    }
  }
  v4l2_get_format(device, &device->format_);

  //init buffer
  struct v4l2_requestbuffers req;
  CLEAR(req);
  req.count = 4;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  if (-1 == xioctl(device->fd_, VIDIOC_REQBUFS, &req)) {
    if (EINVAL == errno) {
      fprintf(stderr, "Error: %s does not support memory mapping\n", device->name_);
    } else {
      fprintf(stderr, "Error requesting buffers on device: %s\n", device->name_);
    }
  }
  if (req.count < 2) {
    fprintf(stderr, "Insufficient buffer memory on %s\n", device->name_);
    return V4L2_STATUS_ERROR;
  }
  //allocate buffers
  device->buffers_ = (v4l2_buffer_t*)calloc(req.count, sizeof(v4l2_buffer_t));
  if (!device->buffers_) {
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
    
    if (-1 == xioctl(device->fd_, VIDIOC_QUERYBUF, &buf)) {
      fprintf(stderr, "Unable to query buffers on %s\n", device->name_);
      return V4L2_STATUS_ERROR;
    }
    
    device->buffers_[i].length_ = buf.length;
    device->buffers_[i].start_ = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, device->fd_, buf.m.offset);
    
    if (MAP_FAILED == device->buffers_[i].start_) {
      fprintf(stderr, "Unable to map buffers on %s\n", device->name_);
      return V4L2_STATUS_ERROR;
    }    
  }
  device->num_buffers_ = req.count;
  
  return V4L2_STATUS_OK;
}

int v4l2_close_device(v4l2_device_t *device) {
  for (unsigned int i = 0; i < device->num_buffers_; i++) {
    if (-1 == munmap(device->buffers_[i].start_, device->buffers_[i].length_)) {
      fprintf(stderr, "Unable to unmap buffers on %s\n", device->name_);
      return V4L2_STATUS_ERROR;
    }
  }
  
  if (-1 == close(device->fd_)) {
    fprintf(stderr, "Unable to close device: %s\n", device->name_);
    return V4L2_STATUS_ERROR;
  }
  
  return V4L2_STATUS_OK;
}

int v4l2_set_format(v4l2_device_t* device, v4l2_format_t* format) {
  struct v4l2_format fmt;
  
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = format->width_;
  fmt.fmt.pix.height = format->height_;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  fmt.fmt.pix.field = V4L2_FIELD_ANY;
  
  if (-1 == xioctl(device->fd_, VIDIOC_S_FMT, &fmt)) {
    fprintf(stderr, "Could not set format[%dx%d] on device: %s\n",
        format->width_, format->height_, device->name_);
    return V4L2_STATUS_ERROR;
  } else {
    return V4L2_STATUS_OK;
  }
}

int v4l2_get_format(v4l2_device_t* device, v4l2_format_t* format) {
  struct v4l2_format fmt;
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (-1 == xioctl(device->fd_, VIDIOC_G_FMT, &fmt)) {
    fprintf(stderr, "Could not get format on device: %s\n", device->name_);
    return V4L2_STATUS_ERROR;
  }
  format->width_ = fmt.fmt.pix.width;
  format->height_ = fmt.fmt.pix.height;
  format->pixel_format_ = fmt.fmt.pix.pixelformat;
  
  return V4L2_STATUS_OK;
}

int v4l2_start_capture(v4l2_device_t *device) {
  //allocate data buffer
  device->data_ = (unsigned char*)malloc(device->buffers_[0].length_);
  
  //queue buffers
  for (unsigned int i = 0; i < device->num_buffers_; i++) {
    struct v4l2_buffer buf;
    CLEAR(buf);
    
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    if (-1 == xioctl(device->fd_, VIDIOC_QBUF, &buf)) {
      fprintf(stderr, "Unable to queue buffers on device: %s\n", device->name_);
      return V4L2_STATUS_ERROR;
    }
  }
  
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  //turn on stream
  if (-1 == xioctl(device->fd_, VIDIOC_STREAMON, &type)) {
    fprintf(stderr, "Unable to turn on stream on device: %s\n", device->name_);
    return V4L2_STATUS_ERROR;
  }
  
  return V4L2_STATUS_OK;
}

int v4l2_stop_capture(v4l2_device_t *device) {
  //free data buffer
  free(device->data_);
  
  //turn off stream
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  
  if (-1 == xioctl(device->fd_, VIDIOC_STREAMOFF, &type)) {
    fprintf(stderr, "Unable to turn off stream on device: %s\n", device->name_);
    return V4L2_STATUS_ERROR;
  }
  
  return V4L2_STATUS_OK;
}

int v4l2_grab_frame(v4l2_device_t *device) {
  struct v4l2_buffer frame_buffer;
  
  //dequeue buffer
  CLEAR(frame_buffer);

  frame_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
  if (-1 == xioctl(device->fd_, VIDIOC_DQBUF, &frame_buffer)) {
    return V4L2_STATUS_ERROR;
  }
  
  memcpy(device->data_, device->buffers_[frame_buffer.index].start_, frame_buffer.bytesused);

  //requeue buffer
  if (-1 == xioctl(device->fd_, VIDIOC_QBUF, &frame_buffer)) {
    fprintf(stderr, "Could not requeue buffer on device: %s\n", device->name_);
    return V4L2_STATUS_ERROR;
  }
  
  return V4L2_STATUS_OK;
}

void v4l2_copy_frame(v4l2_device_t *device, unsigned char* dest) {
  memcpy(dest, device->data_, device->buffers_[0].length_);
}

#ifdef __cplusplus
}
#endif
