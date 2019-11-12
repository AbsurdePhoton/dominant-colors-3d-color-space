/*#-------------------------------------------------
#
#          Draw functions for OpenGL
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1 - 2019/11/08
#
#   - circles
#   - cones
#   - spheres
#   - CMF in color spaces
#
#-------------------------------------------------*/

#include <QtOpenGL>
#include "opencv2/opencv.hpp"
#include "dominant-colors.h"
#include "color-spaces.h"
#include "mat-image-tools.h"
#include "opengl-draw.h"

using namespace std;
using namespace cv;

///////////////////////////////////////////////
//// Draw CMF in color spaces
///////////////////////////////////////////////

void DrawCMFinXYZ(const float &size3d, const bool spectrum_locus) // draw Color Matching Functions in XYZ color space
{
    double R, G, B, X, Y, Z, W;

    glLineWidth(4); // width of line
    glBegin(GL_LINE_STRIP); // continuous lines
    for (int w = 0; w < wavelength_XYZ_nb; w++) {
        W = wavelength_XYZ[w][0];

        if ((W >= 390) and (W <= 700)) { // some wavelengths are not visible - visible range 400-700nm
            WavelengthToXYZ(W, X, Y, Z);
            XYZtoRGB(X, Y, Z, R, G, B); // second option : convert XYZ to RGB

            double sum;
            if (spectrum_locus)
                sum = X + Y + Z;
            else
                sum = 1;

            glColor3d(R, G, B); // change color to computed RGB
            if (spectrum_locus)
                glVertex3f(Y / sum * size3d, -X / sum * size3d, Z / sum * size3d); // add vertex
            else
                glVertex3f(X / sum * size3d, -Y / sum * size3d, Z / sum * size3d); // add vertex
        }
    }
    glEnd();
}

void DrawCMFinLuv(const float &size3d) // draw Color Matching Functions in L*u*v* color space
{
    double W, X, Y, Z, R, G, B, L, u, v;

    glLineWidth(4);
    glBegin(GL_LINE_STRIP);
        for (int w = 0; w < wavelength_XYZ_nb; w++) {
            W = wavelength_XYZ[w][0];

            if ((W >= 390) and (W <= 700)) {
                WavelengthToXYZ(W, X, Y, Z);
                XYZtoRGB(X, Y, Z, R, G, B); // convert XYZ to RGB
                XYZtoLuv(X, Y, Z, L, u, v); // convert XYZ to Luv

                glColor3d(R, G, B); // change color to computed RGB
                glVertex3f(v * size3d, -u * size3d, L * size3d); // add vertex
            }
        }
    glEnd();
}

void DrawCMFinLab(const float &size3d) // draw Color Matching Functions in L*a*b* color space
{
    double W, X, Y, Z, R, G, B, L, a, b;

    glLineWidth(4);
    glBegin(GL_LINE_STRIP);
        for (int w = 0; w < wavelength_XYZ_nb; w++) {
            W = wavelength_XYZ[w][0];

            if ((W >= 390) and (W <= 700)) {
                WavelengthToXYZ(W, X, Y, Z);
                XYZtoRGB(X, Y, Z, R, G, B); // convert XYZ to RGB
                XYZtoLAB(X, Y, Z, L, a, b); // convert XYZ to L*a*b*

                glColor3d(R, G, B); // change color to computed RGB
                glVertex3f(-a * size3d, -b * size3d, L * size3d); // add vertex
            }
        }
    glEnd();
}

void DrawCMFinLMS(const float &size3d) // draw Color Matching Functions in LMS color space
{
    double W, X, Y, Z, R, G, B, L, M, S;

    glLineWidth(4);
    glBegin(GL_LINE_STRIP);
        for (int w = 0; w < wavelength_XYZ_nb; w++) {
            W = wavelength_XYZ[w][0];

            if ((W >= 390) and (W <= 700)) {
                WavelengthToXYZ(W, X, Y, Z);
                XYZtoRGB(X, Y, Z, R, G, B); // convert XYZ to RGB
                XYZtoLMS(X, Y, Z, L, M, S); // convert XYZ to LMS

                glColor3d(R, G, B); // change color to computed RGB
                glVertex3f(L * size3d, -M * size3d, S * size3d); // add vertex
            }
        }
    glEnd();
}

