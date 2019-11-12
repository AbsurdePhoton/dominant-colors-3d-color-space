/*#-------------------------------------------------
#
#            Color spaces conversions
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.0 - 2019/11/08
#
#  Color spaces :
#    - RGB
#    - CIE XYZ
#    - CIE xyY
#    - CIE L*A*B* and CIE LCHab
#    - CIE L*u*v* and CIE LCHuv
#    - HSL
#    - HSV
#    - HWB
#    - Hunter Lab
#    - LMS
#
#-------------------------------------------------*/

#include <algorithm>
#include "color-spaces.h"
#include "mat-image-tools.h"

using namespace std;

/////////////////// Color spaces conversions //////////////////////
//// All values are in range [0..1]

void WavelengthToXYZ(const double w, double &X, double &Y, double &Z) // wavelength to XYZ color space
{
    int n = -1; // index of array

    for (int W = 0; W < wavelength_XYZ_nb; W++) // all values
        if (int(w) == wavelength_XYZ[W][0]) { // wavelength in array ?
            n = W; // index of line
            break; // found
        }

    if (n == -1) { // not found ?
        X = 0;
        Y = 0;
        Z = 0;
    }
    else { // found => return values
        X = wavelength_XYZ[n][1];
        Y = wavelength_XYZ[n][2];
        Z = wavelength_XYZ[n][3];
    }
}

void SpectralColorToRGB(const double &L, double &R, double &G, double &B) // roughly convert wavelength value 400-700 nm to RGB [0..1]
{
    double t;
    R = 0.0;
    G = 0.0;
    B = 0.0;

         if ((L >= 400.0)&&(L<410.0)) { t = (L-400.0)/(410.0-400.0); R =     +(0.33*t)-(0.20*t*t); }
    else if ((L >= 410.0)&&(L<475.0)) { t = (L-410.0)/(475.0-410.0); R = 0.14         -(0.13*t*t); }
    else if ((L >= 545.0)&&(L<595.0)) { t = (L-545.0)/(595.0-545.0); R =     +(1.98*t)-(     t*t); }
    else if ((L >= 595.0)&&(L<650.0)) { t = (L-595.0)/(650.0-595.0); R = 0.98+(0.06*t)-(0.40*t*t); }
    else if ((L >= 650.0)&&(L<700.0)) { t = (L-650.0)/(700.0-650.0); R = 0.65-(0.84*t)+(0.20*t*t); }

         if ((L >= 415.0)&&(L<475.0)) { t = (L-415.0)/(475.0-415.0); G =              +(0.80*t*t); }
    else if ((L >= 475.0)&&(L<590.0)) { t = (L-475.0)/(590.0-475.0); G = 0.8 +(0.76*t)-(0.80*t*t); }
    else if ((L >= 585.0)&&(L<639.0)) { t = (L-585.0)/(639.0-585.0); G = 0.84-(0.84*t)           ; }

         if ((L >= 400.0)&&(L<475.0)) { t = (L-400.0)/(475.0-400.0); B =     +(2.20*t)-(1.50*t*t); }
    else if ((L >= 475.0)&&(L<560.0)) { t = (L-475.0)/(560.0-475.0); B = 0.7 -(     t)+(0.30*t*t); }
}

//// HSV
//// see https://en.wikipedia.org/wiki/HSL_and_HSV
//// All values [0..1]
//// Common range : H [0..359] S [0..100] V [0..100]

