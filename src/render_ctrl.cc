#include "render_ctrl.h"
#include "gl_renderer.h"
#include <sys/time.h>
#include <unistd.h>

static const int RENDERER_STATE_IDLE = 1;
static const int RENDERER_STATE_READY = 2;
static const int RENDERER_STATE_RELEASE = 3;
static const int RENDERER_STATE_DEAD = 4;

RenderCtrl::RenderCtrl() : texture_cache_(), renderer_list_()
{

}

RenderCtrl::~RenderCtrl()
{
  stop();
  renderer_list_.clear();
  std::map<GLRenderer*, int>().swap(renderer_list_);
  texture_cache_.purge_cache();
}

void RenderCtrl::start()
{
  if (is_running_) return;
  is_running_ = true;
  pthread_create(&render_thread_, nullptr, render_loop, this);
}

void RenderCtrl::stop()
{
  if (!is_running_) return;
  is_running_ = false;
  pthread_join(render_thread_, nullptr);
}

void RenderCtrl::set_fps(float fps)
{
  if (fps < 0.0001) return;
  interval_us_ = (int) (1000000 / fps);
}

EGLSurface RenderCtrl::create_surface(Window &window) {
  return egl_core_->create_window_surface(window);
}

void RenderCtrl::release_surface(EGLSurface &surface) {
  egl_core_->release_surface(surface);
}

void RenderCtrl::setup_egl()
{
  if (egl_core_) return;

  egl_core_ = new EglCore();
  cur_background_surface_ = egl_core_->create_offscreen_surface(1, 1);
  egl_core_->make_current(cur_background_surface_);
}

void RenderCtrl::release_egl() {
  if (egl_core_) {
    if (cur_background_surface_) egl_core_->release_surface(cur_background_surface_);
    cur_background_surface_ = 0;
    delete egl_core_;
  }
  egl_core_ = nullptr;
}

void* RenderCtrl::render_loop(void* data)
{
  RenderCtrl* renderer = (RenderCtrl *)data;

  renderer->setup_egl();

  timeval tmp_time;
  long start = 0;
  long end = renderer->interval_us_;
  while (renderer->is_running_) {
    usleep(std::max(0L, renderer->interval_us_ - (end - start)));
    if (!renderer->is_running_) break;

    gettimeofday(&tmp_time, nullptr);
    start = tmp_time.tv_sec * 1000000 + tmp_time.tv_usec;
    renderer->setup_renderers();
    renderer->do_rendering();
    renderer->release_renderers();
    gettimeofday(&tmp_time, nullptr);
    end = tmp_time.tv_sec * 1000000 + tmp_time.tv_usec;
  }

  renderer->release_renderers();
  renderer->release_egl();
  return nullptr;
}

void RenderCtrl::swap_buffer(EGLSurface &surface)
{
  egl_core_->swap_buffers(surface);
}

void RenderCtrl::make_current(EGLSurface &surface)
{
  if (surface != EGL_NO_SURFACE) {
    egl_core_->make_current(surface);
  } else {
    egl_core_->make_current(cur_background_surface_);
  }
}

void RenderCtrl::add_renderer(GLRenderer *renderer)
{
  std::unique_lock<std::mutex> lock(render_mutex_);
  if (renderer_list_.find(renderer) == renderer_list_.end()
    || renderer_list_[renderer] == RENDERER_STATE_DEAD) {
    renderer_list_[renderer] = RENDERER_STATE_IDLE;
  }
}

void RenderCtrl::remove_renderer(GLRenderer *renderer)
{
  std::unique_lock<std::mutex> lock(render_mutex_);
  if (renderer_list_.find(renderer) != renderer_list_.end()) {
    renderer_list_[renderer] = RENDERER_STATE_RELEASE;
  }
  is_done_release_ = false;
  cv_.wait(lock, [&] { return is_done_release_; });
}

void RenderCtrl::clear_renderers()
{
  std::unique_lock<std::mutex> lock(render_mutex_);
  for (auto renderer : renderer_list_) {
    renderer_list_[renderer.first] = RENDERER_STATE_RELEASE;
  }
  is_done_release_ = false;
  cv_.wait(lock, [&] { return is_done_release_; });
}

void RenderCtrl::do_rendering()
{
  std::unique_lock<std::mutex> lock(render_mutex_);
  for (auto renderer : renderer_list_) {
    if (renderer.second == RENDERER_STATE_READY) {
      renderer.first->draw();
    }
  }
}

void RenderCtrl::setup_renderers()
{
  std::unique_lock<std::mutex> lock(render_mutex_);
  for (auto renderer : renderer_list_) {
    if (renderer.second == RENDERER_STATE_IDLE) {
      renderer.first->setup();
      renderer_list_[renderer.first] = RENDERER_STATE_READY;
    }
  }
}

void RenderCtrl::release_renderers()
{
  std::unique_lock<std::mutex> lock(render_mutex_);
  for (auto renderer : renderer_list_) {
    if (renderer.second == RENDERER_STATE_RELEASE) {
      renderer.first->release();
      renderer_list_[renderer.first] = RENDERER_STATE_DEAD;
    }
  }
  is_done_release_ = true;
  lock.unlock();
  cv_.notify_all();
}

Texture* RenderCtrl::fetch_texture(int width, int height, Cacheable::Attributes *attribute)
{
  return texture_cache_.fetch_object(width, height, (Texture::Attributes *)attribute);
}

void RenderCtrl::return_texture(Texture *texture)
{
  texture_cache_.return_object(texture);
}
