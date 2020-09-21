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
#include "glwidget.h"
#include <time.h>
#include <QPainter>
#include <QTime>
#include <QKeyEvent>
#include "camera_device.h"
#include "screen_capturer.h"
#include "window_capturer.h"
#include "cursor_capturer.h"
#include "composite_capturer.h"

static const int GL_WIDTH_ALIGN_SIZE = 2;
static const float IDENTITY_MATRIX[16] = {
                    1.0, 0.0, 0.0, 0.0,
                    0.0, 1.0, 0.0, 0.0,
                    0.0, 0.0, 1.0, 0.0,
                    0.0, 0.0, 0.0, 1.0
};

static char* readString(const char* path)
{
    if (!path) return nullptr;

    FILE *fp = fopen(path, "r");
    if (!fp) return nullptr;

    fseek(fp, 0L, SEEK_END);
    long int len = ftell(fp);
    char* res = (char*) malloc(len + 1);
    memset(res, 0x0, len + 1);
    if (!res) return nullptr; // TODO::oom

    fseek(fp, 0L, SEEK_SET);
    fgets(res, len, fp);
    fclose(fp);

    return res;
}
static int get_gl_width(int width) {
    return (width + GL_WIDTH_ALIGN_SIZE - 1) / GL_WIDTH_ALIGN_SIZE * GL_WIDTH_ALIGN_SIZE;
}
GLWidget::GLWidget(QWidget *parent):
    QOpenGLWidget(parent)
{
    memcpy(mMvpMatrix, IDENTITY_MATRIX, 16 * sizeof(float));
    timer = new QTimer(this);
    fps = 0;
    connect(timer, &QTimer::timeout, [=](){
        update();
    });
    timer->start(1);

    winCapturer = new
     WindowCapturer();
//     ScreenCapturer();
//     CameraDevice();
    capDevice = new CompositeCapturer((ICaptureDevice*) winCapturer);
    capDevice->set_update_callback(this);
    selectDevice();
}
GLWidget::~GLWidget()
{
    delete timer;
    capDevice->stop_device();
    capDevice->unbind_device();
    delete capDevice;
    capDevice = nullptr;

    delete winCapturer;
    winCapturer = nullptr;
}
void GLWidget::selectDevice()
{
    const std::vector<DeviceInfo>& list = capDevice->enum_devices();
    for (DeviceInfo info : list) {
        printf("find device with info [%s | %dx%d]\n", info._name.c_str(), info._width, info._height);
    }
    capDevice->bind_device(0);
    DeviceInfo* info = capDevice->get_cur_device();
    texWidth = info->_width;
    texHeight = info->_height;
    curFormat = info->_format;
    isSizeChanged = true;
    capDevice->start_device();

    resize(texWidth, texHeight);
}
int GLWidget::on_device_updated()
{
    DeviceInfo* info = capDevice->get_cur_device();
    int w = info->_width;
    int h = info->_height;
    if (w != texWidth || h != texHeight) {
        isSizeChanged = true;
    }
    texWidth = w;
    texHeight = h;
    return 0;
}
void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    initTextures();
    initShaders();
}
void GLWidget::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program.programId());

    unsigned char* pixels = nullptr;
    if (capDevice->grab_frame(pixels) > 0) {
        // dump original pixel buffer
        // FILE* tmp = fopen("/tmp/test.raw", "w");
        // fwrite(pixels, width * height * 4, 1, tmp);
        // fclose(tmp);

        glBindTexture(GL_TEXTURE_2D, texture);
        // TODO:: check texture format
        if (curFormat == PIXEL_FORMAT_RGBA) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_BYTE, (uint8_t*) pixels);
        } else { // YUYV for camera
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth >> 1, texHeight, GL_RGBA, GL_UNSIGNED_BYTE, (uint8_t*) pixels);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    draw();

    // dump rendered rgba buffer
    // uint8_t color_buf[width * height * 4];
    // glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, color_buf);
    // FILE* tmp = fopen("/tmp/test.rgba", "w");
    // fwrite(color_buf, width * height * 4, 1, tmp);
    // fclose(tmp);

    glUseProgram(0);

    if (isSizeChanged) resetTextureSize();

    calcFPS();
    paintFPS();
}
void GLWidget::draw()
{
    //顶点
    glEnableVertexAttribArray(verticesHandle);
    glVertexAttribPointer(verticesHandle, 2, GL_FLOAT, 0, 0, vertices);
    //纹理坐标
    glEnableVertexAttribArray(texCoordHandle);
    glVertexAttribPointer(texCoordHandle, 2, GL_FLOAT, 0, 0, texcoords);
    //MVP矩阵
    glUniformMatrix4fv(mvpMatrixHandle, 1, GL_FALSE, mMvpMatrix);

    //纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(colorMapHandle, 0);

    glUniform1f(texWidthHandle, (float) (1.0 / texWidth));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(verticesHandle);
    glDisableVertexAttribArray(texCoordHandle);
}
void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    wndWidth = w;
    wndHeight = h;
    resetTextureSize();
}
void GLWidget::resetTextureSize()
{
    mMvpMatrix[5] = -1.0; // flip virtically
    mMvpMatrix[0] = 1.0; // flip horizontally

    float scale = (float) wndWidth / wndHeight / ((float) texWidth / texHeight);
    // scale to keep ratio
    if (scale < 1.0) {
        mMvpMatrix[5] *= scale; // scale witdh to fit ratio
    } else {
        mMvpMatrix[0] /= scale; // scale height to fit ratio
    }
    // crop to keep ratio
    // if (scale > 1.0) {
    //     mMvpMatrix[5] *= scale; // scale witdh to fit ratio
    // } else {
    //     mMvpMatrix[0] /= scale; // scale height to fit ratio
    // }
    initTextures();
}
void GLWidget::calcFPS()
{
    static QTime time;
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
void GLWidget::updateFPS(qreal v)
{
    fps = v;
}
void GLWidget::paintFPS()
{
    QPainter painter(this);
    painter.setPen(Qt::green);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.drawText(10, 10, QString("FPS:%1").arg(QString::number(fps, 'f', 3)));
}
void GLWidget::initShaders()
{
//    const char* vert_str = readString("/home/adams/Documents/workspace/libv4l2/example_qt/res/shader/vertex.vsh");
//    const char* frag_str = readString("/home/adams/Documents/workspace/libv4l2/example_qt/res/shader/yuyv_fragment.fsh");
//    GLuint prog = glCreateProgram();
//    GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
//    glShaderSource(vert_shader, 1, &vert_str, NULL);
//    glCompileShader(vert_shader);
//    GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
//    glShaderSource(frag_shader, 1, &frag_str, NULL);
//    glCompileShader(frag_shader);
//    glAttachShader(prog, vert_shader);
//    glAttachShader(prog, frag_shader);
//    glLinkProgram(prog);
//    glBindAttribLocation(prog, 0, "model_coords");
//    glBindAttribLocation(prog, 1, "tex_coords");
//    glUseProgram(prog); // FIXME:: 0x0502
//    int err = glGetError();
//    if (err != GL_NO_ERROR) {
//        qDebug() << __FILE__<<__FUNCTION__<< " using program failed.";
//        return;
//    }

    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/vertex.vsh")) {
        qDebug() << __FILE__<<__FUNCTION__<< " add vertex shader file failed.";
        return ;
    }
    // TODO:: check texture format
    QString frag_str = curFormat == PIXEL_FORMAT_RGBA ? ":/shader/rgba_fragment.fsh" : ":/shader/yuyv_fragment.fsh";
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, frag_str)) {
        qDebug() << __FILE__<<__FUNCTION__<< " add fragment shader file failed.";
        return ;
    }
    program.link();
    GLuint programId = program.programId();
    glUseProgram(programId);
    mvpMatrixHandle	= glGetUniformLocation(programId, "mvp_matrix");
    texWidthHandle     = glGetUniformLocation(programId, "tex_width");
    colorMapHandle     = glGetUniformLocation(programId, "color_map");
    verticesHandle		= glGetAttribLocation(programId, "model_coords");
    texCoordHandle		= glGetAttribLocation(programId, "tex_coords");

}
void GLWidget::initTextures()
{
    if (texture) {
        if (isSizeChanged) {
            glDeleteTextures(1, &texture);
        } else {
            return;
        }
    }
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, curFormat == PIXEL_FORMAT_RGBA ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, curFormat == PIXEL_FORMAT_RGBA ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // TODO:: check texture format
    if (curFormat == PIXEL_FORMAT_RGBA) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, get_gl_width(texWidth), texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    } else { // YUYV for camera
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, get_gl_width(texWidth) >> 1, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    }
    isSizeChanged = false;
}