void RGBtoHSV(const double &R, const double &G, const double &B, double& H, double& S, double &V, double &C) // convert RGB value to HSV+C
{ // function checked OK with other calculators
    double cmax = max(max(R, G), B);    // maximum of RGB
    double cmin = min(min(R, G), B);    // minimum of RGB
    double diff = cmax-cmin;       // diff of cmax and cmin.

    if (diff > 0) { // not particular case of diff = 0 -> find if R G or B is dominant
        if (cmax == R) // R is dominant
            H = 60.0 * (fmod(((G - B) / diff), 6)); // compute H
        else if (cmax == G) // G is dominant
            H = 60 * (((B - R) / diff) + 2); // compute H
        else if (cmax == B) // B is dominant
            H = 60 * (((R - G) / diff) + 4); // compute H

        if (cmax > 0) // compute S
            S = diff / cmax;
        else
            S = 0;

        V = cmax; // compute V
    }
    else { // particular case -> H = red (convention)
        H = 0;
        S = 0;
        V = cmax;
    }

    if (H < 0) // H must be in [0..360] range
        H += 360;
    if (H >= 360)
        H -= 360;

    // Final results are in range [0..1]
    H = H / 360.0; // was in degrees
    C = diff; // chroma
}

void HSVtoRGB(const double &H, const double &S, const double &V, double &R, double &G, double &B) // convert HSV value to RGB
{ // function checked OK with other calculators
  double C = V * S; // chroma
  double HPrime = H * 360.0 / 60.0; // dominant 6th part of H - H must be in [0..360]
  double X = C * (1 - abs(fmod(HPrime, 2) - 1)); // intermediate value
  double M = V - C; // difference to add at the end

  // for each part its calculus
  if (0 <= HPrime && HPrime < 1) {
    R = C;
    G = X;
    B = 0;
  }
  else if (1 <= HPrime && HPrime < 2) {
    R = X;
    G = C;
    B = 0;
  }
  else if (2 <= HPrime && HPrime < 3) {
    R = 0;
    G = C;
    B = X;
  }
  else if (3 <= HPrime && HPrime < 4) {
    R = 0;
    G = X;
    B = C;
  }
  else if (4 <= HPrime && HPrime < 5) {
    R = X;
    G = 0;
    B = C;
  }
  else if(5 <= HPrime && HPrime < 6) {
    R = C;
    G = 0;
    B = X;
  } else {
    R = 0;
    G = 0;
    B = 0;
  }

  R += M; // final results
  G += M;
  B += M;
}

//// HSL
//// see https://en.wikipedia.org/wiki/HSL_and_HSV
//// All values [0..1]
//// Common range : H [0..359] S [0..100] L [0..100]

void RGBtoHSL(const double &R, const double &G, const double &B, double &H, double &S, double &L, double &C) // convert RGB value to HSL
{ // function checked OK with other calculators
    double cmax = max(max(R, G), B);    // maximum of RGB
    double cmin = min(min(R, G), B);    // minimum of RGB
    double diff = cmax - cmin;       // diff of cmax and cmin.

    L = (cmax + cmin) / 2.0; // middle of range

    if(cmax == cmin) // particular case : color is a gray
    {
        S = 0;
        H = 0;
    }
    else {
        if (L < .50) // result depends on Lightness
            S = diff / (cmax + cmin); // compute S
        else
            S = diff / (2 - cmax - cmin); // compute S

        // which is the dominant in R, G, B
        if (cmax == R) // red
            H = (G - B) / diff; // compute H
        if (cmax == G) // green
            H = 2 + (B - R) / diff; // compute H
        if (cmax == B) // blue
            H = 4 + (R - G) / diff; // compute H
    }

    H = H * 60; // H in degrees

    if (H < 0) // H in [0..360]
        H += 360;
    if (H >= 360)
        H -= 360;

    // Final results in range [0..1]
    H = H / 360.0; // was in degrees
    C = diff; // Chroma
}

double HueToRGB(const double &v1, const double &v2, const double &H) // Convert Hue to R, G or B value for HSLtoRGB function
{
    double vH = H;

    if (vH < 0) vH += 1; // H must be in range [0..1]
    if (vH > 1) vH -= 1;

    if ((6 * vH) < 1) // which component (R, G, B) to compute ?
        return v1 + (v2 - v1) * 6.0 * vH;
    if ((2 * vH) < 1 )
        return v2;
    if ((3 * vH) < 2 )
        return (v1 + (v2 - v1) * ((2.0 / 3.0) - vH) * 6.0);
    return (v1);
}

