#include "x_window_env.h"
#include <mutex>

namespace XWindowEnv {

static void* x_display_ = nullptr;
static std::mutex dpy_mtx_;

void set_x_display(void* display) {
  std::unique_lock<std::mutex> lock(dpy_mtx_);
  x_display_ = display;
}

void* get_x_display() {
  std::unique_lock<std::mutex> lock(dpy_mtx_);
  return x_display_;
}

}
