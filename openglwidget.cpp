/*#--------------------------------------------------------------*
#
#                 openGL Widget
#       for dominant colors in 3D color spaces
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#               v2 - 2019/11/08
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
# * Draw values in color spaces
#
#--------------------------------------------------------------*/

#include <QtWidgets>

#include <QtOpenGL>

#include "opencv2/opencv.hpp"
#include "openglwidget.h"
#include "mat-image-tools.h"
#include "opengl-draw.h"

using namespace cv;
using namespace std;

///////////////////////////////////////////////
//// Widget
///////////////////////////////////////////////

openGLWidget::openGLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

openGLWidget::~openGLWidget()
{

}

///////////////////////////////////////////////
//// Redefine widget main functions :
////    init, repaint, resize
///////////////////////////////////////////////

void openGLWidget::initializeGL() // launched when the widget is initialized
{
    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(Qt::black); // clear the screen with black color

    glEnable(GL_DEPTH_TEST); // z-sorting
    //glEnable(GL_DEPTH_CLAMP); // no clipping - it's a bit slower
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT); // all material has ambiant light
    glEnable(GL_COLOR_MATERIAL); // to see the two faces of a triangle, needed by lights when not shading
    glShadeModel(GL_SMOOTH); // blend colors - GL_FLAT chooses one color in the polygon

    glDisable(GL_CULL_FACE); // facet culling
    glEnable(GL_BLEND); // prefer using GLBlendFunc, used by the anaglyphic view

    xRot = 287; // initial values of rotation
    yRot = 0;
    zRot = 300;
    xShift = 0; // initial (x,y) position
    yShift = 0;
    zoom3D = 4; // zoom coefficient
    axesEnabled = true; // draw 3D origin axes enabled
    lightEnabled = false; // light disabled
    qualityEnabled = true; // antialiasing enabled

    //// Lights
    GLfloat light_position[] = { 0, 0, 10000, 1.0 };
    GLfloat light_ambient[]  = { 0.7, 0.7, 0.7, 1};
    GLfloat light_diffuse[]  = { 1, 1, 1, 0.5};
    glEnable(GL_LIGHTING); // enable lighting
    glEnable(GL_LIGHT0); // define the first light
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient); // ambient
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse); // diffuse

    // Initialize RGB space spheres for first display
    sphere_size = 30; // sphere size factor
    color_space = "RGB";
    size3d = 1000.0;

    nb_palettes = -1; // we are going to populate the palette with 512 values
    for (int i = 0; i < 9; i++) // 9 * 9 * 9 spĥeres
        for (int j = 0; j < 9; j++)
            for (int k = 0; k < 9; k++) {
                nb_palettes++; // palette index

                // RGB
                palettes[nb_palettes].RGB.R = i / 8.0; // RGB values
                palettes[nb_palettes].RGB.G = j / 8.0;
                palettes[nb_palettes].RGB.B = k / 8.0;
                palettes[nb_palettes].percentage = 1.0 / powl(9.0, 3.0); // percentage
                palettes[nb_palettes].selected = false; // color not selected
                palettes[nb_palettes].visible = true; // color shown
            }
    nb_palettes++; // adjust number of palettes that must be in [1..x]
    ConvertPaletteFromRGB(); // convert RGB to other values

    update(); // show new palette
}

