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

    void render();

public:
    bool continuousRender = false;

protected:
    virtual void paintEvent(QPaintEvent* paintEvent) override;
    virtual void showEvent(QShowEvent* showEvent) override;
    virtual void resizeEvent(QResizeEvent* resizeEvent) override;
    virtual bool event(QEvent* event) override;

private:
    void _doRender();
    void _init();

    void calcFPS();
    void updateFPS(qreal);
    void paintFPS();

private:
    bool mUpdatePending = false;
    bool mIsInited = false;

    void selectDevice(); // test function
    GLRenderer* mGLRenderer = nullptr;
    int wndWidth = 0;
    int wndHeight = 0;
    QTimer *timer;
    qreal fps;

    PixelFormat curFormat;
    int texWidth = 0;
    int texHeight = 0;
    bool isSizeChanged = false;
    ICaptureDevice* capDevice = nullptr;
    ICaptureDevice* winCapturer = nullptr;

};

#endif // VIDEOWIDGET_H
