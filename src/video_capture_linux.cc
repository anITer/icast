/*
** Copyright (c) 2020 The XCast project. All rights reserved.
** Created by adamsyang 2020
*/
#include "../video_capture_manager.h"
#include "../video_capture.h"
#include "xc_media_util.h"
#include "xc_memory.h"
#include "xc_util.h"
#include "v4l2.h"

static const std::string DEVICE_PREFIX = "/dev/video";
static const int MAX_CAMERA_NUMBER = 4;
static const int MIN_CAPTURE_INTERVAL_MS = 1;

typedef struct video_capture_linux_s {
  xc_thread_t   *worker_thread_;
  xc_mutex_t    *camera_mutex_;
  v4l2_device_t *camera_dev_;
  bool           is_capturing_;
} video_capture_linux_t;

static int32_t
video_capture_manager_init(xc_device_manager_t *dm)
{
  return XC_OK;
}

static int32_t
video_capture_manager_uninit(xc_device_manager_t *dm)
{
  return XC_OK;
}

static int32_t
video_capture_manager_enum(xc_device_manager_t *dm, xc_variant_t *devs)
{
  v4l2_device_t* tmp_cam = nullptr;
  for (int i = 0; i < MAX_CAMERA_NUMBER; i++) {
    std::string tmp_name = DEVICE_PREFIX + std::to_string(i);
    tmp_cam = v4l2_create_device(tmp_name.c_str());
    if (!tmp_cam) continue;
    if (v4l2_open_device(tmp_cam) != V4L2_STATUS_OK) {
        v4l2_destroy_device(tmp_cam);
        continue;
    } 
    xc_variant_t *cap_info = vdict_new();
    vdict_set_str(cap_info, "name", tmp_name.c_str());
    vdict_set_uint32(cap_info, "sub-type", xc_camera_unspecified);
    vdict_set_str(cap_info, "id", itoa(i));
    xc_variant_array_append(cap_infos, cap_info);
    xc_variant_unref(cap_info);
    v4l2_close_device(tmp_cam);
    v4l2_destroy_device(tmp_cam);
  }
  return XC_OK;
}

xc_device_manager_entry_t video_capture_manager_entry = {
  .init = video_capture_manager_init,
  .uninit = video_capture_manager_uninit,
  .enum_device = video_capture_manager_enum,
};

static int32_t
video_capture_init(xc_device_t *dev)
{
  video_capture_linux_t *cap = xc_zalloc(video_capture_linux_t);
  if (!cap) {
    return XC_ERR_OUT_OF_MEMORY;
  }

  cap->camera_mutex_ = xc_mutex_new("cam-mtx");
  if (!cap->camera_mutex_) {
    xc_free(cap);
    return XC_ERR_OUT_OF_MEMORY;
  }

  cap->camera_dev_ = v4l2_create_device(dev->id);
  if (!cap->camera_dev_) {
    xc_mutex_free(cap->camera_mutex_);
    xc_free(cap);
    return XC_ERR_OUT_OF_MEMORY;
  }

  cap->worker_thread_ = xc_thread_new("vid-cap", dev);
  if (!cap->worker_thread_) {
    v4l2_destroy_device(cap->camera_dev_);
    xc_mutex_free(cap->camera_mutex_);
    xc_free(cap);
    return XC_ERR_OUT_OF_MEMORY;
  }

  cap->is_capturing_ = false;
  dev->data = cap;

  return XC_OK;
}

static int32_t
video_capture_uninit(xc_device_t *dev)
{
  video_capture_linux_t *cap = (video_capture_linux_t *)dev->data;

  if (cap) {
    xc_thread_free(cap->worker_thread_);
    v4l2_destroy_device(cap->camera_dev_);
    xc_mutex_free(cap->camera_mutex_);
    xc_free(cap);
    dev->data = xc_null;

    return XC_OK;
  }

  return XC_ERR_FAILED;
}

