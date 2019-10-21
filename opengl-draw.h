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

#ifndef OPENGLDRAW_H
#define OPENGLDRAW_H

void XYZPlotFromCSV(std::string name, const float &size3d); // Draw XYZ color space from csv file based on light wavelength

void DrawFilledCircleXY(const float &center_x, const float &center_y, const float &center_z, const float &radius, const int &segments, const float &R, const float &G, const float &B); // filled circle in XY plane
void DrawCircleXY(const float &center_x, const float &center_y, const float &center_z, const float &radius, const int &segments, const float &R, const float &G, const float &B, const float &width); // circle in XY plane

void DrawConeX(const float &center_x, const float &center_y, const float &center_z, const float &length, const float &radius, const int &segments, const float &R, const float &G, const float &B); // cone along X axis
void DrawConeY(const float &center_x, const float &center_y, const float &center_z, const float &length, const float &radius, const int &segments, const float &R, const float &G, const float &B); // cone along Y axis
void DrawConeZ(const float &center_x, const float &center_y, const float &center_z, const float &length, const float &radius, const int &segments, const float &R, const float &G, const float &B); // cone along Z axis

void DrawSphere(int ndiv, float radius, float x, float y, float z, float r, float g, float b); // sphere

#endif // OPENGLDRAW_H

