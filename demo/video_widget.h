#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QResizeEvent>
#include <QApplication>
#include "capture_interface.h"

class GLRenderer;

class VideoWidget : public QWidget
{
  Q_OBJECT
public:
  explicit VideoWidget(QWidget *parent = nullptr);
  virtual ~VideoWidget();
  virtual QPaintEngine* paintEngine() const override;

protected:
  virtual void showEvent(QShowEvent* showEvent) override;
  virtual void resizeEvent(QResizeEvent* resizeEvent) override;

private:
  void _feedData();
  void _init();

private:
  void selectDevice(); // test function
  bool mIsInited = false;

  GLRenderer* mGLRenderer = nullptr;
  QTimer *timer = nullptr;

  int texWidth = 0;
  int texHeight = 0;
  ICaptureDevice* capDevice = nullptr;
  ICaptureDevice* winCapturer = nullptr;
};

#endif // VIDEOWIDGET_H