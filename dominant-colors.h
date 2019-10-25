/*#-------------------------------------------------
#
#     Dominant colors library with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.1 - 2019/10/24
#
#   - eigen vectors algorithm
#   - K-means algorithm
#   - conversions between color spaces
#
#-------------------------------------------------*/

#ifndef DOMINANT_H
#define DOMINANT_H

#include "opencv2/opencv.hpp"

//// palette

struct struct_rgb {
    float R; // range [0..255] - Red
    float G; // range [0..255] - Green
    float B; // range [0..255] - Blue
};
struct struct_hsl {
    float H; // range [0..1] - Hue
    float C; // range [0..1] - Chroma
    float S; // range [0..1] - Saturation
    float L; // range [0..1] - Lightness
};
struct struct_hsv {
    float H; // range [0..1] - Hue
    float C; // range [0..1] - Chroma
    float S; // range [0..1] - Saturation
    float V; // range [0..1] - Value
};
struct struct_hwb {
    float H; // range [0..1] - Hue
    float W; // range [0..1] - White
    float B; // range [0..1] - Black
};
struct struct_xyz {
    float X; // range [0..1] - mix of response curves
    float Y; // range [0..1] - luminance
    float Z; // range [0..1] - quasi-equal to blue
};
struct struct_lab {
    float L; // range [0..1] - Lightness
    float A; // range [-1..1] - red/magenta to green
    float B; // range [-1..1] - yellow to blue
};
struct struct_hlab {
    float L; // range [0..1] - Lightness
    float A; // range [-1..1] - red/magenta to green
    float B; // range [-1..1] - yellow to blue
};
struct struct_lch {
    float L; // range [0..1] - Lightness
    float C; // range [0..1] - Chroma
    float H; // range [0..1] - Hue
};

struct struct_palette { // structure of a color value
    struct_rgb RGB;
    struct_hsl HSL;
    struct_hsv HSV;
    struct_hwb HWB;
    struct_xyz XYZ;
    struct_lab CIELAB;
    struct_hlab HLAB;
    struct_lch LCH;
    int count; // occurences
    float percentage; // percentage
};

//// Dominant colors

typedef struct color_node { // for eigen algorithm
    cv::Mat     mean;
    cv::Mat     cov;
    uchar       class_id;

    color_node *left;
    color_node *right;
} color_node;

std::vector<cv::Vec3b> DominantColorsEigen(const cv::Mat &img, const int &nb_colors, cv::Mat *quantized);

cv::Mat DominantColorsKMeans(const cv::Mat &image, const int &cluster_number, cv::Mat1f *dominant_colors);

//// Color spaces conversions

void SpectralColorToRGB(const float &L, float &R, float &G, float &B); // convert wavelength color value to RGB

void RGBtoHSV(const float &R, const float &G, const float &B, float& H, float& S, float &V, float &C); // convert RGB value to HSV
void HSVtoRGB(const float &H, const float &S, const float &V, float &R, float &G, float &B); // convert HSV value to RGB

void RGBtoHSL(const float &R, const float &G, const float &B, float &H, float &S, float &L, float &C); // convert RGB value to HSL
void HSLtoRGB(const float &H, const float &S, const float &L, float &R, float &G, float &B); // convert HSL value to RGB

void RGBtoXYZ(const float &R, const float &G, const float &B, float &X, float &Y, float &Z); // convert RGB value to CIE XYZ
void XYZtoRGB(const float &X, const float &Y, const float &Z, float &R, float &G, float &B); // convert from XYZ to RGB

void XYZtoLAB(const float &X, const float &Y, const float &Z, float &L, float &A, float &B); // convert CIE XYZ value to CIE LAB
void LABtoXYZ(const float &L, const float &a, const float &b, float &X, float &Y, float &Z); // convert CIE LAB value to CIE XYZ

void XYZtoHLAB(const float &X, const float &Y, const float &Z, float &L, float &A, float &B); // convert from XYZ to Hunter Lab
void HLABtoXYZ(const float &L, const float &A, const float &B, float &X, float &Y, float &Z); // convert from Hunter Lab to XYZ

void HSVtoHWB(const float &h, const float &s, const float &v, float &H, float &W, float &B); // convert HSV value to HWB
void HWBtoHSV(const float &h, const float &w, const float &b, float &H, float &S, float &V); // convert HWB vlaue to HSV

void RGBToHWB(const float &r, const float &g, const float &b, float &H, float &W, float &B); // convert RGB value to HWB
void HWBtoRGB(const float &h, const float &w, const float &b, float &R, float &G, float &B); // convert HWB value to RGB

void LABtoLCH(const float &A, const float &B, float &C, float &H); // convert from LAB to LCH - L is the same so no need to convert
void LCHtoLAB(const float &H, const float &C, float &A, float &B); // convert from LCH to LAB - L is the same so no need to convert

#endif // DOMINANT_H