void DrawCMFinHLAB(const float &size3d) // draw Color Matching Functions in Hunter LAB color space
{
    double W, X, Y, Z, R, G, B, L, a, b;

    glLineWidth(4);
    glBegin(GL_LINE_LOOP);
        for (int w = 0; w < wavelength_XYZ_nb; w++) {
            W = wavelength_XYZ[w][0];

            if ((W >= 443) and (W <= 630)) {
                WavelengthToXYZ(W, X, Y, Z);
                XYZtoRGB(X, Y, Z, R, G, B); // convert XYZ to RGB
                XYZtoHLAB(X, Y, Z, L, a, b); // convert XYZ to Hunter LAB

                glColor3d(R, G, B); // change color to computed RGB
                glVertex3f(-a * size3d, -b * size3d, L * size3d); // add vertex
            }
        }
    glEnd();
}

///////////////////////////////////////////////
//// Circle
///////////////////////////////////////////////

void DrawFilledCircleXY(const float &center_x, const float &center_y, const float &center_z, const float &radius, const int &segments, const float &R, const float &G, const float &B) // filled circle in XY plane
{
    glColor3d(R, G, B); // color to draw
    glBegin(GL_POLYGON); // simple polygon
        glVertex3d(radius * cos(0.0) + center_x, radius * sin(0.0) + center_y, center_z); // add first vertex
        for (int i = 0; i < segments; i++) // for n segments
        {
            float alpha = 2.0f * Pi * float(i) / float(segments); //current angle
            glVertex3d(radius * cos(alpha) + center_x, radius * sin(alpha) + center_y, center_z); // add vertex
        }
    glEnd();
}

void DrawCircleXY(const float &center_x, const float &center_y, const float &center_z, const float &radius, const int &segments, const float &R, const float &G, const float &B, const float &width)
{
    glColor3d(R, G, B); // color to draw
    glLineWidth(width); // line width
    glBegin(GL_LINE_LOOP); // simple line with ends connected
        for (int i = 0; i < segments; i++) // for n segments
        {
            float alpha = 2.0f * Pi * float(i) / float(segments); //current angle
            glVertex3d(radius * cos(alpha) + center_x, radius * sin(alpha) + center_y, center_z); // add vertex
        }
    glEnd();
}

void DrawCircleArcXY(const float &center_x, const float &center_y, const float &center_z, const float &radius, const int &segments, const float &begin, const float &end, const float &R, const float &G, const float &B, const float &width)
{
    float arc_begin = begin / 360.0 * 2 * Pi;
    float arc_end = end / 360.0 * 2 * Pi;

    glColor3d(R, G, B); // color to draw
    glLineWidth(width); // line width
    glBegin(GL_LINE_STRIP); // simple line with ends connected
        for (int i = 0; i < segments; i++) // for n segments
        {
            float alpha = 2.0f * Pi * float(i) / float(segments); //current angle
            if ((alpha >= arc_begin) and (alpha <= arc_end))
                glVertex3d(radius * cos(alpha) + center_x, radius * sin(alpha) + center_y, center_z); // add vertex
        }
    glEnd();
}

///////////////////////////////////////////////
//// Cone
///////////////////////////////////////////////

void DrawConeX(const float &center_x, const float &center_y, const float &center_z, const float &length, const float &radius, const int &segments, const float &R, const float &G, const float &B) // Draw cone along x axis
{
    static const float angle = 2.0f * Pi / float(segments);
    glColor3d(R, G, B); // change color to draw
    for (int i = 0; i < segments; i++) { // for n segments
        float alpha = 2.0f * Pi * float(i) / float(segments); //current angle
        glBegin(GL_POLYGON); // first polygon : base
            glVertex3d(center_x, center_y, center_z); // center
            glVertex3d(center_x, radius * cos(alpha) + center_y, radius * sin(alpha) + center_z);
            glVertex3d(center_x, radius * cos(alpha + angle) + center_y, radius * sin(alpha + angle) + center_z);
        glEnd();
        glBegin(GL_POLYGON); // second polygon : tip
            glVertex3d(center_x + length, center_y, center_z); // tip
            glVertex3d(center_x, radius * cos(alpha) + center_y, radius * sin(alpha) + center_z);
            glVertex3d(center_x, radius * cos(alpha + angle) + center_y, radius * sin(alpha + angle) + center_z);
        glEnd();
    }
}

