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
#ifndef GL_WIDGET_H
#define GL_WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QTimer>
#include "capture_interface.h"

class GLWidget: public QOpenGLWidget,
                protected QOpenGLFunctions,
                protected OnDeviceUpdateCallback
{
	Q_OBJECT
public:
    GLWidget(QWidget *parent = 0);
    ~GLWidget();
protected:
	void initializeGL() Q_DECL_OVERRIDE;
	void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;
	void initShaders();
    void initTextures();
	void draw();

	void calcFPS();
	void updateFPS(qreal);
    void paintFPS();
private:
    void selectDevice(); // test function
    void resetTextureSize();
    int on_device_updated() override;

    int wndWidth = 0;
    int wndHeight = 0;
	QOpenGLShaderProgram program;
    GLuint texture;
    int mvpMatrixHandle;
    int verticesHandle;
    int texCoordHandle;
    int texWidthHandle;
    int colorMapHandle;

    float vertices[8] = {
        -1, -1,
         1, -1,
        -1,  1,
         1,  1,
    };
    float texcoords[8] = {
        0, 0,
        1, 0,
        0, 1,
        1, 1,
    };

    float mMvpMatrix[16];
    QTimer *timer;
	qreal fps;

    PixelFormat curFormat;
    int texWidth = 0;
    int texHeight = 0;
    bool isSizeChanged = false;
    ICaptureDevice* capDevice = nullptr;
    ICaptureDevice* winCapturer = nullptr;
};

#endif // GL_WIDGET_H