void HSLtoRGB(const double &H, const double &S, const double &L, double &R, double &G, double &B) // convert HSL to RGB value - H is in degrees
{ // function checked OK with other calculators
    if ( S == 0 ) { // color is a gray
        R = L;
        G = L;
        B = L;
    }
    else {
        double var_1, var_2;
        if (L < 0.5) // Result depends on Luminance
            var_2 = L * (1.0 + S);
        else
            var_2 = (L + S) - (S * L);

        var_1 = 2.0 * L - var_2; // first component based on Luminance

        R = HueToRGB(var_1, var_2, H + 1.0 / 3.0); // compute R, G, B
        G = HueToRGB(var_1, var_2, H);
        B = HueToRGB(var_1, var_2, H - 1.0 / 3.0);
    }
}

//// HWB
//// see https://en.wikipedia.org/wiki/HWB_color_model
//// All values [0..1]
//// Common range : H [0..359] W [0..100] B [0..100]

void HSVtoHWB(const double &h, const double &s, const double &v, double &H, double &W, double &B) // convert HSV to HWB
{ // function checked OK with other calculators
    // calculus is simple ! There is a direct relation
    H = h;
    W = (1.0 - s) * v;
    B = 1.0 - v;
}

void RGBtoHWB(const double &r, const double &g, const double &b, double &H, double &W, double &B) // convert RGB to HWB
{ // function checked OK with other calculators
    double h, s, v, c;
    RGBtoHSV(r, g, b, h, s, v, c); // first find HSV
    HSVtoHWB(h, s, v, H, W, B); // then convert HSV to HWB
}

void HWBtoHSV(const double &h, const double &w, const double &b, double &H, double &S, double &V) // convert HWB to HSV
{ // function checked OK with other calculators
    // calculus is simple ! There is a direct relation
    H = h;
    S = 1.0 - (w / (1.0 - b));
    V = 1.0 - b;
}

void HWBtoRGB(const double &h, const double &w, const double &b, double &R, double &G, double &B) // convert HWB to RGB
{ // function checked OK with other calculators
    double H, S, V;
    HWBtoHSV(h, w, b, H, S, V); // first convert to HSV
    HSVtoRGB(H, S, V, R, G, B); // then to RGB
}

//// XYZ
//// See https://en.wikipedia.org/wiki/CIE_1931_color_space
////   and https://fr.wikipedia.org/wiki/CIE_XYZ
//// All values [0..1]
//// Common range for XYZ : [0..100]

void RGBtoXYZ(const double &R, const double &G, const double &B, double &X, double &Y, double &Z) // convert RGB (in fact sRGB) value to CIE XYZ
{ // function checked OK with other calculators
    double r, g, b;

    // Gamma correction - conversion to linear space - source http://www.brucelindbloom.com/index.html?Eqn_RGB_to_XYZ.html
    if (R > 0.04045)
        r = powl((R + 0.055) / 1.055, 2.4);
    else
        r = R / 12.92;
    if (G > 0.04045)
        g = powl((G + 0.055) / 1.055, 2.4);
    else
        g = G / 12.92;
    if (B > 0.04045)
        b = powl((B + 0.055) / 1.055, 2.4);
    else
        b = B / 12.92;

    // Gammut conversion to sRGB - source http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    X = r * 0.4124564 + g * 0.3575761 + b * 0.1804375;
    Y = r * 0.2126729 + g * 0.7151522 + b * 0.0721750;
    Z = r * 0.0193339 + g * 0.1191920 + b * 0.9503041;
}

void XYZtoRGB(const double &X, const double &Y, const double &Z, double &R, double &G, double &B) // convert from XYZ to RGB (in fact sRGB)
{ // function checked OK with other calculators
    // Gammut conversion - source http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    R = X *  3.2404542 + Y * -1.5371385 + Z * -0.4985314;
    G = X * -0.9692660 + Y *  1.8760108 + Z *  0.0415560;
    B = X *  0.0556434 + Y * -0.2040259 + Z *  1.0572252;

    // Gamma profile - source http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    if (R > 0.0031308)
        R = 1.055 * powl(R, 1.0/2.4) - 0.055;
    else
        R = R * 12.92;

    if (G > 0.0031308)
        G = 1.055 * powl(G, 1.0/2.4) - 0.055;
    else
        G = G * 12.92;

    if (B > 0.0031308)
        B = 1.055 * powl(B, 1.0/2.4) - 0.055;
    else
        B = B * 12.92;
}

