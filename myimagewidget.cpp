#include "myimagewidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

MyImageWidget::MyImageWidget(QWidget *parent) : QWidget(parent)
{

}


void MyImageWidget::paintBitmap(cv::Mat ImIn)
{
    //QPainter painter(this);
    cvtColor(ImIn,ImIn,CV_BGR2RGB);
    Im = ImIn;
    //QImage QIm((unsigned char*)Im.data, Im.cols, Im.rows, QImage::Format_RGB888);

    //painter.drawImage(QIm.rect(), QIm);
}

void MyImageWidget::paintEvent(QPaintEvent *event)
{
   QPainter painter(this);

   QImage QIm((unsigned char*)Im.data, Im.cols, Im.rows, QImage::Format_RGB888);

   QRect rectangleToPaint = geometry();
   rectangleToPaint.setX(0);
   rectangleToPaint.setY(0);
   rectangleToPaint.setWidth(width());
   rectangleToPaint.setHeight(height());
   painter.drawImage(rectangleToPaint, QIm);
   //painter.drawImage(QIm.rect(), QIm);
   event->accept();
}

void MyImageWidget::mousePressEvent(QMouseEvent *event)
{
    const QPoint point = event->pos();
    const int butPressed = event->buttons();
    emit on_mousePressed(point,butPressed);
}

void MyImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint point = event->pos();
    const int butPressed = event->buttons();
    emit on_mouseMove(point, butPressed);
}
void MyImageWidget::keyPressEvent(QKeyEvent * event)
{
    const int keyPressed = event->key();
    emit on_KeyPressed(keyPressed);
}