void DrawConeY(const float &center_x, const float &center_y, const float &center_z, const float &length, const float &radius, const int &segments, const float &R, const float &G, const float &B) // Draw cone along y axis
{
    // see DrawConeX for comments, it's almost the same
    static const float angle = 2.0f * Pi / float(segments);
    glColor3d(R, G, B);
    for (int i = 0; i < segments; i++) {
        float alpha = 2.0f * Pi * float(i) / float(segments); //current angle
        glBegin(GL_POLYGON);
            glVertex3d(center_x, center_y, center_z);
            glVertex3d(radius * sin(alpha) + center_x, center_y, radius * cos(alpha) + center_z);
            glVertex3d(radius * sin(alpha + angle) + center_x, center_y, radius * cos(alpha + angle) + center_z);
        glEnd();
        glBegin(GL_POLYGON);
            glVertex3d(center_x, center_y + length, center_z);
            glVertex3d(radius * sin(alpha) + center_x, center_y, radius * cos(alpha) + center_z);
            glVertex3d(radius * sin(alpha + angle) + center_x, center_y, radius * cos(alpha + angle) + center_z);
        glEnd();
    }
}

void DrawConeZ(const float &center_x, const float &center_y, const float &center_z, const float &length, const float &radius, const int &segments, const float &R, const float &G, const float &B) // Draw cone along z axis
{
    // see DrawConeX for comments, it's almost the same
    static const float angle = 2.0f * Pi / float(segments);
    glColor3d(R, G, B);
    for (int i = 0; i < segments; i++) {
        float alpha = 2.0f * Pi * float(i) / float(segments); //current angle
        glBegin(GL_POLYGON);
            glVertex3d(center_x, center_y, center_z);
            glVertex3d(radius * cos(alpha) + center_x, radius * sin(alpha) + center_y, center_z);
            glVertex3d(radius * cos(alpha + angle) + center_x, radius * sin(alpha + angle) + center_y, center_z);
        glEnd();
        glBegin(GL_POLYGON);
            glVertex3d(center_x, center_y, center_z + length);
            glVertex3d(radius * cos(alpha) + center_x, radius * sin(alpha) + center_y, center_z);
            glVertex3d(radius * cos(alpha + angle) + center_x, radius * sin(alpha + angle) + center_y, center_z);
        glEnd();
    }
}

///////////////////////////////////////////////
//// Sphere by recursive triangles subdivision
///////////////////////////////////////////////

#define X .525731112119133606 // pre-calculated matrix
#define Z .850650808352039932

static float vdata[12][3] = {
    {-X, 0.0, Z}, {X, 0.0, Z}, {-X, 0.0, -Z}, {X, 0.0, -Z},
    {0.0, Z, X}, {0.0, Z, -X}, {0.0, -Z, X}, {0.0, -Z, -X},
    {Z, X, 0.0}, {-Z, X, 0.0}, {Z, -X, 0.0}, {-Z, -X, 0.0}
};
static GLuint tindices[20][3] = {
    {0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1},
    {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},
    {7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6},
    {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11} };

void normalize(float *a) // normalize a vector
{
    float d = sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]); // euclidian distance
    a[0]/=d; a[1]/=d; a[2]/=d; // normalized
}