//// L*A*B*
//// see https://en.wikipedia.org/wiki/CIELAB_color_space for LAB
//// All values [0..1]
//// Common range for L*A*B*: L [0..100] S [-128..127] V [-128..127]

void XYZtoLAB(const double &X, const double &Y, const double &Z, double &L, double &A, double &B) // convert CIE XYZ value to CIE L*A*B*
{ // function checked OK with other calculators
    // reference white in XYZ
    double ref_X = 0.95047;
    double ref_Y = 1.0;
    double ref_Z = 1.08883;
    // CIE values
    double E = 216.0 / 24389.0;
    double K = 24389.0 / 27.0;

    double Xr = X / ref_X;
    double Yr = Y / ref_Y;
    double Zr = Z / ref_Z;

    double fX, fY, fZ;
    if (Xr > E)
        fX = powl(Xr, 1.0 / 3.0);
    else
        fX = (K * Xr + 16.0) / 116.0;
    if (Yr > E)
        fY = powl(Yr, 1.0 / 3.0);
    else
        fY = (K * Yr + 16.0) / 116.0;
    if (Zr > E)
        fZ = powl(Zr, 1.0 / 3.0);
    else
        fZ = (K * Zr + 16.0) / 116.0;

    L = 116.0 * fY - 16.0;
    A = 500.0 * (fX - fY);
    B = 200.0 * (fY - fZ);

    L = L / 100.0; // to stay in [0..1] range
    A = A / 127.0;
    B = B / 127.0;
}

void LABtoXYZ(const double &L, const double &A, const double &B, double &X, double &Y, double &Z) // convert CIE L*A*B* to CIE XYZ
{ // function checked OK with other calculators
    // reference white in XYZ
    double ref_X = 0.95047;
    double ref_Y = 1.0;
    double ref_Z = 1.08883;
    // CIE values
    double E = 216.0 / 24389.0;
    double K = 24389.0 / 27.0;

    double fY = (L * 100.0 + 16.0) / 116.0;
    double fZ = fY - B * 127.0 / 200.0;
    double fX = A * 127.0 / 500.0 + fY;

    double Xr, Yr, Zr;

    if (powl(fX, 3.0) > E)
        Xr = powl(fX, 3.0);
    else
        Xr = (116.0 * fX -16.0) / K;
    if (L * 100.0 > K * E)
        Yr = powl((L * 100.0 + 16.0) / 116.0, 3.0);
    else
        Yr = L * 100.0 / K;
    if (powl(fZ, 3.0) > E)
        Zr = powl(fZ, 3.0);
    else
        Zr = (116.0 * fZ -16.0) / K;

    X = Xr * ref_X;
    Y = Yr * ref_Y;
    Z = Zr * ref_Z;
}

//// CIE xyY
//// see https://en.wikipedia.org/wiki/CIE_1931_color_space#CIE_xy_chromaticity_diagram_and_the_CIE_xyY_color_space
//// All values [0..1]
//// Common range for xyY: x [0..1] y [0..1] Y [0..100]
//// Note that Y is exactly equal to Y in CIE XYZ

void XYZtoxyY(const double &X, const double &Y, const double &Z, double &x, double &y) // convert CIE XYZ value to CIE xyY
{ // function checked OK with other calculators
    if ((X == 0) and (Y == 0) and (Z == 0)) { // D65 white point
        x = 0.3127;
        y = 0.3290;
    }
    else { // from formula X + Y + Z = 1
        x = X / (X + Y +Z);
        y = Y / (X + Y +Z);
    }
}