static int32_t
video_capture_start(xc_device_t *dev)
{
  video_capture_linux_t *cap = (video_capture_linux_t *)dev->data;

  if (cap && !cap->is_capturing_) {
    xc_mutex_acquire(cap->camera_mutex_);
    if (v4l2_open_device(cap->camera_dev_) != V4L2_STATUS_OK) {
      xc_mutex_release(cap->camera_mutex_);
      return XC_ERR_DEVICE_START_FAILED;
    }
    v4l2_get_format(cap, &cap->format_);
    if (v4l2_start_capture(cap) != V4L2_STATUS_OK) {
      v4l2_close_device(cap);
      xc_mutex_release(cap->camera_mutex_);
      return XC_ERR_DEVICE_START_FAILED;
    }
    xc_thread_start(cap->worker_thread_, (xc_func_pt)video_capture_worker);
    cap->is_capturing_ = true;
    xc_mutex_release(cap->camera_mutex_);
    return XC_OK;
  }
  return XC_ERR_DEVICE_START_FAILED;
}

static int32_t
video_capture_stop(xc_device_t *dev)
{
  video_capture_linux_t *cap = (video_capture_linux_t *)dev->data;
 
  if (cap && cap->is_capturing_) {
    xc_mutex_acquire(cap->camera_mutex_);
    xc_thread_set_quit(cap->worker_thread_);
    xc_mutex_signal(cap->camera_mutex_, false);
    xc_thread_stop(cap->worker_thread_);
    
    if (v4l2_stop_capture(cap->camera_dev_) != V4L2_STATUS_OK) {
      xc_mutex_release(cap->camera_mutex_);
      return XC_ERR_DEVICE_STOP_FAILED;
    }
    if (v4l2_close_device(cap->camera_dev_) != V4L2_STATUS_OK) {
      xc_mutex_release(cap->camera_mutex_);
      return XC_ERR_DEVICE_STOP_FAILED;
    }
    cap->is_capturing_ = false;
    xc_mutex_release(cap->camera_mutex_);
  }
  return XC_ERR_DEVICE_STOP_FAILED;
}

static int32_t
video_capture_pause(xc_device_t *dev)
{
  XC_UNUSED(dev);
  return XC_ERR_NOT_IMPLEMENTED;
}

static int32_t
video_capture_config(xc_device_t *dev, xc_variant_t *params)
{
  video_capture_t *vc = (video_capture_t *)dev;
  video_capture_linux_t *cap = (video_capture_linux_t *)dev->data;
  
  if (!cap || !params) return XC_ERR_INVALID;

  xc_video_config_set(vc, params);
  if (xc_video_need_reset(vc) && !cap->is_capturing_/*TODO:: hot change*/) {
    xc_size_t cap_size = vc->config.cap_size;
    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = cap_size.cx;
    fmt.fmt.pix.height = cap_size.cy;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // TODO:: support orther format
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
  
    xc_mutex_acquire(cap->camera_mutex_);
    if (xioctl(cap->camera_dev_->_fd, VIDIOC_S_FMT, &fmt) == V4L2_STATUS_ERROR) {
      xc_mutex_release(cap->camera_mutex_);
      xc_log(XC_LOG_ERR, "%p.size.config.leave.failed", cap);
      return NOT_SUPPORTED;
    }
    xc_mutex_release(cap->camera_mutex_);

    uint32_t fps = vc->config.fps;
    struct v4l2_streamparm param;
    param.type = V4L2_BUF_TYPE_META_CAPTURE;
    param.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;
    param.parm.capture.timeperframe.numerator = 1;
    param.parm.capture.timeperframe.denominator = fps;

    xc_mutex_acquire(cap->camera_mutex_);
    if (v4l2_set_param(cap->camera_dev_, &param) == V4L2_STATUS_OK) {
      xc_mutex_release(cap->camera_mutex_);
      return XC_OK;
    } else {
      xc_mutex_release(cap->camera_mutex_);
      xc_log(XC_LOG_ERR, "%p.fps.config.leave.failed", cap);
      return NOT_SUPPORTED;
    }
  }

  return XC_ERR_FAILED;
}

