/*#--------------------------------------------------------------*
#
#                 openGL Widget
#       for dominant colors in 3D color spaces
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#               v0.1 - 2019/10/24
#     - Lights
#     - Mouse control :
#         . zoom with wheel
#         . rotate view on x/y axes with left mouse button
#         . rotate view on z axis with CTRL + left mouse button
#         . move view on x/y axes with right mouse button
#         . sphere size with CTRL + wheel
#
# * QT signals sent when zoomed, moved, rotated, sphere size
#
# * Public access to zoom, position and rotation
#
#--------------------------------------------------------------*/

#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include "opencv2/opencv.hpp"
#include "dominant-colors.h"

class openGLWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit openGLWidget(QWidget *parent = 0);
    ~openGLWidget();

    struct_palette palettes[1000]; // 1000 is very large !
    int nb_palettes; // number of colors in palette

    int sphere_size; // size factor for spheres

    double xRot, yRot, zRot; // rotation values
    int xShift, yShift; // position values

    double zoom3D; // zoom coefficient

    bool axesEnabled; // draw 3D origin axes
    bool lightEnabled; // light
    bool qualityEnabled; // antialiasing

    QImage capture3D; // image of captured 3D scene

    std::string color_space; // color space to plot

    float size3d;

    void Capture(); // take a snapshot of rendered 3D scene
    void ConvertPalette();


protected:

    void initializeGL(); // launched when the widget is initialized
    void paintGL(); // 3D rendering
    void resizeGL(int width, int height); // called when the widget is resized
    void mousePressEvent(QMouseEvent *event); // save initial mouse position for move and rotate
    void mouseMoveEvent(QMouseEvent *event); // move and rotate view with mouse buttons
    void wheelEvent(QWheelEvent *event); // zoom


public slots:

    void SetXRotation(int angle); // rotate view
    void SetYRotation(int angle);
    void SetZRotation(int angle);
    void SetAngleXMinus();
    void SetAngleXPlus();
    void SetAngleYMinus();
    void SetAngleYPlus();
    void SetAngleZMinus();
    void SetAngleZPlus();

    void SetXShift(int value); // move view
    void SetYShift(int value);
    void SetShiftUp();
    void SetShiftDown();
    void SetShiftLeft();
    void SetShiftRight();

    void SetSphereSize(int size);


signals:

    void xRotationChanged(int angle); // rotation signals
    void yRotationChanged(int angle);
    void zRotationChanged(int angle);

    void xShiftChanged(int dec); // move signals
    void yShiftChanged(int dec);

    void zoomChanged(double zoom); // zoom signal
    void sphereSizeChanged(int size); // zoom signal

    void verticesChanged(int nb_Vertices); // number of vertices signal


private:

    QPoint lastPos; // save mouse position
};

#endif // OPENGLWIDGET_H

