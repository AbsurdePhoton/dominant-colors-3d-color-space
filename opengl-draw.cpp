/*#-------------------------------------------------
#
#          Draw primitives for OpenGL
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v0 - 2019/10/21
#
#   - circles
#   - cones
#   - spheres
#   - XYZ color space
#
#-------------------------------------------------*/

#include <QtOpenGL>
#include "dominant-colors.h"
#include "mat-image-tools.h"

using namespace std;

///////////////////////////////////////////////
//// Draw from file
///////////////////////////////////////////////

void XYZPlotFromCSV(std::string name, const float &size3d) // draw XYZ color space
{
    std::string line; // line to read in text file
    ifstream file; // file to read
    file.open(name); // read color file file
    setlocale(LC_ALL, "C"); // for numeric separator=dot instead of comma when using stof()

    if (file) { // if successfully read
        getline(file, line); // jump csv header line
        size_t pos; // index for find function
        std::string s; // used for item extraction
        float R, G, B, W, X, Y, Z;
        glBegin(GL_LINE_STRIP); // continuous lines
            glLineWidth(32); // width of line
            while (getline(file, line)) { // read each line of text file: X Y Z wavelength
                pos = 0; // find index at the beginning of the line
                int pos2 = line.find(";", pos); // find first semicolon char
                s = line.substr(pos, pos2 - pos); // extract X value
                X = std::stof(s); // X
                pos = pos2 + 1; // next char
                pos2 = line.find(";", pos); // find second semicolon char
                s = line.substr(pos, pos2 - pos); // extract Y value
                Y = std::stof(s); // Y
                pos = pos2 + 1; // next char
                pos2 = line.find(";", pos); // find third semicolon char
                s = line.substr(pos, pos2 - pos); // extract Z value
                Z = std::stof(s); // Z
                s = line.substr(pos2 + 1, line.length() - pos2); // color wavelength is at the end of the line
                W = std::stof(s); // wavelength

                SpectralColorToRGB(W, R, G, B); // convert wavelength to RGB
                if (R + G + B != 0) { // some wavelengths are not visible
                    glColor3d(R, G, B); // change color to computed RGB
                    glVertex3f(X * size3d, -Y * size3d, Z * size3d); // add vertex
                }
            }
        glEnd();

        file.close(); // close text file
    }
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
//// Sphere
///////////////////////////////////////////////

#define X .525731112119133606 // pre-calculated matrix
#define Z .850650808352039932

static GLfloat vdata[12][3] = {
    {-X, 0.0, Z}, {X, 0.0, Z}, {-X, 0.0, -Z}, {X, 0.0, -Z},
    {0.0, Z, X}, {0.0, Z, -X}, {0.0, -Z, X}, {0.0, -Z, -X},
    {Z, X, 0.0}, {-Z, X, 0.0}, {Z, -X, 0.0}, {-Z, -X, 0.0}
};
static GLuint tindices[20][3] = {
    {0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1},
    {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},
    {7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6},
    {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11} };

void normalize(GLfloat *a) // normalize a vector
{
    GLfloat d = sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]); // euclidian distance
    a[0]/=d; a[1]/=d; a[2]/=d; // normalized
}

void DrawTri(GLfloat *a, GLfloat *b, GLfloat *c, int div, float r, float x, float y, float z) // used to subdivide surfaces
{
    if (div <= 0) {
        glNormal3fv(a);
        glVertex3f(a[0]*r + x, a[1]*r - y, a[2]*r + z);
        glNormal3fv(b);
        glVertex3f(b[0]*r + x, b[1]*r - y, b[2]*r + z);
        glNormal3fv(c);
        glVertex3f(c[0]*r + x, c[1]*r - y, c[2]*r + z);
    } else {
        GLfloat ab[3], ac[3], bc[3];
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

void DrawSphere(int ndiv, float radius, float x, float y, float z, float r, float g, float b) // draw a sphere with ndiv as number of subdivisions
{
    glColor3d(r,g,b); // change color
    glBegin(GL_TRIANGLES); // draw triangles
        for (int i = 0; i < 20; i++) // 20 times
            DrawTri(vdata[tindices[i][0]], vdata[tindices[i][1]], vdata[tindices[i][2]], ndiv, radius, x, y, z); // recursive drawing with triangle subdivision with sphere data
    glEnd();
}