void xyYtoXYZ(const double &x, const double &y, const double &Y, double &X, double &Z) // convert CIE xyY value to CIE XYZ
{ // function checked OK with other calculators
    if (Y == 0) { // Y is lightness so if 0 => no color
        X = 0;
        Z = 0;
    }
    else { // from formula X + Y + Z = 1
        X = x * Y / y;
        Z = (1 - x - y) * Y / y;
    }
}

//// L*u*v*
//// see https://en.wikipedia.org/wiki/CIELUV
//// All values [0..1]
//// Common range for L*u*v*: L* [0..100] u* [-134..220] v* [-140..122]

void XYZtoLuv(const double &X, const double &Y, const double &Z, double &L, double &u, double &v) // convert CIE XYZ value to CIE L*u*v*
{ // function checked OK with other calculators
    // reference white in XYZ
    double ref_X = 0.95047;
    double ref_Y = 1.0;
    double ref_Z = 1.08883;
    // CIE values
    double E = 216.0 / 24389.0;
    double K = 24389.0 / 27.0;

    if (Y / ref_Y > E) // two-part equation
        L = 116.0 * powl(Y / ref_Y, 1.0 / 3.0) - 16.0;
    else
        L = K * Y / ref_Y;

    double u_prime = 4.0 * X / (X + 15.0 * Y + 3.0 * Z); // intermediate calculus
    double u_ref = 4.0 * ref_X / (ref_X + 15.0 * ref_Y + 3.0 * ref_Z);
    double v_prime = 9.0 * Y / (X + 15.0 * Y + 3.0 * Z);
    double v_ref = 9.0 * ref_Y / (ref_X + 15.0 * ref_Y + 3.0 * ref_Z);

    u = 13.0 * L * (u_prime - u_ref); // final values
    v = 13.0 * L * (v_prime - v_ref);

    L = L / 100.0; // to stay in range [0..1]
    u = u / 100.0;
    v = v / 100.0;
}

void LuvToXYZ(const double &L, const double &U, const double &V, double &X, double &Y, double &Z) // convert CIE L*u*v* value to CIE XYZ
{ // function checked OK with other calculators
    // reference white in XYZ
    double ref_X = 0.95047;
    double ref_Y = 1.0;
    double ref_Z = 1.08883;
    // CIE values
    double E = 216.0 / 24389.0;
    double K = 24389.0 / 27.0;

    double l = L * 100.0;
    double u = U * 100.0;
    double v = V * 100.0;

    double u0 = 4.0 * ref_X / (ref_X + 15.0 * ref_Y + 3.0 * ref_Z); // white point intermediate values
    double v0 = 9.0 * ref_Y / (ref_X + 15.0 * ref_Y + 3.0 * ref_Z);

    double u_prime = u / (13.0 * l) + u0;
    double v_prime = v / (13.0 * l) + v0;

    if (l > K * E)
        Y = ref_Y * powl((l + 16.0) / 116.0, 3);
    else
        Y = ref_Y * l * (powl(3.0 / 29.0, 3));

    X = Y * 9.0 * u_prime / 4.0 / v_prime;
    Z = Y * (12.0 - 3 * u_prime - 20 * v_prime) / 4.0 / v_prime;
}

//// Hunter Lab (HLAB)
//// See https://en.wikipedia.org/wiki/CIELAB_color_space#Hunter_Lab
//// All values [0..1]
//// Common range : L [0..100] a [-100..100] b [-100..100]

void XYZtoHLAB(const double &X, const double &Y, const double &Z, double &L, double &A, double &B) // convert from XYZ to Hunter Lab
{ // function checked OK with other calculators
    if (Y == 0) { // lightness is 0 => no color
        L = 0;
        A = 0;
        B = 0;
    }
    else {
        // reference white in XYZ
        double ref_X = 0.95047;
        double ref_Y = 1.0;
        double ref_Z = 1.08883;

        double Ka = (175.0 / 198.04) * (ref_X + ref_Y); // CIE standard values VS white point
        double Kb = ( 70.0f / 218.11) * (ref_Y + ref_Z);

        L = sqrtl(Y / ref_Y); // final values
        A = Ka * (((X / ref_X) - (Y / ref_Y)) / sqrtl(Y / ref_Y));
        B = Kb * (((Y / ref_Y) - (Z / ref_Z)) / sqrtl(Y / ref_Y));
    }
}

