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
  timer = new QTimer(this);
  fps = 0;
  connect(timer, &QTimer::timeout, [=](){
    render();
  });
  timer->start(33);

  winCapturer = new
   WindowCapturer();
//   ScreenCapturer();
//   CameraDevice();
  capDevice = new CompositeCapturer((ICaptureDevice*) winCapturer);
  selectDevice();
  mGLRenderer->start();
}

VideoWidget::~VideoWidget()
{
  if (mGLRenderer) {
    mGLRenderer->stop();
    delete mGLRenderer;
  }
  mGLRenderer = nullptr;
  delete timer;
  capDevice->stop_device();
  capDevice->unbind_device();
  delete capDevice;
  capDevice = nullptr;

  delete winCapturer;
  winCapturer = nullptr;
}

void VideoWidget::selectDevice()
{
  const std::vector<DeviceInfo>& list = capDevice->enum_devices();
  for (DeviceInfo info : list) {
    printf("find device with info [%s | %dx%d]\n", info.name_.c_str(), info.width_, info.height_);
  }
  DeviceInfo dev = list[0];
  capDevice->bind_device(dev);
  DeviceInfo& info = capDevice->get_cur_device();
  texWidth = info.width_;
  texHeight = info.height_;
  curFormat = info.format_;
  capDevice->start_device();
  mGLRenderer->set_texture_format(curFormat);

  resize(texWidth, texHeight);
}

QPaintEngine* VideoWidget::paintEngine() const
{
  return nullptr;
}

void VideoWidget::render()
{
  if(mUpdatePending == false)
  {
    mUpdatePending = true;
    QApplication::postEvent(this, new QEvent{QEvent::UpdateRequest});
  }
}

void VideoWidget::paintEvent(QPaintEvent* paintEvent)
{
  if(mIsInited == false)
    _init();
  render();
}

void VideoWidget::showEvent(QShowEvent* showEvent)
{
  QWidget::showEvent(showEvent);
  if(mIsInited == false)
    _init();
}

void VideoWidget::resizeEvent(QResizeEvent* resizeEvent)
{
  QWidget::resizeEvent(resizeEvent);
  auto sz = resizeEvent->size();
  if((sz.width() < 0) || (sz.height() < 0))
    return;

  wndWidth = sz.width();
  wndHeight = sz.height();
  mGLRenderer->set_output_size(sz.width(), sz.height());

  // because Qt is not sending update request when resizing smaller
  render();
}

bool VideoWidget::event(QEvent* event)
{
  switch(event->type())
  {
  case QEvent::UpdateRequest:
    mUpdatePending = false;
    _doRender();
    return true;
  default:
    return QWidget::event(event);
  }
}

void VideoWidget::_doRender()
{
  if(isVisible() == false)
    return;
  if(mIsInited == false)
    return;

  // you may want to add some code to control rendering frequency
  // and ensure you are not rendering too fast in case of continuous
  // rendering...
  // if you control the rendering frequency, don't forget to make a
  // call to render() if you're not going to do the rendering...

  unsigned char* pixels = nullptr;
  if (capDevice->grab_frame(pixels) > 0) {
    texWidth = capDevice->get_cur_device().width_;
    texHeight = capDevice->get_cur_device().height_;
    mGLRenderer->upload_texture(&pixels, 1, texWidth, texHeight);
  }
//  mGLRenderer->draw();

  calcFPS();
  paintFPS();

  // next frame if rendering continuously
  if(continuousRender == true)
    render();
}

void VideoWidget::_init()
{
  // you can grab the native window handler (HWND for Windows) of
  // this widget:
  auto nativeWindowHandler = winId();

//  mGLRenderer->setup();
  mGLRenderer->bind_window(nativeWindowHandler);

  mIsInited = true;
}

void VideoWidget::calcFPS()
{
  static QElapsedTimer time;
  static int once = [=](){time.start(); return 0;}();
  Q_UNUSED(once)
  static int frame = 0;
  if (frame++ > 100) {
    qreal elasped = time.elapsed();
    updateFPS(frame/ elasped * 1000);
    time.restart();
    frame = 0;
  }
}

void VideoWidget::updateFPS(qreal v)
{
  fps = v;
}

void VideoWidget::paintFPS()
{
//  QPainter painter(this);
//  painter.setPen(Qt::green);
//  painter.setRenderHint(QPainter::TextAntialiasing);
//  painter.drawText(10, 10, QString("FPS:%1").arg(QString::number(fps, 'f', 3)));
}
