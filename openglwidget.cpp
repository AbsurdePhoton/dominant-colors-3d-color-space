/*#--------------------------------------------------------------*
#
#                 openGL Widget
#       for dominant colors in 3D color spaces
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#               v0 - 2019/10/21
#
#
# * Options :
#     - Lights
#     - Mouse control :
#         . zoom with wheel
#         . rotate view on x/y axes with left mouse button
#         . rotate view on z axis with CTRL + left mouse button
#         . move view on x/y axes with right mouse button
#
# * QT signals sent when zoomed, moved or rotated
#
# * Public access to zoom, position and rotation
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
    glClearColor(0.2, 0.2, 0.2, 1.0f);
    glClear(Qt::black); // clear the screen with black color

    glEnable(GL_DEPTH_TEST); // z-sorting
    //glEnable(GL_DEPTH_CLAMP); // no clipping - it's a bit slower
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT); // all material has ambiant light
    glEnable(GL_COLOR_MATERIAL); // to see the two faces of a triangle, needed by lights when not shading
    glShadeModel(GL_SMOOTH); // blend colors - GL_FLAT chooses one color in the polygon

    glDisable(GL_CULL_FACE); // facet culling
    glEnable(GL_BLEND); // prefer using GLBlendFunc, used by the anaglyphic view

    xRot = 156; // initial values of rotation
    yRot = 28;
    zRot = 0;
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
    size3d = 1000.0f;

    nb_palettes = -1; // we are going to populate the palette
    for (int i = 0; i < 9; i++) // 4 * 4 *4 spĥeres
        for (int j = 0; j < 9; j++)
            for (int k = 0; k < 9; k++) {
                nb_palettes++; // palette index

                // RGB
                palettes[nb_palettes].RGB.R = i / 8.0f; // RGB values
                palettes[nb_palettes].RGB.G = j / 8.0f;
                palettes[nb_palettes].RGB.B = k / 8.0f;
                palettes[nb_palettes].percentage = 1.0f / 512.0f; // percentage

                // HSV
                float H, S, V, C; // HSLVC values
                RGBtoHSV(palettes[nb_palettes].RGB.R,
                         palettes[nb_palettes].RGB.G,
                         palettes[nb_palettes].RGB.B,
                         H, S, V, C); // convert RGB to HSV values
                palettes[nb_palettes].HSV.H = H;
                palettes[nb_palettes].HSV.C = C;
                palettes[nb_palettes].HSV.S = S;
                palettes[nb_palettes].HSV.V = V;

                // HWB
                float h, W, B; // HWB values
                HSVtoHWB(H, S, V, h, W, B);
                palettes[nb_palettes].HWB.H = h;
                palettes[nb_palettes].HWB.W = W;
                palettes[nb_palettes].HWB.B = B;

                // HSL
                float L;
                RGBtoHSL(palettes[nb_palettes].RGB.R,
                         palettes[nb_palettes].RGB.G,
                         palettes[nb_palettes].RGB.B,
                         H, S, L, C); // convert RGB to HSV values
                palettes[nb_palettes].HSL.H = H;
                palettes[nb_palettes].HSL.C = C;
                palettes[nb_palettes].HSL.S = S;
                palettes[nb_palettes].HSL.L = L;

                // XYZ
                float X, Y, Z; // XYZ and LAB values
                RGBtoXYZ(palettes[nb_palettes].RGB.R, palettes[nb_palettes].RGB.G, palettes[nb_palettes].RGB.B, X, Y, Z); // convert RGB to XYZ values
                palettes[nb_palettes].XYZ.X = X;
                palettes[nb_palettes].XYZ.Y = Y;
                palettes[nb_palettes].XYZ.Z = Z;

                // LAB
                float A;
                XYZtoLAB(X, Y, Z, L, A, B); // convert XYZ to LAB values
                palettes[nb_palettes].LAB.L = L;
                palettes[nb_palettes].LAB.A = A;
                palettes[nb_palettes].LAB.B = B;
            }
    nb_palettes++; // adjust number of palettes that must be in [1..x]
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

    //// draw color space

    if (color_space == "RGB") { // RGB
        // red x axis
        glLineWidth(32); // bigger width of the lines to really see them
        glBegin(GL_LINES); // draw several lines
            glColor3d(0,0,0); // axis origin : black
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(1,0,0); // x/R axis color : red
            glVertex3f(size3d, 0.0f, 0.0f);
        glEnd();
        DrawConeX(size3d, 0.0f, 0.0f, 200.0f, 50.0f, 100, 1, 0, 0);

        // green y axis
        glLineWidth(32); // bigger width of the lines to really see them
        glBegin(GL_LINES);
            glColor3d(0,0,0); // axis origin : black
            glVertex3f( 0.0f,     0.0f, 0.0f );
            glColor3d(0,1,0); // green
            glVertex3f( 0.0f, -size3d, 0.0f );
        glEnd();
        DrawConeY(0.0f, -size3d, 0.0f, -200.0f, 50.0f, 100, 0, 1, 0);

        // blue z axis
        glLineWidth(32); // bigger width of the lines to really see them
        glBegin(GL_LINES);
            glColor3d(0,0,0); // axis origin : black
            glVertex3f( 0.0f, 0.0f,    0.0f);
            glColor3d(0,0,1); // z axis color : blue
            glVertex3f( 0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 0, 0, 1);

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
            DrawSphere(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].RGB.R * size3d,
                       palettes[n].RGB.G * size3d,
                       palettes[n].RGB.B * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G,palettes[n].RGB.B);
    }

    if (color_space == "HSV") { // HSV
        // colored circle
        int num_segments = 360;
        float R, G, B;
        glLineWidth(32);
        glBegin(GL_LINE_LOOP); // colored circle
            for (int i = 0; i < num_segments; i++) {
                float theta = 2.0f * Pi * float(i) / float(num_segments); //current angle
                HSVtoRGB(float(i) / float(num_segments) * 360.0f, 1, 1, R, G, B);
                glColor3d(R, G, B); // color of segment
                glVertex3f(size3d * cosf(theta), -size3d * sinf(theta), size3d); // vertex
            }
        glEnd();

        // black circle
        DrawCircleXY(0, 0, 0, size3d, 360, 0, 0, 0, 32);

        // vertical axis black to white
        glLineWidth(32);
        glBegin(GL_LINES);
            glColor3d(0,0,0); // axis origin : black
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(1,1,1); // axis end : white
            glVertex3f(0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSphere(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].HSV.S * cos(palettes[n].HSV.H * 2 * Pi) * size3d,
                       palettes[n].HSV.S * sin(palettes[n].HSV.H * 2 * Pi) * size3d,
                       palettes[n].HSV.V * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B);
    }

    if (color_space == "HSL") { // HSL
        // colored circle
        int num_segments = 360;
        float R, G, B;
        glLineWidth(32);
        glBegin(GL_LINE_LOOP); // colored circle
            for (int i = 0; i < num_segments; i++) {
                float theta = 2.0f * Pi * float(i) / float(num_segments); //current angle
                HSLtoRGB(float(i) / float(num_segments) * 360.0f, 1, 0.5, R, G, B);
                glColor3d(R, G, B); // color of segment
                glVertex3f(size3d * cosf(theta), -size3d * sinf(theta), 500.0f); // vertex
            }
        glEnd();

        // black circle
        DrawCircleXY(0, 0, 0, size3d, 360, 0, 0, 0, 32);

        // white circle
        DrawCircleXY(0, 0, size3d, size3d, 360, 1, 1, 1, 32);

        // vertical axis black to white
        glLineWidth(32);
        glBegin(GL_LINES);
            glColor3d(0,0,0); // axis origin : black
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(1,1,1); // axis end : white
            glVertex3f(0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSphere(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].HSL.S * cos(palettes[n].HSL.H * 2 * Pi) * size3d,
                       palettes[n].HSL.S * sin(palettes[n].HSL.H * 2 * Pi) * size3d,
                       palettes[n].HSL.L * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B);
    }

    if (color_space == "HWB") { // HWB
        // colored circle
        int num_segments = 360;
        float R, G, B;
        glLineWidth(32); // bigger width of the lines to really see them
        glBegin(GL_LINE_LOOP);       
            for (int i = 0; i < num_segments; i++) {
                float theta = 2.0f * Pi * float(i) / float(num_segments); //current angle
                HWBtoRGB(float(i) / float(num_segments) * 360.0f, 0, 0, R, G, B);
                glColor3d(R, G, B); // color of segment
                glVertex3f(size3d * cosf(theta), -size3d * sinf(theta), 0.0f); // vertex
            }
        glEnd();

        // vertical axis white to black
        glLineWidth(32);
        glBegin(GL_LINES); // vertical axis
            glColor3d(1,1,1); // axis origin : white
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(0,0,0); // axis end : black
            glVertex3f(0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, 0.0f, -200.0f, 50.0f, 100, 1, 1, 1);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSphere(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].HWB.W * cos(palettes[n].HWB.H * 2 * Pi) * size3d,
                       palettes[n].HWB.W * sin(palettes[n].HWB.H * 2 * Pi) * size3d,
                       palettes[n].HWB.B * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B);
    }

    if (color_space == "HCV") { // HCV
        // colored circle
        int num_segments = 360;
        float R, G, B;
        glLineWidth(32); // bigger width of the lines to really see them
        glBegin(GL_LINE_LOOP); // colored circle   
            for (int i = 0; i < num_segments; i++) {
                float theta = 2.0f * Pi * float(i) / float(num_segments); //current angle
                HSVtoRGB(float(i) / float(num_segments) * 360.0f, 1, 1, R, G, B);
                glColor3d(R, G, B); // color of segment
                glVertex3f(size3d * cosf(theta), -size3d * sinf(theta), size3d); // vertex
            }
        glEnd();

        // vertical axis black to white
        glLineWidth(32);
        glBegin(GL_LINES); // vertical axis
            glColor3d(0,0,0); // axis origin : black
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(1,1,1); // axis end : white
            glVertex3f(0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSphere(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].HSV.C * cos(palettes[n].HSV.H * 2 * Pi) * size3d,
                       palettes[n].HSV.C * sin(palettes[n].HSV.H * 2 * Pi) * size3d,
                       palettes[n].HSV.V * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B);
    }

    if (color_space == "HCL") { // HCL
        // colored circle
        int num_segments = 360;
        float R, G, B;
        glLineWidth(32);
        glBegin(GL_LINE_LOOP); // colored circle
            for (int i = 0; i < num_segments; i++) {
                float theta = 2.0f * Pi * float(i) / float(num_segments); //current angle
                HSLtoRGB(float(i) / float(num_segments) * 360.0f, 1, 0.5, R, G, B);
                glColor3d(R, G, B); // color of segment
                glVertex3f(size3d * cosf(theta), -size3d * sinf(theta), 500.0f); // vertex
            }
        glEnd();

        // vertical axis black to white
        glLineWidth(32);
        glBegin(GL_LINES); // vertical axis
            glColor3d(0,0,0); // axis origin : black
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor3d(1,1,1); // axis end : white
            glVertex3f(0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);
        DrawConeZ(0.0f, 0.0f, 0.0f, -200.0f, 50.0f, 100, 0, 0, 0);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSphere(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].HSL.C * cos(palettes[n].HSL.H * 2 * Pi) * size3d,
                       palettes[n].HSL.C * sin(palettes[n].HSL.H * 2 * Pi) * size3d,
                       palettes[n].HSL.L * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B);
    }

    if (color_space == "XYZ") { // CIE XYZ
        // colored boundaries
        XYZPlotFromCSV("xyz-space.csv", size3d);

        // White point
        glLineWidth(4);
        glBegin(GL_LINES); // draw several lines
            glColor3d(1,1,1);
            glVertex3f(0,0,0);
            glVertex3f(1.0f / 3.0f * size3d, -1.0f / 3.0f * size3d, 1.0f / 3.0f * size3d);
        glEnd();

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
            glVertex3f( 0.0f, 0.0f,    0.0f);
            glVertex3f( 0.0f, 0.0f, size3d);
        glEnd();
        DrawConeZ(0.0f, 0.0f, size3d, 200.0f, 50.0f, 100, 1, 1, 1);

        // values
        for (int n = 0; n < nb_palettes;n++) { // for each color in palette
            float sum = palettes[n].XYZ.X + palettes[n].XYZ.Y + palettes[n].XYZ.Z;

            float x = palettes[n].XYZ.X / sum;
            float y = palettes[n].XYZ.Y / sum;
            float z = 1.0 - x - y;
            DrawSphere(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       x * size3d, y * size3d, z * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B);
        }
    }

    if (color_space == "LAB") { // LAB
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
            glColor3d(0,1,0); // green
            glVertex3f(-size3d, 0.0f, 0.0f);
            glColor3d(1,0,0); // red
            glVertex3f(size3d, 0.0f, 0.0f);
        glEnd();
        DrawConeX(size3d, 0.0f, 0.0f, 200.0f, 50.0f, 100, 1, 0, 0); // red arrow
        DrawConeX(-size3d, 0.0f, 0.0f, -200.0f, 50.0f, 100, 0, 1, 0); // green arrow

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

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSphere(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].LAB.A / 127.0f * size3d,
                       palettes[n].LAB.B / 127.0f * size3d,
                       palettes[n].LAB.L / 100.0f * size3d,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B);
    }

    if (color_space == "Wheel") { // Color Wheel
        // external circle
        float radius = size3d + 150.0f;
        DrawCircleXY(0, 0, 0, radius, 100, 1, 1, 1, 32);
        // internal circle
        DrawCircleXY(0, 0, 0, size3d, 100, 0.5, 0.5, 0.5, 4);
        // reference colors : a sphere with a white circle
        // red
        float angle = 0.0f;
        DrawCircleXY(radius * cos(angle), -radius * sin(angle), 0, 100, 100, 1, 1, 1, 32);
        DrawSphere(3, 100, radius * cos(angle), -radius * sin(angle), 0, 1, 0, 0);
        // blue
        angle = 120.0f / 360.0f * 2 * Pi;
        DrawCircleXY(radius * cos(angle), -radius * sin(angle), 0, 100, 100, 1, 1, 1, 32);
        DrawSphere(3, 100, radius * cos(angle), -radius * sin(angle), 0, 0, 0, 1);
        // green
        angle = 240.0f / 360.0f * 2 * Pi;
        DrawCircleXY(radius * cos(angle), -radius * sin(angle), 0, 100, 100, 1, 1, 1, 32);
        DrawSphere(3, 100, radius * cos(angle), -radius * sin(angle), 0, 0, 1, 0);
        // magenta
        angle = 60.0f / 360.0f * 2 * Pi;
        DrawCircleXY(radius * cos(angle), -radius * sin(angle), 0, 75, 100, 1, 1, 1, 32);
        DrawSphere(3, 75, radius * cos(angle), -radius * sin(angle), 0, 1, 0, 1);
        // cyan
        angle = 180.0f / 360.0f * 2 * Pi;
        DrawCircleXY(radius * cos(angle), -radius * sin(angle), 0, 75, 100, 1, 1, 1, 32);
        DrawSphere(3, 75, radius * cos(angle), -radius * sin(angle), 0, 0, 1, 1);
        // yellow
        angle = 300.0f / 360.0f * 2 * Pi;
        DrawCircleXY(radius * cos(angle), -radius * sin(angle), 0, 75, 100, 1, 1, 1, 32);
        DrawSphere(3, 75, radius * cos(angle), -radius * sin(angle), 0, 1, 1, 0);
        // pink
        angle = 30.0f / 360.0f * 2 * Pi;
        DrawCircleXY(radius * cos(angle), -radius * sin(angle), 0, 50, 100, 1, 1, 1, 32);
        DrawSphere(3, 50, radius * cos(angle), -radius * sin(angle), 0, 1, 0, 0.5);
        // purple
        angle = 90.0f / 360.0f * 2 * Pi;
        DrawCircleXY(radius * cos(angle), -radius * sin(angle), 0, 50, 100, 1, 1, 1, 32);
        DrawSphere(3, 50, radius * cos(angle), -radius * sin(angle), 0, 0.5, 0, 1);
        // azure
        angle = 150.0f / 360.0f * 2 * Pi;
        DrawCircleXY(radius * cos(angle), -radius * sin(angle), 0, 50, 100, 1, 1, 1, 32);
        DrawSphere(3, 50, radius * cos(angle), -radius * sin(angle), 0, 0, 0.5, 1);
        // blue-green
        angle = 210.0f / 360.0f * 2 * Pi;
        DrawCircleXY(radius * cos(angle), -radius * sin(angle), 0, 50, 100, 1, 1, 1, 32);
        DrawSphere(3, 50, radius * cos(angle), -radius * sin(angle), 0, 0, 1, 0.5);
        // chartreuse
        angle = 270.0f / 360.0f * 2 * Pi;
        DrawCircleXY(radius * cos(angle), -radius * sin(angle), 0, 50, 100, 1, 1, 1, 32);
        DrawSphere(3, 50, radius * cos(angle), -radius * sin(angle), 0, 0.5, 1, 0);
        // orange
        angle = 330.0f / 360.0f * 2 * Pi;
        DrawCircleXY(radius * cos(angle), -radius * sin(angle), 0, 50, 100, 1, 1, 1, 32);
        DrawSphere(3, 50, radius * cos(angle), -radius * sin(angle), 0, 1, 0.5, 0);

        // values
        for (int n = 0; n < nb_palettes;n++) // for each color in palette
            DrawSphere(3, palettes[n].percentage * size3d * nb_palettes / 500.0f * sphere_size,
                       palettes[n].HSL.L * cos(palettes[n].HSL.H * 2 * Pi) * size3d,
                       palettes[n].HSL.L * sin(palettes[n].HSL.H * 2 * Pi) * size3d,
                       0,
                       palettes[n].RGB.R, palettes[n].RGB.G, palettes[n].RGB.B);
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
    int n = event->delta(); // amount of wheel turn
    //zoom3D += n / 120 / 2; // should work with this (standard) value

    if (n < 0) // zoom out
        zoom3D = zoom3D / 1.25;
    else // zoom in
        zoom3D = zoom3D * 1.25;

    emit zoomChanged(zoom3D); // emit signal

    update(); // redraw 3d scene
}

///////////////////////////////////////////////
//// Capture 3D scene to QImage
///////////////////////////////////////////////

void openGLWidget::Capture() // take a snapshot of rendered 3D scene
{
    capture3D = grabFramebuffer(); // slow because it relies on glReadPixels() to read back the pixels
}
