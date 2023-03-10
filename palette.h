/*#-------------------------------------------------
#
#          Palette structure for color spaces
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.0 - 2021/04/14
#
#-------------------------------------------------*/

#ifndef PALETTE_H
#define PALETTE_H

#include <string>

//// palette

struct struct_rgb {
    double R; // range [0..1] - Red - display [0..255]
    double G; // range [0..1] - Green - display [0..255]
    double B; // range [0..1] - Blue - display [0..255]
};
struct struct_hsl {
    double H; // range [0..1] - Hue - display [0..360]
    double S; // range [0..1] - Saturation - display [0..100]
    double L; // range [0..1] - Lightness - display [0..100]
    double C; // range [0..1] - Chroma - display [0..100]
};
struct struct_hsv {
    double H; // range [0..1] - Hue - display [0..360]
    double S; // range [0..1] - Saturation - display [0..100]
    double V; // range [0..1] - Value - display [0..100]
    double C; // range [0..1] - Chroma - display [0..100]
};
struct struct_hwb {
    double H; // range [0..1] - Hue - display [0..360]
    double W; // range [0..1] - White - display [0..100]
    double B; // range [0..1] - Black - display [0..100]
};
struct struct_xyz {
    double X; // range [0..1] - mix of response curves - display [0..100]
    double Y; // range [0..1] - luminance - display [0..100]
    double Z; // range [0..1] - quasi-equal to blue - display [0..100]
};
struct struct_xyy {
    double x; // range [0..1] - chromaticity - display [0..1]
    double y; // range [0..1] - chromaticity - display [0..1]
    double Y; // range [0..1] - luminance - display [0..100]
};
struct struct_lab {
    double L; // range [0..1]  - Lightness - display [0..100]
    double A; // range [-1..1] - red/magenta to green - display [-128..127]
    double B; // range [-1..1] - yellow to blue - display [-128..127]
};
struct struct_hlab {
    double L; // range [0..1]  - Lightness - display [0..100]
    double A; // range [-1..1] - red/magenta to green - display [-100..100]
    double B; // range [-1..1] - yellow to blue - display [-100..100]
};
struct struct_lchab {
    double L; // range [0..1] - Lightness - display [0..100]
    double C; // range [0..1] - Chroma - display [0..100]
    double H; // range [0..1] - Hue - display [0..360]
};
struct struct_lchuv {
    double L; // range [0..1] - Lightness - display [0..100]
    double C; // range [0..1] - Chroma - display [0..100]
    double H; // range [0..1] - Hue - display [0..360]
};
struct struct_lms {
    double L; // range [0..1] - Long - display [0..100]
    double M; // range [0..1] - Medium - display [0..100]
    double S; // range [0..1] - Short - display [0..100]
};
struct struct_luv {
    double L; // range [0..1] - Lightness - display [0..100]
    double u; // range [0..1] - chromaticity - display [-134..220]
    double v; // range [0..1] - chromaticity - display [-140..122]
};
struct struct_cmyk {
    double C; // range [0..1] - Cyan - display [0..100]
    double M; // range [0..1] - Magenta - display [0..100]
    double Y; // range [0..1] - Yellow - display [0..100]
    double K; // range [0..1] - Black - display [0..100]
};
struct struct_oklab {
    double L; // range [0..1]  - Lightness - display [0..100]
    double A; // range [-1..1] - red/magenta to green - display [-128..127]
    double B; // range [-1..1] - yellow to blue - display [-128..127]
};
struct struct_oklch {
    double L; // range [0..1] - Lightness - display [0..100]
    double C; // range [0..1] - Chroma - display [0..100]
    double H; // range [0..1] - Hue - display [0..360]
};

struct struct_palette { // structure of a color value
    struct_rgb RGB; // all known color spaces
    struct_hsl HSL;
    struct_hsv HSV;
    struct_hwb HWB;
    struct_xyz XYZ;
    struct_xyy XYY;
    struct_lab CIELAB;
    struct_hlab HLAB;
    struct_lchab LCHAB;
    struct_lchuv LCHUV;
    struct_oklab OKLAB;
    struct_oklch OKLCH;
    struct_lms LMS;
    struct_luv LUV;
    struct_cmyk CMYK;
    std::string hexa;
    int count; // occurences
    double percentage; // percentage
    std::string name; // name of color
    bool selected; // selection indicator
    bool visible; // visibility indicator
};

#endif // PALETTE_H
