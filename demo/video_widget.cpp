#include "video_widget.h"
#include <QKeyEvent>
#include "render_ctrl.h"
#include "gl_renderer.h"
#include "camera_device.h"
#include "screen_capturer.h"
#include "window_capturer.h"
#include "composite_capturer.h"

#include <wayland-egl-core.h>
#include <qpa/qplatformnativeinterface.h>

VideoWidget::VideoWidget(QWidget *parent) : QWidget{parent}
{
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_PaintOnScreen);
  setAttribute(Qt::WA_NoSystemBackground);

  mRenderCtrl = new RenderCtrl();
  mGLRenderer = new GLRenderer(mRenderCtrl);
  mRenderCtrl->start();
  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, [=](){
    _feedData();
  });
  timer->start(33);

  winCapturer = new
   WindowCapturer();
//   ScreenCapturer();
//   CameraDevice();
  capDevice = new CompositeCapturer((ICaptureDevice*) winCapturer);
  selectDevice();
}

VideoWidget::~VideoWidget()
{
  if (mGLRenderer) {
    mRenderCtrl->clear_renderers();
    mRenderCtrl->stop();
    delete mGLRenderer;
    mGLRenderer = nullptr;
  }

  if (mGLRenderer) {
    delete mGLRenderer;
    mGLRenderer = nullptr;
  }

  if (capDevice) {
    capDevice->stop_device();
    capDevice->unbind_device();
    delete capDevice;
    capDevice = nullptr;
  }

  if (native_window_) {
    wl_egl_window_destroy(native_window_);
    native_window_ = nullptr;
  }

  delete timer;
  timer = nullptr;
  delete winCapturer;
  winCapturer = nullptr;
}

void VideoWidget::selectDevice()
{
  const std::vector<DeviceInfo>& list = capDevice->enum_devices();
  DeviceInfo dev = list[list.size() - 1];
  capDevice->bind_device(dev);
  capDevice->start_device();
  DeviceInfo& info = capDevice->get_cur_device();
  mGLRenderer->set_texture_format(info.format_);
}

QPaintEngine* VideoWidget::paintEngine() const
{
  return nullptr;
}

void VideoWidget::showEvent(QShowEvent* showEvent)
{
  QWidget::showEvent(showEvent);
  if(mIsInited == false) _init();
}

void VideoWidget::resizeEvent(QResizeEvent* resizeEvent)
{
  QWidget::resizeEvent(resizeEvent);
  auto sz = resizeEvent->size();
  if((sz.width() < 0) || (sz.height() < 0)) return;
  mGLRenderer->set_output_size(sz.width(), sz.height());
  if (native_window_) {
    wl_egl_window_resize(native_window_, sz.width(), sz.height(), 0, 0);
  }
}

void VideoWidget::_feedData()
{
  if(isVisible() == false)
    return;
  if(mIsInited == false)
    return;

  unsigned char* pixels = nullptr;
  if (capDevice->grab_frame(pixels) > 0) {
    int texWidth = capDevice->get_cur_device().width_;
    int texHeight = capDevice->get_cur_device().height_;
    mGLRenderer->upload_texture(&pixels, 1, texWidth, texHeight);
  }
}

void VideoWidget::_init()
{
  // XID nativeWindowHandler = winId();
  QPlatformNativeInterface* native = QApplication::platformNativeInterface();
  wl_surface *surface = static_cast<wl_surface*>(native->nativeResourceForWindow("surface", windowHandle()));
  native_window_ = wl_egl_window_create(surface, width(), height());
  std::string source_id = "";
  mRenderCtrl->add_renderer(mGLRenderer);
  mGLRenderer->bind_window_for_source(native_window_, source_id);
  mIsInited = true;
}