static void
attached_frame_free(void *data)
{
  XC_UNUSED(data);
}

static int32_t
attached_frame_map(void *data, xc_media_frame_mem_t *mapped_mem, uint32_t flag)
{
  XC_UNUSED(flag);
  *mapped_mem = *((xc_media_frame_mem_t *)data);
  return XC_OK;
}

static int32_t
attached_frame_unmap(void *data, uint32_t flag)
{
  XC_UNUSED(data);
  XC_UNUSED(flag);
  return XC_OK;
}

static int32_t
do_video_capture(video_capture_t *vc, uint64_t tick)
{
  video_capture_linux_t *cap = (video_capture_linux_t *)vc->base.data;
  if (!cap || !cap->is_capturing_) {
    return XC_OK;
  }

  xc_mutex_acquire(cap->camera_mutex_);
  if (v4l2_grab_frame(cap) != V4L2_STATUS_OK) {
    // TODO:: check if device plugged in or not
    xc_mutex_release(cap->camera_mutex_);
    return XC_ERR_FAILED;
  }
  
  xc_video_format_desc_t desc;
  memset(&desc, 0, sizeof(desc));
  desc.format = xc_video_format_yuyv; // TODO:: suport orther format
  desc.width = (uint32_t)cap->format_.width_;
  desc.height = (uint32_t)cap->format_.height_;
  xc_media_frame_mem_t mem = { 0, { 0 }, { 0 }, { 0 } };
  mem.planes_num = 1;
  mem.plane[0] = (uint8_t *)cap->data;
  mem.stride[0] = (uint32_t)XC_ALIGN((uint32_t)desc.width / 2, 4);

  xc_media_frame_t *attached =
    xc_video_frame_custom_alloc(desc, &mem, attached_frame_free, attached_frame_map, attached_frame_unmap);

  if (!attached) {
    xc_mutex_release(cap->camera_mutex_);
    return XC_ERR_FAILED;
  }

  xc_video_media_frame_output(vc, attached);
  xc_media_frame_unref(attached);
  xc_mutex_release(cap->camera_mutex_);

  return XC_OK;
}

static int32_t
video_capture_proc(xc_thread_t *t, video_capture_t *vc)
{
  video_capture_linux_t *cap = (video_capture_linux_t *)vc->base.data;
  int32_t              delay = 0;
  int32_t           interval = (int32_t)XC_MSEC_PER_SEC / vc->config.fps;
  
  while (!xc_thread_is_quit(t)) {
    uint64_t start, end;

    int32_t wait = xc_mutex_wait(cap->camera_mutex_, delay * XC_USEC_PER_MSEC);
    if (wait && wait != XC_ERR_TIME_OUT) {
      xc_log(XC_LOG_ERR, "CAP|video_capture_proc wait error!");
      return CX_ERR_FAILED;
    }

    if (xc_thread_is_quit(t)) {
      return XC_OK;
    }
    
    start = xc_tick_count();
    do_video_capture(vp, start);
    end = xc_tick_count();
    
    delay = CLAMP(interval - (int32_t)(end - start), MIN_CAPTURE_INTERVAL_MS, delay);
  }
  
  return XC_OK;
}

static int32_t
video_capture_worker(xc_thread_t* t, xc_device_t *dev)
{
  video_capture_t *vc = (video_capture_t *)dev;
    
  return video_capture_proc(t, vc);
}

xc_device_entry_t video_capture_entry = {
  .init = video_capture_init,
  .uninit = video_capture_uninit,
  .start = video_capture_start,
  .stop = video_capture_stop,
  .pause = video_capture_pause,
  .config = (xc_func_pt)video_capture_config,
  .worker = video_capture_worker,
};