void openGLWidget::paintGL() // 3D rendering
{
    // lighting
    if (lightEnabled) {
        glEnable(GL_LIGHTING); // turn on the lights...
    } else
        glDisable(GL_LIGHTING); // ... or not

    // quality
    if (qualityEnabled) { // antialiasing
        glShadeModel(GL_SMOOTH); // blend colors
        glEnable(GL_POINT_SMOOTH); // draw points with anti-aliasing
        glEnable(GL_LINE_SMOOTH); // draw lines with anti-aliasing
        glEnable(GL_POLYGON_SMOOTH); // draw polygons with anti-aliasing // can result in noise
        glEnable(GL_DITHER); //dither color components
        glEnable(GL_MULTISAMPLE); // use multiple fragment samples in computing the final color of a pixel
    }
    else { // no antialiasing
        glShadeModel(GL_FLAT); // don't blend colors
        glDisable(GL_POINT_SMOOTH); // draw aliased points
        glDisable(GL_LINE_SMOOTH); // draw aliased lines
        //glDisable(GL_POLYGON_SMOOTH); // draw aliased polygons
        glDisable(GL_DITHER); //dither color components
        glDisable(GL_MULTISAMPLE); // use multiple fragment samples in computing the final color of a pixel
    }

    //// init 3D view

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear color and depth buffers

    glLoadIdentity(); // replace current matrix with identity matrix (reset)

    glTranslatef(xShift, yShift, 0); // translation matrix for all objects - no z used

    glScaled(zoom3D, zoom3D, zoom3D); // scale objects with zoom factor

    glRotatef(xRot, 1.0, 0.0, 0.0); // rotate all objects
    glRotatef(yRot, 0.0, 1.0, 0.0);
    glRotatef(zRot, 0.0, 0.0, 1.0);

    //// Test text
    //DrawText("(c)2019 AbsurdePhoton", -1000.0f, 1000.0f, size3d + 300, 15, 1, 1, 1, 4);

    //// draw color space

    if (color_space == "RGB") { // RGB
        // green x axis
        glLineWidth(32); // bigger width of the lines to really see them
        glBegin(GL_LINES); // draw several lines
            glColor3d(0,0,0); // axis origin : black
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(0,1,0); // x/R axis color : red
            glVertex3f(size3d, 0.0f, 0.0f);
        glEnd();
        DrawConeX(size3d, 0.0f, 0.0f, 200.0f, 50.0f, 100, 0, 1, 0);
        DrawText("G", size3d + 300, -50.0f, 0.0f, 20, 0, 1, 0, 4);

        // red y axis
        glLineWidth(32); // bigger width of the lines to really see them
        glBegin(GL_LINES);
            glColor3d(0,0,0); // axis origin : black
            glVertex3f( 0.0f,     0.0f, 0.0f );
            glColor3d(1,0,0); // green
            glVertex3f( 0.0f, -size3d, 0.0f );
        glEnd();
        DrawConeY(0.0f, -size3d, 0.0f, -200.0f, 50.0f, 100, 1, 0, 0);
        DrawText("R", -50.0f, -size3d - 400, 0.0f, 20, 1, 0, 0, 4);

        // blue z axis
        glLineWidth(32); // bigger width of the lines to really see them
        glBegin(GL_LINES);
            glColor3d(0,0,0); // axis origin : black
            glVertex3f( 0.0f, 0.0f,    0.0f);
            glColor3d(0,0,1); // z axis color : blue
            glVertex3f( 0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 0, 0, 1);
        DrawText("B", -150.0f, 150.0f, size3d + 300, 20, 0, 0, 1, 4);

        // Triangle
        glLineWidth(4);
        glBegin(GL_LINE_LOOP); // draw closed lines
            glColor3d(1, 0, 0); // red
            glVertex3f(0, -size3d, 0); // vertex
            glColor3d(0, 0, 1); // blue
            glVertex3f(0, 0, size3d); // vertex
            glColor3d(0, 1, 0); // green
            glVertex3f(size3d, 0, 0); // vertex
        glEnd();

        /*// Rest of the cube
        glLineWidth(4);
        glBegin(GL_LINES);
            glColor3d(1,1,1); // axis origin : white
            glVertex3f(size3d, -size3d, size3d);
            glColor3d(1,0,1); // violet
            glVertex3f(size3d, 0.0f, size3d);
        glEnd();

        glBegin(GL_LINES);
            glColor3d(1,1,1); // axis origin : white
            glVertex3f(size3d, -size3d, size3d);
            glColor3d(1,1,0); // yellow
            glVertex3f(size3d, -size3d, 0.0f);
        glEnd();

        glBegin(GL_LINES);
            glColor3d(1,1,1); // axis origin : white
            glVertex3f(size3d, -size3d, size3d);
            glColor3d(0,1,1); // cyan
            glVertex3f(0.0f, -size3d, size3d);
        glEnd();

        glBegin(GL_LINES);
            glColor3d(0,1,1); // axis origin : cyan
            glVertex3f(0.0f, -size3d, size3d);
            glColor3d(0,1,0); // green
            glVertex3f(0.0f, -size3d, 0.0f);
        glEnd();

        glBegin(GL_LINES);
            glColor3d(0,1,0); // axis origin : green
            glVertex3f(0.0f, -size3d, 0.0f);
            glColor3d(1,1,0); // yellow
            glVertex3f(size3d, -size3d, 0.0f);
        glEnd();

        glBegin(GL_LINES);
            glColor3d(1,1,0); // axis origin : yellow
            glVertex3f(size3d, -size3d, 0.0f);
            glColor3d(1,0,0); // red
            glVertex3f(size3d, 0.0f, 0.0f);
        glEnd();

        glBegin(GL_LINES);
            glColor3d(1,0,1); // axis origin : violet
            glVertex3f(size3d, 0.0f, size3d);
            glColor3d(1,0,0); // red
            glVertex3f(size3d, 0.0f, 0.0f);
        glEnd();

        glBegin(GL_LINES);
            glColor3d(1,0,1); // axis origin : violet
            glVertex3f(size3d, 0.0f, size3d);
            glColor3d(0,0,1); // blue
            glVertex3f(0.0f, 0.0f, size3d);
        glEnd();

        glBegin(GL_LINES);
            glColor3d(0,0,1); // axis origin : blue
            glVertex3f(0.0f, 0.0f, size3d);
            glColor3d(0,1,1); // cyan
            glVertex3f(0.0f, -size3d, size3d);
        glEnd();*/

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].RGB.G * size3d,
                       palettes[n].RGB.R * size3d,
                       palettes[n].RGB.B * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G,palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
    }

    if (color_space == "RGB Triangle") { // RGB triangle
        // green x axis
        glLineWidth(32); // bigger width of the lines to really see them
        glBegin(GL_LINES); // draw several lines
            glColor3d(0,0,0); // axis origin : black
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(0,1,0); // x/R axis color : red
            glVertex3f(size3d, 0.0f, 0.0f);
        glEnd();
        DrawConeX(size3d, 0.0f, 0.0f, 200.0f, 50.0f, 100, 0, 1, 0);
        DrawText("G", size3d + 300, -50.0f, 0.0f, 20, 0, 1, 0, 4);

        // red y axis
        glLineWidth(32); // bigger width of the lines to really see them
        glBegin(GL_LINES);
            glColor3d(0,0,0); // axis origin : black
            glVertex3f( 0.0f,     0.0f, 0.0f );
            glColor3d(1,0,0); // green
            glVertex3f( 0.0f, -size3d, 0.0f );
        glEnd();
        DrawConeY(0.0f, -size3d, 0.0f, -200.0f, 50.0f, 100, 1, 0, 0);
        DrawText("R", -50.0f, -size3d - 400, 0.0f, 20, 1, 0, 0, 4);

        // blue z axis
        glLineWidth(32); // bigger width of the lines to really see them
        glBegin(GL_LINES);
            glColor3d(0,0,0); // axis origin : black
            glVertex3f( 0.0f, 0.0f,    0.0f);
            glColor3d(0,0,1); // z axis color : blue
            glVertex3f( 0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 0, 0, 1);
        DrawText("B", -150.0f, 150.0f, size3d + 300, 20, 0, 0, 1, 4);

        // Triangle
        glLineWidth(4);
        glBegin(GL_LINE_LOOP); // draw closed lines
            glColor3d(1, 0, 0); // red
            glVertex3f(0, -size3d, 0); // vertex
            glColor3d(0, 0, 1); // blue
            glVertex3f(0, 0, size3d); // vertex
            glColor3d(0, 1, 0); // green
            glVertex3f(size3d, 0, 0); // vertex
        glEnd();

        // values
        for (int n = 0; n < nb_palettes;n++) { // for each color in palette
            float sum = palettes[n].RGB.R + palettes[n].RGB.G + palettes[n].RGB.B;
            float r = palettes[n].RGB.R / sum;
            float g = palettes[n].RGB.G / sum;
            float b = palettes[n].RGB.B / sum;
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       g * size3d, r * size3d, b * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G,palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
        }
    }

    if (color_space == "HSV") { // HSV
        // colored circle
        int num_segments = 360;
        double R, G, B;
        glLineWidth(32);
        glBegin(GL_LINE_LOOP); // colored circle
            for (int i = 0; i < num_segments; i++) {
                double angle = double(i) / double(num_segments); //current angle
                HSVtoRGB(angle, 1, 1, R, G, B);
                glColor3d(R, G, B); // color of segment
                glVertex3f(size3d * cosl(-angle * 2.0f * Pi), -size3d * sin(-angle * 2.0f * Pi), size3d); // vertex
            }
        glEnd();

        // arc = H
        DrawCircleArcXY(0, 0, size3d + 300, size3d + 400, 360, 315, 360,  1, 1, 1, 32);
        DrawConeY(size3d + 400, -30, size3d + 300, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("H", size3d + 530, 0, size3d + 300, 20, 1, 1, 1, 4);

        /*// black circle
        DrawCircleXY(0, 0, 0, size3d, 360, 0, 0, 0, 32);*/

        // vertical axis black to white
        glLineWidth(32);
        glBegin(GL_LINES);
            glColor3d(0,0,0); // axis origin : black
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(1,1,1); // axis end : white
            glVertex3f(0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("V", -150.0f, 150.0f, size3d + 300, 20, 1, 1, 1, 4);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].HSV.S * cos(-palettes[n].HSV.H * 2 * Pi) * size3d,
                       palettes[n].HSV.S * sin(-palettes[n].HSV.H * 2 * Pi) * size3d,
                       palettes[n].HSV.V * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
    }

    if (color_space == "HSL") { // HSL
        // colored circle
        int num_segments = 360;
        double R, G, B;
        glLineWidth(32);
        glBegin(GL_LINE_LOOP); // colored circle
            for (int i = 0; i < num_segments; i++) {
                double theta = 2.0f * Pi * double(i) / double(num_segments); //current angle
                HSLtoRGB(-double(i) / double(num_segments), 1, 0.5, R, G, B);
                glColor3d(R, G, B); // color of segment
                glVertex3f(size3d * cosl(theta), -size3d * sin(theta), 500.0f); // vertex
            }
        glEnd();

        // arc = H
        DrawCircleArcXY(0, 0, size3d + 300, size3d + 400, 360, 315, 360,  1, 1, 1, 32);
        DrawConeY(size3d + 400, -30, size3d + 300, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("H", size3d + 530, 0, size3d + 300, 20, 1, 1, 1, 4);

        /*// black circle
        DrawCircleXY(0, 0, 0, size3d, 360, 0, 0, 0, 32);

        // white circle
        DrawCircleXY(0, 0, size3d, size3d, 360, 1, 1, 1, 32);*/

        // vertical axis black to white
        glLineWidth(32);
        glBegin(GL_LINES);
            glColor3d(0,0,0); // axis origin : black
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(1,1,1); // axis end : white
            glVertex3f(0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("L", -150.0f, 150.0f, size3d + 300, 20, 1, 1, 1, 4);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].HSL.S * cos(-palettes[n].HSL.H * 2 * Pi) * size3d,
                       palettes[n].HSL.S * sin(-palettes[n].HSL.H * 2 * Pi) * size3d,
                       palettes[n].HSL.L * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
    }

    if (color_space == "HWB") { // HWB
        // colored circle
        int num_segments = 360;
        double R, G, B;
        glLineWidth(32); // bigger width of the lines to really see them
        glBegin(GL_LINE_LOOP);       
            for (int i = 0; i < num_segments; i++) {
                double theta = 2.0f * Pi * double(i) / double(num_segments); //current angle
                HWBtoRGB(double(i) / double(num_segments), 0, 0, R, G, B);
                glColor3d(R, G, B); // color of segment
                glVertex3f(size3d * cosl(theta), -size3d * sin(theta), 0.0f); // vertex
            }
        glEnd();

        // arc = H
        DrawCircleArcXY(0, 0, size3d + 300, size3d + 400, 360, 0, 45,  1, 1, 1, 32);
        DrawConeY(size3d + 400, 0, size3d + 300, -200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("H", size3d + 530, 0, size3d + 300, 20, 1, 1, 1, 4);

        /*// black circle
        DrawCircleXY(0, 0, size3d, size3d, 360, 0, 0, 0, 32);*/

        // vertical axis white to black
        glLineWidth(32);
        glBegin(GL_LINES); // vertical axis
            glColor3d(0.75,0.75,0.75); // axis origin : white
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(0,0,0); // axis end : black
            glVertex3f(0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 0, 0, 0);
        DrawText("B", -150.0f, 150.0f, size3d + 300, 20, 0.15, 0.15, 0.15, 4);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       (1 - palettes[n].HWB.W) * cos(palettes[n].HWB.H * 2 * Pi) * size3d,
                       (1 - palettes[n].HWB.W) * sin(palettes[n].HWB.H * 2 * Pi) * size3d,
                       palettes[n].HWB.B * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
    }

    if (color_space == "HCV") { // HCV
        // colored circle
        int num_segments = 360;
        double R, G, B;
        glLineWidth(32); // bigger width of the lines to really see them
        glBegin(GL_LINE_LOOP); // colored circle   
            for (int i = 0; i < num_segments; i++) {
                double angle = double(i) / double(num_segments); //current angle
                HSVtoRGB(angle, 1, 1, R, G, B);
                glColor3d(R, G, B); // color of segment
                glVertex3f(size3d * cosl(-angle * 2.0f * Pi), -size3d * sin(-angle * 2.0f * Pi), size3d); // vertex
            }
        glEnd();

        // arc = H
        DrawCircleArcXY(0, 0, size3d + 300, size3d + 400, 360, 315, 360,  1, 1, 1, 32);
        DrawConeY(size3d + 400, -30, size3d + 300, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("H", size3d + 530, 0, size3d + 300, 20, 1, 1, 1, 4);

        // vertical axis black to white
        glLineWidth(32);
        glBegin(GL_LINES); // vertical axis
            glColor3d(0,0,0); // axis origin : black
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(1,1,1); // axis end : white
            glVertex3f(0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("V", -150.0f, 150.0f, size3d + 300, 20, 1, 1, 1, 4);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].HSV.C * cos(-palettes[n].HSV.H * 2 * Pi) * size3d,
                       palettes[n].HSV.C * sin(-palettes[n].HSV.H * 2 * Pi) * size3d,
                       palettes[n].HSV.V * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
    }

    if (color_space == "HCL") { // HCL
        // colored circle
        int num_segments = 360;
        double R, G, B;
        glLineWidth(32);
        glBegin(GL_LINE_LOOP); // colored circle
            for (int i = 0; i < num_segments; i++) {
                double theta = 2.0f * Pi * double(i) / double(num_segments); //current angle
                HSLtoRGB(-double(i) / double(num_segments), 1, 0.5, R, G, B);
                glColor3d(R, G, B); // color of segment
                glVertex3f(size3d * cosl(theta), -size3d * sin(theta), 500.0f); // vertex
            }
        glEnd();

        // arc = H
        DrawCircleArcXY(0, 0, size3d + 300, size3d + 400, 360, 315, 360,  1, 1, 1, 32);
        DrawConeY(size3d + 400, -30, size3d + 300, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("H", size3d + 530, 0, size3d + 300, 20, 1, 1, 1, 4);

        // vertical axis black to white
        glLineWidth(32);
        glBegin(GL_LINES); // vertical axis
            glColor3d(0,0,0); // axis origin : black
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(1,1,1); // axis end : white
            glVertex3f(0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("L", -150.0f, 150.0f, size3d + 300, 20, 1, 1, 1, 4);
        //DrawConeZ(0.0f, 0.0f, 0.0f, -200.0f, 50.0f, 100, 0, 0, 0);
        //DrawText("-L", -150.0f, 150.0f, 0 - 300, 20, 0.15, 0.15, 0.15, 4);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].HSL.C * cos(-palettes[n].HSL.H * 2 * Pi) * size3d,
                       palettes[n].HSL.C * sin(-palettes[n].HSL.H * 2 * Pi) * size3d,
                       palettes[n].HSL.L * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
    }

    if (color_space == "CIE XYZ") { // CIE XYZ
        // colored boundaries
        DrawCMFinXYZ(size3d, false);

        // x axis
        glLineWidth(32);
        glBegin(GL_LINES); // draw several lines
            glColor3d(1,1,1);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(size3d, 0.0f, 0.0f);
        glEnd();
        DrawConeX(size3d, 0.0f, 0.0f, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("X", size3d + 300, -50.0f, 0.0f, 20, 1, 1, 1, 4);

        // y axis
        glLineWidth(32);
        glBegin(GL_LINES);
            glVertex3f( 0.0f,     0.0f, 0.0f );
            glVertex3f( 0.0f, -size3d, 0.0f );
        glEnd();
        DrawConeY(0.0f, -size3d, 0.0f, -200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("Y", -50.0f, -size3d - 400, 0.0f, 20, 1, 1, 1, 4);

        // z axis
        glLineWidth(32);
        glBegin(GL_LINES);
            glColor3d(0,0,0);
            glVertex3f( 0.0f, 0.0f,    0.0f);
            glColor3d(1,1,1);
            glVertex3f( 0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("Z", -150.0f, 150.0f, size3d + 300, 20, 1, 1, 1, 4);

        // values
        for (int n = 0; n < nb_palettes;n++) { // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].XYZ.X * size3d, palettes[n].XYZ.Y * size3d, palettes[n].XYZ.Z * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
        }
    }

    if (color_space == "LMS") { // LMS
        // x axis
        glLineWidth(32);
        glBegin(GL_LINES); // draw several lines
            glColor3d(1,1,1);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(size3d, 0.0f, 0.0f);
        glEnd();
        DrawConeX(size3d, 0.0f, 0.0f, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("L", size3d + 300, -50.0f, 0.0f, 20, 1, 1, 1, 4);

        // y axis
        glLineWidth(32);
        glBegin(GL_LINES);
            glVertex3f( 0.0f,     0.0f, 0.0f );
            glVertex3f( 0.0f, -size3d, 0.0f );
        glEnd();
        DrawConeY(0.0f, -size3d, 0.0f, -200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("M", -50.0f, -size3d - 400, 0.0f, 20, 1, 1, 1, 4);

        // z axis
        glLineWidth(32);
        glBegin(GL_LINES);
            glVertex3f( 0.0f, 0.0f,    0.0f);
            glVertex3f( 0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("S", -150.0f, 150.0f, size3d + 300, 20, 1, 1, 1, 4);

        // colored boundaries
        DrawCMFinLMS(size3d);

        // values
        for (int n = 0; n < nb_palettes;n++) { // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].LMS.L * size3d, palettes[n].LMS.M * size3d, palettes[n].LMS.S * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
        }
    }

    if (color_space == "CIE xyY") { // CIE xyY
        // colored boundaries
        DrawCMFinXYZ(size3d, true);

        // White point
        glLineWidth(4);
        glBegin(GL_LINES); // draw several lines
            glColor3d(1,1,1);
            glVertex3f(0,0,0);
            float sum = 0.9505f + 1.0f + 1.089f;
            glVertex3f(1.0f / sum * size3d, -0.9505f / sum * size3d, 1.089f / sum * size3d);
        glEnd();

        // y axis
        glLineWidth(32);
        glBegin(GL_LINES); // draw several lines
            glColor3d(1,1,1);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(size3d, 0.0f, 0.0f);
        glEnd();
        DrawConeX(size3d, 0.0f, 0.0f, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("y", size3d + 300, -50.0f, 0.0f, 20, 1, 1, 1, 4);

        // x axis
        glLineWidth(32);
        glBegin(GL_LINES);
            glVertex3f( 0.0f,     0.0f, 0.0f );
            glVertex3f( 0.0f, -size3d, 0.0f );
        glEnd();
        DrawConeY(0.0f, -size3d, 0.0f, -200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("x", -50.0f, -size3d - 400, 0.0f, 20, 1, 1, 1, 4);

        // z axis
        glLineWidth(32);
        glBegin(GL_LINES);
            glVertex3f( 0.0f, 0.0f,    0.0f);
            glVertex3f( 0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("Y", -150.0f, 150.0f, size3d + 300, 20, 1, 1, 1, 4);

        // values
        for (int n = 0; n < nb_palettes;n++) { // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].XYY.y * size3d, palettes[n].XYY.x * size3d, (1 - palettes[n].XYY.x - palettes[n].XYY.y) * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
        }
    }

    if (color_space == "CIE L*u*v*") { // CIE L*u*v*
        // x axis
        glLineWidth(32);
        glBegin(GL_LINES); // draw several lines
            glColor3d(1,1,1);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(size3d, 0.0f, 0.0f);
        glEnd();
        DrawConeX(size3d, 0.0f, 0.0f, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("v", size3d + 300, -50.0f, 0.0f, 20, 1, 1, 1, 4);

        // y axis
        glLineWidth(32);
        glBegin(GL_LINES);
            glVertex3f( 0.0f,     0.0f, 0.0f );
            glVertex3f( 0.0f, -size3d, 0.0f );
        glEnd();
        DrawConeY(0.0f, -size3d, 0.0f, -200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("u", -50.0f, -size3d - 400, 0.0f, 20, 1, 1, 1, 4);

        // z axis
        glLineWidth(32);
        glBegin(GL_LINES);
            glColor3d(0,0,0);
            glVertex3f( 0.0f, 0.0f,    0.0f);
            glColor3d(1,1,1);
            glVertex3f( 0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("L", -150.0f, 150.0f, size3d + 300, 20, 1, 1, 1, 4);

        // color boundaries
        DrawCMFinLuv(size3d);

        // values
        for (int n = 0; n < nb_palettes;n++) { // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].LUV.v * size3d, palettes[n].LUV.u * size3d, palettes[n].LUV.L * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
        }
    }

    if (color_space == "CIE L*a*b*") { // LAB
        // vertical L axis
        glLineWidth(32);
        glBegin(GL_LINES); // vertical axis
            glColor3d(0,0,0); // axis origin : black
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(1,1,1); // axis end : white
            glVertex3f(0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("L", -150.0f, 150.0f, size3d + 300, 15, 1, 1, 1, 4);

        // a axis (x) : green (-) to red (+)
        glLineWidth(32);
        glBegin(GL_LINES); // draw several lines
            glColor3d(1,0,0); // red
            glVertex3f(-size3d, 0.0f, 0.0f);
            glColor3d(0,1,0); // green
            glVertex3f(size3d, 0.0f, 0.0f);
        glEnd();
        DrawConeX(size3d, 0.0f, 0.0f, 200.0f, 50.0f, 100, 0, 1, 0); // green arrow
        DrawText("-a", size3d + 250, -100.0f, 0.0f, 15, 0, 1, 0, 4);
        DrawConeX(-size3d, 0.0f, 0.0f, -200.0f, 50.0f, 100, 1, 0, 0); // red arrow
        DrawText("+a", -size3d - 400, -100.0f, 0.0f, 15, 1, 0, 0, 4);

        // b axis (y) : blue (-) to yellow (+)
        glLineWidth(32);
        glBegin(GL_LINES);
            glColor3d(0,0,1); // blue
            glVertex3f( 0.0f, size3d, 0.0f );
            glColor3d(1,1,0); // yellow
            glVertex3f( 0.0f, -size3d, 0.0f );
        glEnd();
        DrawConeY(0.0f, -size3d, 0.0f, -200.0f, 50.0f, 100, 1, 1, 0); // yellow arrow
        DrawText("+b", -60.0f, -size3d - 450, 0.0f, 15, 1, 1, 0, 4);
        DrawConeY(0.0f, size3d, 0.0f, 200.0f, 50.0f, 100, 0, 0, 1); // blue arrow
        DrawText("-b", -60.0f, size3d + 250, 0.0f, 15, 0, 0, 1, 4);

        // color boundaries
        DrawCMFinLab(size3d);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       -palettes[n].CIELAB.A * size3d,
                       palettes[n].CIELAB.B * size3d,
                       palettes[n].CIELAB.L * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
    }

    if (color_space == "Hunter Lab") { // Hunter LAB
        // vertical L axis
        glLineWidth(32);
        glBegin(GL_LINES); // vertical axis
            glColor3d(0,0,0); // axis origin : black
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(1,1,1); // axis end : white
            glVertex3f(0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawText("L", -150.0f, 150.0f, size3d + 300, 15, 1, 1, 1, 4);

        // a axis (x) : green (-) to red (+)
        glLineWidth(32);
        glBegin(GL_LINES); // draw several lines
            glColor3d(1,0,0); // red
            glVertex3f(-size3d, 0.0f, 0.0f);
            glColor3d(0,1,0); // green
            glVertex3f(size3d, 0.0f, 0.0f);
        glEnd();
        DrawConeX(size3d, 0.0f, 0.0f, 200.0f, 50.0f, 100, 0, 1, 0); // green arrow
        DrawText("-a", size3d + 250, -100.0f, 0.0f, 15, 0, 1, 0, 4);
        DrawConeX(-size3d, 0.0f, 0.0f, -200.0f, 50.0f, 100, 1, 0, 0); // red arrow
        DrawText("+a", -size3d - 400, -100.0f, 0.0f, 15, 1, 0, 0, 4);

        // b axis (y) : blue (-) to yellow (+)
        glLineWidth(32);
        glBegin(GL_LINES);
            glColor3d(0,0,1); // blue
            glVertex3f( 0.0f, size3d, 0.0f );
            glColor3d(1,1,0); // yellow
            glVertex3f( 0.0f, -size3d, 0.0f );
        glEnd();
        DrawConeY(0.0f, -size3d, 0.0f, -200.0f, 50.0f, 100, 1, 1, 0); // yellow arrow
        DrawText("+b", -60.0f, -size3d - 450, 0.0f, 15, 1, 1, 0, 4);
        DrawConeY(0.0f, size3d, 0.0f, 200.0f, 50.0f, 100, 0, 0, 1); // blue arrow
        DrawText("-b", -60.0f, size3d + 250, 0.0f, 15, 0, 0, 1, 4);

        // color boundaries
        DrawCMFinHLAB(size3d);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       -palettes[n].HLAB.A * size3d,
                        palettes[n].HLAB.B * size3d,
                        palettes[n].HLAB.L * size3d,
                        palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B,
                        palettes[n].selected, palettes[n].visible);
    }

    if (color_space == "CIE LCHab") { // CIE LCHab
        // vertical L axis
        glLineWidth(32);
        glBegin(GL_LINES); // vertical axis
            glColor3d(0,0,0); // axis origin : black
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(1,1,1); // axis end : white
            glVertex3f(0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);

        // a axis (x) : green (-) to red (+)
        glLineWidth(32);
        glBegin(GL_LINES); // draw several lines
            glColor3d(1,0,0); // red
            glVertex3f(-size3d, 0.0f, 0.0f);
            glColor3d(0,1,0); // green
            glVertex3f(size3d, 0.0f, 0.0f);
        glEnd();
        DrawConeX(size3d, 0.0f, 0.0f, 200.0f, 50.0f, 100, 0, 1, 0); // green arrow
        DrawConeX(-size3d, 0.0f, 0.0f, -200.0f, 50.0f, 100, 1, 0, 0); // red arrow

        // b axis (y) : blue (-) to yellow (+)
        glLineWidth(32);
        glBegin(GL_LINES);
            glColor3d(0,0,1); // blue
            glVertex3f( 0.0f, size3d, 0.0f );
            glColor3d(1,1,0); // yellow
            glVertex3f( 0.0f, -size3d, 0.0f );
        glEnd();
        DrawConeY(0.0f, -size3d, 0.0f, -200.0f, 50.0f, 100, 1, 1, 0); // yellow arrow
        DrawConeY(0.0f, size3d, 0.0f, 200.0f, 50.0f, 100, 0, 0, 1); // blue arrow

        // color boundaries
        DrawCMFinLab(size3d);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       -palettes[n].LCHAB.C / 127.0 * 100.0 * cos(palettes[n].LCHAB.H * 2.0f * Pi) * size3d,
                       palettes[n].LCHAB.C / 127.0 * 100.0 * sin(palettes[n].LCHAB.H * 2.0f * Pi) * size3d,
                       palettes[n].LCHAB.L * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
    }

    if (color_space == "CIE LCHuv") { // CIE LCHuv
        // x axis
        glLineWidth(32);
        glBegin(GL_LINES); // draw several lines
            glColor3d(1,1,1);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(size3d, 0.0f, 0.0f);
        glEnd();
        DrawConeX(size3d, 0.0f, 0.0f, 200.0f, 50.0f, 100, 1, 1, 1);

        // y axis
        glLineWidth(32);
        glBegin(GL_LINES);
            glVertex3f( 0.0f,     0.0f, 0.0f );
            glVertex3f( 0.0f, -size3d, 0.0f );
        glEnd();
        DrawConeY(0.0f, -size3d, 0.0f, -200.0f, 50.0f, 100, 1, 1, 1);

        // z axis
        glLineWidth(32);
        glBegin(GL_LINES);
            glColor3d(0,0,0);
            glVertex3f( 0.0f, 0.0f,    0.0f);
            glColor3d(1,1,1);
            glVertex3f( 0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);

        // color boundaries
        DrawCMFinLuv(size3d);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       -palettes[n].LCHUV.C * cos(palettes[n].LCHUV.H * 2.0f * Pi + Pi / 2.0f) * size3d,
                       palettes[n].LCHUV.C * sin(palettes[n].LCHUV.H * 2.0f * Pi + Pi / 2.0f) * size3d,
                       palettes[n].LCHUV.L * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
    }

    if (color_space == "Wheel") { // Color Wheel
        // external circle
        float radius = size3d + 150.0f;
        DrawCircleXY(0, 0, 0, radius, 100, 1, 1, 1, 32);

        // internal circle
        DrawCircleXY(0, 0, 0, size3d, 100, 0.5, 0.5, 0.5, 4);

        // cross = middle
        glLineWidth(4);
        glColor3d(0.5,0.5,0.5); // gray
        glBegin(GL_LINES); // a white line on each axis
            glVertex3f( 50.0f, 0.0f, 0.0f);
            glVertex3f(-50.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, -50.0f, 0.0f );
            glVertex3f(0.0f,  50.0f, 0.0f );
            glVertex3f(0.0f, 0.0f,  50.0f );
            glVertex3f(0.0f, 0.0f, -50.0f );
        glEnd();

        // primary, secondary and tertiary colors : a sphere with a white circle
        // red
        float angle = 0.0f;
        DrawSpherePlus(3, 100, radius * cos(angle), -radius * sin(angle), 0, 1, 0, 0, true, true);
        // blue
        angle = 120.0f / 360.0f * 2 * Pi;
        DrawSpherePlus(3, 100, radius * cos(angle), -radius * sin(angle), 0, 0, 0, 1, true, true);
        // green
        angle = 240.0f / 360.0f * 2 * Pi;
        DrawSpherePlus(3, 100, radius * cos(angle), -radius * sin(angle), 0, 0, 1, 0, true, true);
        // magenta
        angle = 60.0f / 360.0f * 2 * Pi;
        DrawSpherePlus(3, 75, radius * cos(angle), -radius * sin(angle), 0, 1, 0, 1, true, true);
        // cyan
        angle = 180.0f / 360.0f * 2 * Pi;
        DrawSpherePlus(3, 75, radius * cos(angle), -radius * sin(angle), 0, 0, 1, 1, true, true);
        // yellow
        angle = 300.0f / 360.0f * 2 * Pi;
        DrawSpherePlus(3, 75, radius * cos(angle), -radius * sin(angle), 0, 1, 1, 0, true, true);
        // pink
        angle = 30.0f / 360.0f * 2 * Pi;
        DrawSpherePlus(3, 50, radius * cos(angle), -radius * sin(angle), 0, 1, 0, 0.5, true, true);
        // purple
        angle = 90.0f / 360.0f * 2 * Pi;
        DrawSpherePlus(3, 50, radius * cos(angle), -radius * sin(angle), 0, 0.5, 0, 1, true, true);
        // azure
        angle = 150.0f / 360.0f * 2 * Pi;
        DrawSpherePlus(3, 50, radius * cos(angle), -radius * sin(angle), 0, 0, 0.5, 1, true, true);
        // blue-green
        angle = 210.0f / 360.0f * 2 * Pi;
        DrawSpherePlus(3, 50, radius * cos(angle), -radius * sin(angle), 0, 0, 1, 0.5, true, true);
        // chartreuse
        angle = 270.0f / 360.0f * 2 * Pi;
        DrawSpherePlus(3, 50, radius * cos(angle), -radius * sin(angle), 0, 0.5, 1, 0, true, true);
        // orange
        angle = 330.0f / 360.0f * 2 * Pi;
        DrawSpherePlus(3, 50, radius * cos(angle), -radius * sin(angle), 0, 1, 0.5, 0, true, true);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSpherePlus(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].HSL.L * cos(palettes[n].HSL.H * 2 * Pi) * size3d,
                       palettes[n].HSL.L * sin(palettes[n].HSL.H * 2 * Pi) * size3d,
                       0,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B,
                       palettes[n].selected, palettes[n].visible);
    }
}

void openGLWidget::resizeGL(int width, int height) // called when the widget is resized
{
    double ratio = double(width) / height; // ratio of the widget : width / height
    glViewport(0, 0, width, height); // resize openGL viewport

    glMatrixMode(GL_PROJECTION); // openGL projection mode
    glLoadIdentity();
#ifdef QT_OPENGL_ES_1 // older versions of openGL
    glOrthof(-4 * 2048, +4 * 2048, -4 * 2048 / ratio, +4 * 2048  / ratio, -5000*2048, 5000*2048); // define view rectangle and clipping
#else
    glOrtho(-4 * 2048, +4 * 2048, -4 * 2048 / ratio, +4 * 2048 / ratio, -5000*2048, 5000*2048);
#endif
    glMatrixMode(GL_MODELVIEW); // now openGL model mode
}

void openGLWidget::ConvertPaletteFromRGB() // convert entire palette values in color spaces from RGB values
{
    for (int n = 0; n < nb_palettes; n++) {
        // hexadecimal
        QString hex = QString(" %1").arg(((int(round(palettes[n].RGB.R * 255.0)) & 0xff) << 16)
                                       + ((int(round(palettes[n].RGB.G * 255.0)) & 0xff) << 8)
                                       +  (int(round(palettes[n].RGB.B * 255.0)) & 0xff)
                                         , 6, 16, QChar('0')).trimmed(); // compute hexa RGB value
        hex = "#" + hex;
        palettes[n].hexa = hex.toUpper().toUtf8().constData();

        // HSV
        double H, S, V, C; // HSLVC values
        RGBtoHSV(palettes[n].RGB.R,
                 palettes[n].RGB.G,
                 palettes[n].RGB.B,
                 H, S, V, C); // convert RGB to HSV values
        palettes[n].HSV.H = H;
        palettes[n].HSV.C = C;
        palettes[n].HSV.S = S;
        palettes[n].HSV.V = V;

        // HWB
        double h, W, B; // HWB values
        HSVtoHWB(H, S, V, h, W, B);
        palettes[n].HWB.H = h;
        palettes[n].HWB.W = W;
        palettes[n].HWB.B = B;

        // HSL
        double L;
        RGBtoHSL(palettes[n].RGB.R,
                 palettes[n].RGB.G,
                 palettes[n].RGB.B,
                 H, S, L, C); // convert RGB to HSV values
        palettes[n].HSL.H = H;
        palettes[n].HSL.C = C;
        palettes[n].HSL.S = S;
        palettes[n].HSL.L = L;

        // XYZ
        double X, Y, Z; // XYZ values
        RGBtoXYZ(palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B, X, Y, Z); // convert RGB to XYZ values
        palettes[n].XYZ.X = X;
        palettes[n].XYZ.Y = Y;
        palettes[n].XYZ.Z = Z;

        // xyY
        double x, y;
        XYZtoxyY(X, Y, Z, x, y);
        palettes[n].XYY.x = x;
        palettes[n].XYY.y = y;
        palettes[n].XYY.Y = Y;

        // L*u*v*
        double u, v;
        XYZtoLuv(X, Y, Z, L, u, v);
        palettes[n].LUV.L = L;
        palettes[n].LUV.u = u;
        palettes[n].LUV.v = v;

        // LCHuv
        LUVtoLCHuv(u, v, C, H); // convert LUV to LCHuv values
        palettes[n].LCHUV.L = L;
        palettes[n].LCHUV.C = C;
        palettes[n].LCHUV.H = H;

        // L*A*B*
        double A;
        XYZtoLAB(X, Y, Z, L, A, B); // convert XYZ to LAB values
        palettes[n].CIELAB.L = L;std::string hexa;
        palettes[n].CIELAB.A = A;
        palettes[n].CIELAB.B = B;

        // LCHab
        LABtoLCHab(A, B, C, H); // convert LAB to LCHab values
        palettes[n].LCHAB.L = L;
        palettes[n].LCHAB.C = C;
        palettes[n].LCHAB.H = H;

        // Hunter LAB
        XYZtoHLAB(X, Y, Z, L, A, B); // convert XYZ to Hunter LAB values
        palettes[n].HLAB.L = L;
        palettes[n].HLAB.A = A;
        palettes[n].HLAB.B = B;

        // LMS
        double M;
        XYZtoLMS(X, Y, Z, L, M, S); // convert XYZ to LMS values
        palettes[n].LMS.L = L;
        palettes[n].LMS.M = M;
        palettes[n].LMS.S = S;

        // CMYK
        double K;
        RGBtoCMYK(palettes[n].RGB.R,
                  palettes[n].RGB.G,
                  palettes[n].RGB.B,
                  C, M, Y, K); // convert RGB to CMYK values
        palettes[n].CMYK.C = C;
        palettes[n].CMYK.M = M;
        palettes[n].CMYK.Y = Y;
        palettes[n].CMYK.K = K;
    }
}

void openGLWidget::DrawSpherePlus(const int &ndiv, const float &radius, const float &x, const float y, const float z, const float r, const float g, const float b, const bool circle, const bool visible) // draw a sphere with a white circle if color chosen
{
    if (!visible)
        return;

    DrawSphere(ndiv, radius, x, y, z, r, g, b); // first draw the sphere
    if (circle) // draw white circle around the sphere ?
        DrawCircleXY(x, -y, z, radius + 4.0f, 100, 1, 1, 1, 4); // draw the white circle around the sphere
}

///////////////////////////////////////////////
//// Drag with mouse + zoom with mouse wheel
////    + emit signals to get new values
///////////////////////////////////////////////

void openGLWidget::SetSphereSize(int size) // sphere size factor
{
    sphere_size = size;
    update();
}

static void NormalizeAngle(int &angle) // angle must be between 0 and 359°
{
    while (angle < 0) angle = 360 + angle; // no negative values
    angle = angle % 360; // 0 -> 359°
}

void openGLWidget::SetXRotation(int angle) // set X rotation and emit a signal
{
    NormalizeAngle(angle); // angle must be between 0 and 359°
    xRot = angle;
    emit xRotationChanged(angle); // emit signal
    update(); // update 3D rendering
}

void openGLWidget::SetYRotation(int angle) // set Y rotation and emit a signal
{
    NormalizeAngle(angle); // angle must be between 0 and 359°
    yRot = angle;
    emit yRotationChanged(angle); // emit signal
    update(); // update 3D rendering
}

void openGLWidget::SetZRotation(int angle) // set Z rotation and emit a signal
{
    NormalizeAngle(angle); // angle must be between 0 and 359°
    zRot = angle;
    emit zRotationChanged(angle); // emit signal
    update(); // update 3D rendering
}

void openGLWidget::SetAngleXMinus() // for keyboard control of x and y angles
{
    SetXRotation(xRot - 10);
}

void openGLWidget::SetAngleXPlus()
{
    SetXRotation(xRot + 10);
}

void openGLWidget::SetAngleYMinus()
{
    SetYRotation(yRot - 10);
}

void openGLWidget::SetAngleYPlus()
{
    SetYRotation(yRot + 10);
}

void openGLWidget::SetAngleZMinus()
{
    SetZRotation(zRot - 10);
}

void openGLWidget::SetAngleZPlus()
{
    SetZRotation(zRot + 10);
}

void openGLWidget::SetXShift(int value) // move view (x)
{
    xShift = value;
    emit xShiftChanged(xShift); // emit signal
    update(); // update 3D rendering
}

void openGLWidget::SetYShift(int value) // move view (y)
{
    yShift = value;
    emit yShiftChanged(yShift); // emit signal
    update(); // update 3D rendering
}

void openGLWidget::SetShiftUp() // for keyboard control of x and y positions
{
    SetYShift(yShift + 500);
}

void openGLWidget::SetShiftDown()
{
    SetYShift(yShift - 500);
}

void openGLWidget::SetShiftLeft()
{
    SetXShift(xShift - 500);
}

void openGLWidget::SetShiftRight()
{
    SetXShift(xShift + 500);
}

void openGLWidget::mousePressEvent(QMouseEvent *event) // save initial mouse position for move and rotate
{
    lastPos = event->pos(); // save initial position of the mouse
}

void openGLWidget::mouseMoveEvent(QMouseEvent *event) // move and rotate view with mouse buttons
{
    int dx = event->x() - lastPos.x(); // how much the mouse has moved in pixels
    int dy = event->y() - lastPos.y();

    bool key_control = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier); // modifier keys pressed ? (shift control etc)

    if ((event->buttons() & Qt::LeftButton) & (key_control)) { // left button = rotate on x and y axes
        SetZRotation(zRot + dx); // z = azimuth
    } else if (event->buttons() & Qt::LeftButton) { // left button = rotate on x and y axes
        SetXRotation(xRot + dy); // x = vertical
        SetYRotation(yRot + dx); // y = horizontal
    } else if (event->buttons() & Qt::RightButton) { // right button = move the view on x and y axes
        //setXRotation(xRot + 8 * dy);
        //setZRotation(zRot + 8 * dx);
        SetXShift(xShift + dx * 48);
        SetYShift(yShift - dy * 48);

        update(); // redraw 3d scene
    }

    lastPos = event->pos(); // save mouse position again
}

void openGLWidget::wheelEvent(QWheelEvent *event) // zoom
{
    bool key_control = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier); // modifier keys pressed ? (shift control etc)

    int n = event->delta(); // amount of wheel turn
    //zoom3D += n / 120 / 2; // should work with this (standard) value

    if (key_control) {
        if (n < 0) { // sphere size decrease
            sphere_size--;
            if (sphere_size < 1)
                sphere_size = 1;
            emit sphereSizeChanged(sphere_size); // emit signal
        }
        else { // sphere size increase
            sphere_size++;
            emit sphereSizeChanged(sphere_size); // emit signal
        }

    }
    else {
        if (n < 0) // zoom out
            zoom3D = zoom3D / 1.25;
        else // zoom in
            zoom3D = zoom3D * 1.25;

        emit zoomChanged(zoom3D); // emit signal
    }

    update(); // redraw 3d scene
}

///////////////////////////////////////////////
//// Capture 3D scene to QImage
///////////////////////////////////////////////

void openGLWidget::Capture() // take a snapshot of rendered 3D scene
{
    capture3D = grabFramebuffer(); // slow because it relies on glReadPixels() to read back the pixels
}