void DrawTri(float *a, float *b, float *c, int div, float r, float x, float y, float z) // used to subdivide surfaces
{
    if (div <= 0) {
        glNormal3fv(a);
        glVertex3f(a[0]*r + x, a[1]*r - y, a[2]*r + z);
        glNormal3fv(b);
        glVertex3f(b[0]*r + x, b[1]*r - y, b[2]*r + z);
        glNormal3fv(c);
        glVertex3f(c[0]*r + x, c[1]*r - y, c[2]*r + z);
    } else {
        float ab[3], ac[3], bc[3];
        for (int i = 0; i < 3; i++) { // half
            ab[i] = (a[i]+b[i]) / 2.0;
            ac[i] = (a[i]+c[i]) / 2.0;
            bc[i] = (b[i]+c[i]) / 2.0;
        }
        normalize(ab);
        normalize(ac);
        normalize(bc);
        DrawTri(a, ab, ac, div-1, r, x, y, z); // new divided triangles
        DrawTri(b, bc, ab, div-1, r, x, y, z);
        DrawTri(c, ac, bc, div-1, r, x, y, z);
        DrawTri(ab, bc, ac, div-1, r, x, y, z);  // comment this line and sphere looks really cool in wireframe
    }
}

void DrawSphere(const int &ndiv, const float &radius, const float &x, const float &y, const float &z, const float &r, const float &g, const float &b) // draw a sphere with ndiv as number of subdivisions
{
    glColor3d(r,g,b); // change color
    glBegin(GL_TRIANGLES); // draw triangles
        for (int i = 0; i < 20; i++) // 20 times
            DrawTri(vdata[tindices[i][0]], vdata[tindices[i][1]], vdata[tindices[i][2]], ndiv, radius, x, y, z); // recursive drawing with triangle subdivision with sphere data
    glEnd();
}

///////////////////////////////////////////////
//// Matrix to lines
///////////////////////////////////////////////

void DrawLinesFromMatrix(const Mat &matrix, const float &x0, const float &y0, const float &z0, const float &scale, const float &R, const float &G, const float &B, const float &width)
{
    glColor3f(R,G,B);
    glLineWidth(width);
    glBegin(GL_LINES);
        for (int x = 0; x < matrix.cols; x++) // down
            for (int y = 0; y < matrix.rows - 1; y++)
                if ((matrix.at<uchar>(y, x) != 0) & (matrix.at<uchar>(y + 1, x) != 0)) {
                    glVertex3f(scale * x + x0, -scale * y + y0, z0);
                    glVertex3f(scale * x + x0, -scale * (y + 1) + y0, z0);
                }
        for (int x = 0; x < matrix.cols - 1; x++) // right
            for (int y = 0; y < matrix.rows; y++)
                if ((matrix.at<uchar>(y, x) != 0) & (matrix.at<uchar>(y, x + 1) != 0)) {
                    glVertex3f(scale * x + x0, -scale * y + y0, z0);
                    glVertex3f(scale * (x + 1) + x0, -scale * y + y0, z0);
                }
        for (int x = 0; x < matrix.cols - 1; x++) // down-right
            for (int y = 0; y < matrix.rows - 1; y++)
                if ((matrix.at<uchar>(y, x) != 0) & (matrix.at<uchar>(y + 1, x + 1) != 0) & (matrix.at<uchar>(y, x + 1) == 0) & (matrix.at<uchar>(y + 1, x) == 0)) {
                    glVertex3f(scale * x + x0, -scale * y + y0, z0);
                    glVertex3f(scale * (x + 1) + x0, -scale * (y + 1) + y0, z0);
                }
        for (int x = 1; x < matrix.cols; x++) // down-left
            for (int y = 0; y < matrix.rows - 1; y++)
                if ((matrix.at<uchar>(y, x) != 0) & (matrix.at<uchar>(y + 1, x - 1) != 0) & (matrix.at<uchar>(y, x - 1) == 0) & (matrix.at<uchar>(y + 1, x) == 0)) {
                    glVertex3f(scale * x + x0, -scale * y + y0, z0);
                    glVertex3f(scale * (x - 1) + x0, -scale * (y + 1) + y0, z0);
                }
    glEnd();

        // isolated dots
        for (int x = 0; x < matrix.cols; x++) // down-left
            for (int y = 0; y < matrix.rows; y++)
                if ((matrix.at<uchar>(y, x) != 0) & (matrix.at<uchar>(y - 1, x - 1) == 0) & (matrix.at<uchar>(y - 1, x) == 0) & (matrix.at<uchar>(y - 1, x + 1) == 0)
                        & (matrix.at<uchar>(y, x - 1) == 0) & (matrix.at<uchar>(y, x + 1) == 0)
                        & (matrix.at<uchar>(y + 1, x - 1) == 0) & (matrix.at<uchar>(y + 1, x) == 0) & (matrix.at<uchar>(y + 1, x + 1) == 0)) {
                    //DrawCircleXY(scale * x + x0, scale * y + y0, z0 , 4, 10, 1, 1, 1, 4);
                    DrawSphere(3, width / 2.0, scale * x + x0, -scale * y - y0, z0, R, G, B);
                }

}