void HLABtoXYZ(const double &L, const double &A, const double &B, double &X, double &Y, double &Z) // convert from Hunter Lab to XYZ
{ // function checked OK with other calculators
    // reference white in XYZ
    double ref_X = 0.95047;
    double ref_Y = 1.0;
    double ref_Z = 1.08883;

    double Ka = (175.0 / 198.04) * (ref_Y + ref_X); // CIE standard values VS white point
    double Kb = ( 70.0f / 218.11) * (ref_Y + ref_Z);

    Y = powl(L / ref_Y, 2); // final values
    X =  (A / Ka * sqrtl(Y / ref_Y) + (Y / ref_Y)) * ref_X;
    Z = -(B / Kb * sqrtl(Y / ref_Y) - (Y / ref_Y)) * ref_Z;

}

//// CIE LCHab
//// See https://en.wikipedia.org/wiki/CIELAB_color_space#Cylindrical_representation:_CIELCh_or_CIEHLC
//// All values [0..1]
//// Common range : L [0..100] C [0..100] h [0..360]
//// Note that L is exactly equal to L of CIE L*a*b*

void LABtoLCHab(const double &A, const double &B, double &C, double &H) // convert from LAB to HLC - L is the same so no need to convert
{ // function checked OK with other calculators
    C = sqrtl(powl(A * 127.0, 2) + powl(B * 127.0, 2)) / 100.0; // Chroma

    H = atan2l(B, A) / 2.0 / Pi; // Hue - cartesian to polar
    while (H < 0) // Hue in range [0..1]
        H += 1;
    while (H > 1)
        H -= 1;
}

void LCHabToLAB(const double &C, const double &H, double &A, double &B) // convert from HLC to LAB - L is the same so no need to convert
{ // function checked OK with other calculators
    A = C * cosl(H * 2.0 * Pi); // polar to cartesian
    B = C * sinl(H * 2.0 * Pi);
}

//// CIE LCHuv
//// See https://en.wikipedia.org/wiki/CIELUV#Cylindrical_representation_(CIELCH)
//// All values [0..1]
//// Common range : L [0..100] C [0..100] h [0..360]
//// Note that L is exactly equal to L of CIE Luv

void LUVtoLCHuv(const double &u, const double &v, double &C, double &H) // convert from Luv to LCHuv - L is the same so no need to convert
{ // function checked OK with other calculators
    C = sqrtl(powl(u * 100.0, 2) + powl(v * 100.0, 2)) / 100.0; // Chroma

    H = atan2l(v, u) / 2.0 / Pi; // Hue - cartesian to polar
    while (H < 0) // Hue in range [0..1]
        H += 1;
    while (H > 1)
        H -= 1;
}

void LCHuvToLUV(const double &C, const double &H, double &u, double &v) // convert from LCHuv to LUV - L is the same so no need to convert
{ // function checked OK with other calculators
    u = C * cosl(H * 2.0 * Pi); // polar to cartesian
    v = C * sinl(H * 2.0 * Pi);
}

//// LMS
//// See https://en.wikipedia.org/wiki/LMS_color_space
//// All values [0..1]
//// Common range : no range specified

void XYZtoLMS(const double &X, const double &Y, const double &Z, double &L, double &M, double &S) // convert from XYZ to LMS
{ // couldn't find any online calculator to check this function, but it is pretty straightforward
    // CIECAM02 is the successor to CIECAM97s - the best matrix so far, just coordinates change
    L = 0.7328  * X + 0.4296 * Y - 0.1624 * Z;
    M = -0.7036 * X + 1.6975 * Y + 0.0061 * Z;
    S = 0.0030  * X + 0.0136 * Y + 0.9834 * Z;
}
