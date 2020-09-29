#include "video_widget.h"
#include <time.h>
#include <QPainter>
#include <QElapsedTimer>
#include <QKeyEvent>
#include "camera_device.h"
#include "screen_capturer.h"
#include "window_capturer.h"
#include "composite_capturer.h"
#include "gl_renderer.h"

VideoWidget::VideoWidget(QWidget *parent) : QWidget{parent}
{
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_PaintOnScreen);
  setAttribute(Qt::WA_NoSystemBackground);

  mGLRenderer = new GLRenderer();
  mGLRenderer->start();
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
    mGLRenderer->stop();
    delete mGLRenderer;
    mGLRenderer = nullptr;
  }

  if (capDevice) {
    capDevice->stop_device();
    capDevice->unbind_device();
    delete capDevice;
    capDevice = nullptr;
  }

  delete timer;
  timer = nullptr;
  delete winCapturer;
  winCapturer = nullptr;
}

void VideoWidget::selectDevice()
{
  const std::vector<DeviceInfo>& list = capDevice->enum_devices();
  DeviceInfo dev = list[0];
  capDevice->bind_device(dev);
  capDevice->start_device();
  DeviceInfo& info = capDevice->get_cur_device();
  texWidth = info.width_;
  texHeight = info.height_;
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
}

void VideoWidget::_feedData()
{
  if(isVisible() == false)
    return;
  if(mIsInited == false)
    return;

  unsigned char* pixels = nullptr;
  if (capDevice->grab_frame(pixels) > 0) {
    texWidth = capDevice->get_cur_device().width_;
    texHeight = capDevice->get_cur_device().height_;
    mGLRenderer->upload_texture(&pixels, 1, texWidth, texHeight);
  }
}

void VideoWidget::_init()
{
  auto nativeWindowHandler = winId();
  mGLRenderer->bind_window(nativeWindowHandler);
  mIsInited = true;
}