///////////////////////////////////////////////
//// Draw text from string
///////////////////////////////////////////////

void DrawChar(const uchar ch, const float &x0, const float &y0, const float &z0, const float &scale, const float &R, const float &G, const float &B, const float &width)
{
    if ((ch < characters_begin) or (ch > characters_end))
            return;
    int c = ch - characters_begin;

    glColor3f(R,G,B);
    glLineWidth(width);
    glBegin(GL_LINES);
        for (int x = 0; x < characters_matrix_cols; x++) // down
            for (int y = 0; y < characters_matrix_rows - 1; y++)
                if ((characters[c].data[x][y] != 0) & (characters[c].data[x][y + 1] != 0)) {
                    glVertex3f(scale * x + x0, scale * y + y0, z0);
                    glVertex3f(scale * x + x0, scale * (y + 1) + y0, z0);
                }
        for (int x = 0; x < characters_matrix_cols - 1; x++) // right
            for (int y = 0; y < characters_matrix_rows; y++)
                if ((characters[c].data[x][y] != 0) & (characters[c].data[x + 1][y] != 0)) {
                    glVertex3f(scale * x + x0, scale * y + y0, z0);
                    glVertex3f(scale * (x + 1) + x0, scale * y + y0, z0);
                }
        for (int x = 0; x < characters_matrix_cols - 1; x++) // down-right
            for (int y = 0; y < characters_matrix_rows - 1; y++)
                if ((characters[c].data[x][y] != 0) & (characters[c].data[x + 1][y + 1] != 0) & (characters[c].data[x + 1][y] == 0) & (characters[c].data[x][y + 1] == 0)) {
                    glVertex3f(scale * x + x0, scale * y + y0, z0);
                    glVertex3f(scale * (x + 1) + x0, scale * (y + 1) + y0, z0);
                }
        for (int x = 1; x < characters_matrix_cols; x++) // down-left
            for (int y = 0; y < characters_matrix_rows - 1; y++)
                if ((characters[c].data[x][y] != 0) & (characters[c].data[x - 1][y + 1] != 0) & (characters[c].data[x - 1][y] == 0) & (characters[c].data[x][y + 1] == 0)) {
                    glVertex3f(scale * x + x0, scale * y + y0, z0);
                    glVertex3f(scale * (x - 1) + x0, scale * (y + 1) + y0, z0);
                }
    glEnd();

        // isolated dots
        for (int x = 0; x < characters_matrix_cols; x++) // down-left
            for (int y = 0; y < characters_matrix_rows; y++)
                if ((characters[c].data[x][y] != 0)
                        & (characters[c].data[x - 1][y - 1] == 0) & (characters[c].data[x][y - 1] == 0) & (characters[c].data[x + 1][y - 1] == 0)
                        & (characters[c].data[x - 1][y] == 0) & (characters[c].data[x + 1][y] == 0)
                        & (characters[c].data[x - 1][y + 1] == 0) & (characters[c].data[x][y + 1] == 0) & (characters[c].data[x + 1][y + 1] == 0)) {
                    //DrawCircleXY(scale * x + x0, scale * y + y0, z0 , 4, 10, 1, 1, 1, 4);
                    DrawSphere(3, width, scale * x + x0, -scale * y - y0, z0, R, G, B);
                }

}

void DrawText(const std::string &text, const float &x0, const float &y0, const float &z0, const float &scale, const float &R, const float &G, const float &B, const float &width)
{
    float xCurrent = 0;

    for (int n = 0; n < text.length(); n++) {
        DrawChar(text[n], x0, y0 + xCurrent, z0, scale, R, G, B, width);
        xCurrent += scale * characters_matrix_cols;
    }
}
