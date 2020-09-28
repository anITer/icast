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
#include "video_widget.h"
#include "ui_widget.h"
#include <QApplication>
#include <QPushButton>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QWidget window;
    Ui::Widget ui;
    ui.setupUi(&window);

    VideoWidget gl_widget;
    QVBoxLayout layout_video;
    layout_video.addWidget(&gl_widget);
    layout_video.setMargin(0);
    ui.frame_video->setLayout(&layout_video);

    QHBoxLayout layout_control;
    QPushButton button_start;
    button_start.setText("S");
    QPushButton button_join;
    button_start.setText("J");
    QPushButton button_setting;
    button_start.setText("C");
    layout_control.addWidget(&button_setting);
    layout_control.addWidget(&button_join);
    layout_control.addWidget(&button_start);
    ui.columnView_control->setLayout(&layout_control);

    window.show();

    return app.exec();
}
