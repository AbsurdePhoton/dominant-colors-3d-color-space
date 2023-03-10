/*#-------------------------------------------------
#
#         Color spaces conversions library
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v2.1 - 2022/12/07
#
#  Color spaces : all additive except RYB
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
#    - CYMK
#    - RYB (substractive)
#    - OKLAB
#
#  + RGB and CIELAb and OKLAB color utils
#  + subtractive mixing by reflectance
#
#-------------------------------------------------*/

#include "color-spaces.h"

std::vector<double> RGBlinearLUT = InitRGBLinearLUT(); // initialize the linear RGB LUT at startup


///////////////////////////////////////////////////////////
//// General
///////////////////////////////////////////////////////////

std::vector<double> InitRGBLinearLUT() // populate RGB linear LUT - it is called at startup to populate the linear RGB LUT used by any conversion function using RGB to linear RGB (CIELab, OKLab, etc)
{
    std::vector<double> array;
    double N;
    for (int n = 0; n < 256; n++) {
        N = n / 255.0;
        if (N > 0.04045)
            array.push_back(pow((N + 0.055) / 1.055, 2.4));
        else
            array.push_back(N / 12.92);
    }

    return array;
}

double GetValueRangeZeroOne(const double &val)
{
    if (val < 0.0)
        return 0.0;
    else if (val > 1.0)
        return 1.0;
    else
        return val;
}

///////////////////////////////////////////////////////////
//// Color distance
///////////////////////////////////////////////////////////
// All input values are in range [0..1]

double EuclideanDistanceSpace(const double &x1, const double &y1, const double &z1,
                              const double &x2, const double &y2, const double &z2) // Euclidean distance in 3 dimensions (vector length)
{
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) + (z1 - z2) * (z1 - z2)); // Euclidean distance formula sqrt(x²+y²+z²) (vector)
}

double EuclideanDistanceSpaceFast(const double &x1, const double &y1, const double &z1,
                                  const double &x2, const double &y2, const double &z2) // is faster because no sqrt operation - useful for comparing distances
{
    return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) + (z1 - z2) * (z1 - z2); // Euclidean distance formula sqrt(x²+y²+z²)
}

double EuclideanDistancePlane(const double &x1, const double &y1, const double &x2, const double &y2) // Euclidean distance in 2 dimensions (vector length)
{
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)); // Euclidean distance formula sqrt(x²+y²)
}

double EuclideanDistancePlaneFast(const double &x1, const double &y1, const double &x2, const double &y2) // is faster because no sqrt operation - useful for comparing distances
{
    return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2); // Euclidean distance formula sqrt(x²+y²)
}

double ToroidalDistancePlane (const double &x1, const double &y1, const double &x2, const double &y2, const int &width, const int &height) // Euclidian distance in a wrapped plane ("tiled")
{
    double dX = abs(x2 - x1);
    double dY = abs(y2 - y1);

    if (dX > width / 2)
        dX = width - dX;

    if (dY > height / 2)
        dY = height - dY;

    return sqrt(dX * dX + dY * dY);
}

double ToroidalDistancePlaneFast (const double &x1, const double &y1, const double &x2, const double &y2, const int &width, const int &height) // is faster because no sqrt operation - useful for comparing distances
{
    double dX = abs(x2 - x1);
    double dY = abs(y2 - y1);

    if (dX > width / 2)
        dX = width - dX;

    if (dY > height / 2)
        dY = height - dY;

    return dX * dX + dY * dY;
}

double DistanceCIEDE2000(const double &L1, const double &A1, const double &B1,
                            const double &L2, const double &A2, const double &B2,
                            const double &k_L, const double &k_C, const double &k_H) // distance in CIELab space using lastest (and best) formula (CIEDE2000) compared to CIE94 and CIE76 - it is SLOW to compute
{
    // Adapted from Gregory Fiumara
    // source : http://github.com/gfiumara/CIEDE2000
    // Based on "The CIEDE2000 Color-Difference Formula: Implementation Notes, Supplementary Test Data, and Mathematical Observations" by Gaurav Sharma, Wencheng Wu, and Edul N. Dalal
    // Results checked against all article values page 24 : OK
    // "For these and all other numerical/graphical delta E00 values reported in this article, we set the parametric weighting factors to unity(i.e., k_L = k_C = k_H = 1.0)." (p27)
    // This function is SLOW, for speed use Euclidean distance

    // Step 0 : convert values to correct ranges : L, a and b must be in [0..100]
    const double l1 = L1 * 100.0;
    const double l2 = L2 * 100.0;
    const double a1 = A1 * 100.0;
    const double a2 = A2 * 100.0;
    const double b1 = B1 * 100.0;
    const double b2 = B2 * 100.0;

    // Remark : all angles in equations are expressed in degrees, but atan, cos and sin need radians !

    // Step 1
    // equation 2 : chromas (it's a distance !)
    double C1 = sqrt((a1 * a1) + (b1 * b1));
    double C2 = sqrt((a2 * a2) + (b2 * b2));
    // equation 3 : chromas mean
    double pow_barC_7 = pow((C1 + C2) / 2.0, 7.0);
    // equation 4 : G
    double G = 0.5 * (1.0 - sqrt(pow_barC_7 / (pow_barC_7 + pow_25_7)));
    // equation 5 : a
    double a1Prime = (1.0 + G) * a1;
    double a2Prime = (1.0 + G) * a2;
    // equation 6 : C' from LCH
    double CPrime1 = sqrt((a1Prime * a1Prime) + (b1 * b1));
    double CPrime2 = sqrt((a2Prime * a2Prime) + (b2 * b2));
    // equation 7 : H' from LCH
    double hPrime1;
    if ((b1 == 0.0) and (a1Prime == 0.0))
        hPrime1 = 0.0;
    else {
        hPrime1 = atan2(b1, a1Prime);
        if (hPrime1 < 0.0)
            hPrime1 += pi_rad; // add 2 * Pi radians
        hPrime1 *= pi_deg; // convert to degrees
    }

    double hPrime2;
    if ((b2 == 0.0) and (a2Prime == 0.0))
        hPrime2 = 0.0;
    else {
        hPrime2 = atan2(b2, a2Prime);
        if (hPrime2 < 0.0)
            hPrime2 += pi_rad; // add 2 * Pi radians
        hPrime2 *= pi_deg; // convert to degrees
    }

    // Step 2
    // equation 8 : delta L
    double deltaLPrime = l2 - l1;
    // equation 9 : delta C'
    double deltaCPrime = CPrime2 - CPrime1;
    // equation 10 : delta h'
    double deltahPrime;
    double CPrimeProduct = CPrime1 * CPrime2;

    if (CPrimeProduct == 0.0)
        deltahPrime = 0.0;
    else {
        deltahPrime = hPrime2 - hPrime1;
        if (deltahPrime < -180.0)
            deltahPrime += 360.0;
        else if (deltahPrime > 180.0)
            deltahPrime -= 360.0;
    }
    // Equation 11 : delta H'
    double deltaHPrime = 2.0 * sqrt(CPrimeProduct) * sin(DegToRad(deltahPrime) / 2.0);

    // Step 3
    // equation 12 : L mean
    double barLPrime = (l1 + l2) / 2.0;
    // equation 13 : C mean
    double barCPrime = (CPrime1 + CPrime2) / 2.0;
    // equation 14 : bar h'
    double barhPrime, hPrimeSum = hPrime1 + hPrime2;

    if (CPrime1 * CPrime2 == 0) {
        barhPrime = hPrimeSum;
    } else {
        if (abs(hPrime1 - hPrime2) <= 180.0)
            barhPrime = hPrimeSum / 2.0;
        else {
            if (hPrimeSum < 360.0)
                barhPrime = (hPrimeSum + 360.0) / 2.0;
            else
                barhPrime = (hPrimeSum - 360.0) / 2.0;
        }
    }
    // equation 15 : T
    double T = 1.0 - (0.17 * cos(DegToRad(barhPrime) - deg2rad_30))
                       + (0.24 * cos(2.0 * DegToRad(barhPrime)))
                       + (0.32 * cos((3.0 * DegToRad(barhPrime)) + deg2rad_6))
                       - (0.20 * cos((4.0 * DegToRad(barhPrime)) - deg2rad_63));
    // equation 16 : delta theta
    double deltaTheta = deg2rad_30 * exp(-pow((DegToRad(barhPrime) - deg2rad_275) / deg2rad_25, 2.0));
    // equation 17
    double pow_barCPrime_7 = pow(barCPrime, 7.0);
    double R_C = 2.0 * sqrt(pow_barCPrime_7 / (pow_barCPrime_7 + pow_25_7));
    // equation 18
    double S_L = 1.0 + 0.015 * (barLPrime - 50.0) * (barLPrime - 50.0) / sqrt(20.0 + (barLPrime - 50.0) * (barLPrime - 50.0));
    // equation 19
    double S_C = 1.0 + 0.045 * barCPrime;
    // equation 20
    double S_H = 1.0 + 0.015 * barCPrime * T;
    // equation 21
    double R_T = -sin(2.0 * deltaTheta) * R_C;

    // equation 22 : delta E (distance)
    double deltaE = sqrt(deltaLPrime / (k_L * S_L) * deltaLPrime / (k_L * S_L)
                       + deltaCPrime / (k_C * S_C) * deltaCPrime / (k_C * S_C)
                       + deltaHPrime / (k_H * S_H) * deltaHPrime / (k_H * S_H)
                       + (R_T * (deltaCPrime / (k_C * S_C)) * (deltaHPrime / (k_H * S_H))));

    return (deltaE);
}

double DistanceCIEDE2000Revised(const double &L1, const double &A1, const double &B1,
                         const double &L2, const double &A2, const double &B2,
                         const double &kL, const double &kC, const double &kH,
                         const bool &original) // revised CIEDE2000 that preserves triangle inequality and continuity - new formula a bit slower than original (choose with original=false)
    // see https://rangevoting.org/ColorPack.html for details
{
    // Step 0: convert values to correct ranges : L, a and b must be in [0..100]
    const double l1 = L1 * 100.0;
    const double l2 = L2 * 100.0;
    const double a1 = A1 * 100.0;
    const double a2 = A2 * 100.0;
    const double b1 = B1 * 100.0;
    const double b2 = B2 * 100.0;
    double C1p, C2p, h1p, h2p;
    double C1, G, pow_aC1C2_7, a1p, a2p, C2, dhp, dHHp; /*note dhp and dHHp differ*/
    double aL, aCp, ahp, T, dro, RC, SL, SC, SH, RT, dE00, dLp, dCp, mul, REPdHHp;

    // Step 1: compute C1p, C2p, h1p, h2p
    C1 = sqrt(a1 * a1 + b1 * b1); // (EQ2)
    C2 = sqrt(a2 * a2 + b2 * b2); // (EQ2)

    pow_aC1C2_7 = pow((C1 + C2) * 0.5, 7.0); // (EQ3)

    G = 0.5 * (1.0 - sqrt(pow_aC1C2_7 / (pow_aC1C2_7 + 6103515625.0))); //(EQ4)

    a1p = (1.0 + G) * a1; // (EQ5)
    a2p = (1.0 + G) * a2; // (EQ5)

    C1p = sqrt(a1p * a1p + b1 * b1); // (EQ6)
    C2p = sqrt(a2p * a2p + b2 * b2); // (EQ6)

    //h1p = hpf(b1, a1p); // (EQ7)
    //h2p = hpf(b2, a2p); // (EQ7)

    if ((b1 == 0.0) and (a1p == 0.0))
        h1p = 0.0;
    else {
        h1p = atan2(b1, a1p);
        if (h1p < 0.0)
            h1p += pi_rad; // add 2 * Pi radians
        h1p *= pi_deg; // convert to degrees
    }

    if ((b2 == 0.0) and (a2p == 0.0))
        h2p = 0.0;
    else {
        h2p = atan2(b2, a2p);
        if (h2p < 0.0)
            h2p += pi_rad; // add 2 * Pi radians
        h2p *= pi_deg; // convert to degrees
    }

    // Step 2: compute dLp, dCp, dHHp
    dLp = l2 - l1; // (EQ8)

    dCp = C2p - C1p; // (EQ9)

    //dhp = dhpf(C1,C2, h1p, h2p); // (EQ10) hue difference
    double CPrimeProduct = C1p * C2p;
    if (CPrimeProduct == 0.0)
        dhp = 0.0;
    else {
        dhp = h2p - h1p;
        if (dhp < -180.0)
            dhp += 360.0;
        else if (dhp > 180.0)
            dhp -= 360.0;
    }

    dHHp = 2.0 * sqrt(C1p * C2p) * sin(DegToRad(dhp * 0.5)); // (EQ11)

    // Step 3: Calculate CIEDE2000 Color-Difference
    aL = (l1 + l2) * 0.5; // (EQ12)

    aCp = (C1p + C2p) * 0.5; // (EQ13)

    //ahp = ahpf(C1, C2, h1p, h2p); // (EQ14) mean hue
    double hPrimeSum = h1p + h2p;

    if (C1p * C2p == 0) {
        ahp = hPrimeSum;
    } else {
        if (abs(h1p - h2p) <= 180.0)
            ahp = hPrimeSum / 2.0;
        else {
            if (hPrimeSum < 360.0)
                ahp = (hPrimeSum + 360.0) / 2.0;
            else
                ahp = (hPrimeSum - 360.0) / 2.0;
        }
    }

    if (original) {
        dro = 30.0 * exp(-(ahp - 275.0) / 25.0 * (ahp - 275.0) / 25.0); //(EQ16) very tiny discontinuity
        T = 1 - 0.17 * cos(DegToRad(ahp - 30.0)) + 0.24 * cos(DegToRad(2.0 * ahp)) + 0.32 * cos(DegToRad(3.0 * ahp + 6.0)) - 0.20 * cos(DegToRad(4 * ahp - 63)); // (EQ15)
    }
    else {
        T = 1 + 0.24 * cos(DegToRad(2 * ahp)) - 0.20 * cos(DegToRad(4 * ahp - 63.0))
            + (0.32 * cos(DegToRad(3 * ahp + 6)) - 0.17 * cos(DegToRad(ahp - 30.0))) * cos(DegToRad(dhp * 0.5)); /*repaired (EQ15)*/
        dro = ahp - 275.0;
        if (fabs(dro) >= 85.0) {
            dro = 0.0;
        }
        else {
            dro = dro / 25.0 * dro / 25.0;
            mul = 1.0 - dro * 25.0 / 289.0;
            dro = 30.0 * exp(-dro) * mul; //(EQ16) as now modified to remove discontinuity
        }
    }

    double pow_aCp_7 = pow(aCp, 7.0);
    RC = sqrt(pow_aCp_7 / (pow_aCp_7 + 6103515625.0)); // (EQ17)

    SL = 1.0 + 0.015 * (aL-50) * (aL-50) / sqrt(20.0 + (aL-50) * (aL-50)); // (EQ18)

    SC = 1.0 + 0.045 * aCp; // (EQ19)

    SH = 1.0 + 0.015 * aCp * T; // (EQ20)

    RT = -2.0 * RC * sin(DegToRad(2.0 * dro)); // (EQ21)

    REPdHHp = dHHp;

    if (!original) {
        if (fabs(dhp) > 140.0)
            REPdHHp = dHHp * (180.0 - fabs(dhp)) / 40.0; // repair
            //REPdHHp = dHHp * cos(DegToRad(dhp * 0.5)); // alternative repair which also seems to work
    }

    dE00 = sqrt((dLp / (SL * kL)) * (dLp / (SL * kL)) + (dCp / (SC * kC)) * (dCp / (SC * kC))
            + (dHHp/(SH*kH)) * (dHHp/(SH*kH)) + RT * (dCp / (SC * kC)) * (REPdHHp / (SH * kH))); // (EQ22)

    return dE00;
}

double DistanceCIEDE94(const double &L1, const double &A1, const double &B1,
                            const double &L2, const double &A2, const double &B2,
                            const double &kL, const double &kC, const double &kH) // distance in CIELab space - much faster but way less acurate than CIEDE2000, a bit better than CIEDE76
{
    // convert values to correct ranges : L, a and b must be in [0..100]
    const double l1 = L1 * 100.0;
    const double l2 = L2 * 100.0;
    const double a1 = A1 * 100.0;
    const double a2 = A2 * 100.0;
    const double b1 = B1 * 100.0;
    const double b2 = B2 * 100.0;

    double da = a1 - a2;
    double db = b1 - b2;
    double dL = l1 - l2;
    double C1 = sqrt(a1 * a1 + b1 * b1);
    double C2 = sqrt(a2 * a2 + b2 * b2);
    double dC = C1 - C2;
    double dH = sqrt(da * da + db * db - dC * dC);
    const double SL = 1.0;
    // kL = 1 and k1, k2 defined for graphics arts
    const double k1 = 0.045;
    const double k2 = 0.015;
    double SC = 1.0 + k1 * C1;
    double SH = 1.0 + k2 * C1;
    double dE94 = sqrt(dL / (kL * SL) * dL / (kL * SL)
                        + dC / (kC * SC) * dC / (kC * SC)
                        + dH / (kH * SH) * dH / (kH * SH));

    return dE94;
}

double DistanceCIEDE94Fast(const double &L1, const double &A1, const double &B1,
                                const double &L2, const double &A2, const double &B2,
                                const double &kL, const double &kC, const double &kH) // faster because no sqrt operation - useful for comparing distances
{
    // convert values to correct ranges : L, a and b must be in [0..100]
    const double l1 = L1 * 100.0;
    const double l2 = L2 * 100.0;
    const double a1 = A1 * 100.0;
    const double a2 = A2 * 100.0;
    const double b1 = B1 * 100.0;
    const double b2 = B2 * 100.0;

    double da = a1 - a2;
    double db = b1 - b2;
    double dL = l1 - l2;
    double C1 = sqrt(a1 * a1 + b1 * b1);
    double C2 = sqrt(a2 * a2 + b2 * b2);
    double dC = C1 - C2;
    double dH = sqrt(da * da + db * db - dC * dC);
    const double SL = 1.0;
    // kL = 1 and k1, k2 defined for graphics arts
    const double k1 = 0.045;
    const double k2 = 0.015;
    double SC = 1.0 + k1 * C1;
    double SH = 1.0 + k2 * C1;
    double dE94 = dL / (kL * SL) * dL / (kL * SL)
                        + dC / (kC * SC) * dC / (kC * SC)
                        + dH / (kH * SH) * dH / (kH * SH);

    return dE94;
}

double DistanceCIEDE94Revised(const double &L1, const double &A1, const double &B1,
                                const double &L2, const double &A2, const double &B2,
                                const double &kL, const double &kC, const double &kH) // revised CIEDE94 that preserves triangle inequality and continuity - new formula a bit slower than original
// see https://rangevoting.org/ColorPack.html for details
{
    // convert values to correct ranges : L, a and b must be in [0..100]
    const double l1 = L1 * 100.0;
    const double l2 = L2 * 100.0;
    const double a1 = A1 * 100.0;
    const double a2 = A2 * 100.0;
    const double b1 = B1 * 100.0;
    const double b2 = B2 * 100.0;

    double da = a1 - a2;
    double db = b1 - b2;
    double dL = l1 - l2;
    double C1 = sqrt(a1 * a1 + b1 * b1);
    double C2 = sqrt(a2 * a2 + b2 * b2);
    double dC = C1 - C2;
    double dH = sqrt(da * da + db * db - dC * dC);
    const double SL = 1.0;
    // kL = 1 and k1, k2 defined for graphics arts
    const double k1 = 0.045;
    const double k2 = 0.015;
    const double Cavg = (C1 + C2) / 2.0;
    /*double SC = 1.0 + k1 * C1;
    double SH = 1.0 + k2 * C1;*/
    double SC = 1.0 + k1 * Cavg; // revision : use the average instead of C1 for symmetry
    double SH = 1.0 + k2 * Cavg;
    double dE94 = sqrt(dL / (kL * SL) * dL / (kL * SL)
                        + dC / (kC * SC) * dC / (kC * SC)
                        + dH / (kH * SH) * dH / (kH * SH));
    dE94 = 205.85012080886 * dE94 / (100.0 + pow(dE94, 82.0 / 81.0)); // revision : use this formula for triangle inequality
    //dE94 = 225.85012080886 * dE94 / (120.0 + pow(dE94, 82.0 / 81.0)); // alternative that also seems to work

    return dE94;
}

double DistanceCIEDE76(const double &L1, const double &a1, const double &b1,
                            const double &L2, const double &a2, const double &b2) // CIEDE76 distance - just a euclidean distance with weights
{
    return EuclideanDistanceSpace(L1 * 100.0, a1 * 100.0, b1 * 100.0, L2 * 100.0, a2 * 100.0, b2 * 100.0);
}

double DistanceCIEDE76Fast(const double &L1, const double &a1, const double &b1,
                            const double &L2, const double &a2, const double &b2) // CIEDE76 distance - just a euclidean distance with weights and without sqrt
{
    return EuclideanDistanceSpaceFast(L1 * 100.0, a1 * 100.0, b1 * 100.0, L2 * 100.0, a2 * 100.0, b2 * 100.0);
}

double DistanceCIEAdaptive(const double &L1, const double &a1, const double &b1,
                            const double &L2, const double &a2, const double &b2,
                            const double &kL, const double &kC, const double &kH,
                            const double &threshold) // uses first CIEDE76 then CIEDE2000 if the result is under the threshold (4.6 is twice JND [Just Noticeable Distance])
    // threshold=15 is a good compromise between accuracy and speed
    // JND (just noticeable difference) distance is ~2.3, so setting threshold to twice this should work well
    // "Color-difference evaluation for digital images using a categorical judgment method" by Haoxue Liu & al. evaluated KL=2 KC=3 KH=1 to be the best values
{
    double d = DistanceCIEDE76(L1, a1, b1, L2, a2, b2);
    if (d < threshold)
        return DistanceCIEDE2000Revised(L1, a1, b1, L2, a2, b2, kL, kC, kH, false);
    else
        return d;
}

double DistanceFromBlackRGB(const double &R, const double &G, const double &B) // CIELab distance from RGB(0,0,0)
{
    double X, Y, Z, L, a, b;
    RGBtoXYZ(R, G, B, X, Y, Z); // convert RGB to XYZ
    XYZtoCIELab(X, Y, Z, L, a, b); // convert XYZ to CIELab
    return DistanceCIEAdaptive(L, a, b, 0.0, 0.0, 0.0, 2.0, 1.0, 1.0, 15); // CIELab distance from pure black
}

double DistanceFromWhiteRGB(const double &R, const double &G, const double &B) // CIELab distance from RGB(1,1,1)
{
    double X, Y, Z, L, a, b;
    RGBtoXYZ(R, G, B, X, Y, Z); // convert RGB to XYZ
    XYZtoCIELab(X, Y, Z, L, a, b); // convert XYZ to CIELab
    return DistanceCIEAdaptive(L, a, b, 1.0, 0.0, 0.0, 2.0, 1.0, 1.0, 15); // CIELab distance from pure white
}

double DistanceFromGrayRGB(const double &R, const double &G, const double &B) // CIELab distance from nearest gray (computed in CIELab space)
{
    double X, Y, Z, L, a, b;
    RGBtoXYZ(R, G, B, X, Y, Z); // convert RGB to XYZ
    XYZtoCIELab(X, Y, Z, L, a, b); // convert XYZ to CIELab
    return DistanceCIEAdaptive(L, a, b, L, 0.0, 0.0, 2.0, 1.0, 1.0, 15); // CIELab distance from corresponding gray (same L value with a=b=0, i.e. no chroma)
}

double DistanceRGB(const double &R1, const double &G1, const double &B1,
                        const double &R2, const double &G2, const double &B2,
                        const double k_L, const double k_C, const double k_H) // CIELab distance between 2 RGB values
{
    double L1, a1, b1, L2, a2, b2;
    RGBtoCIELab(R1, G1, B1, L1, a1, b1); // convert RGB to XYZ
    RGBtoCIELab(R2, G2, B2, L2, a2, b2); // same for 2nd RGB value
    return DistanceCIEAdaptive(L1, a1, b1, L2, a2, b2, k_L, k_C, k_H, 15); // CIELab distance
}

double DistanceRGB(const int &R1, const int &G1, const int &B1,
                        const int &R2, const int &G2, const int &B2,
                        const double k_L, const double k_C, const double k_H) // CIELab distance between 2 RGB values
{
    double L1, a1, b1, L2, a2, b2;
    RGBtoCIELab(R1, G1, B1, L1, a1, b1); // convert RGB to CIELab
    RGBtoCIELab(R2, G2, B2, L2, a2, b2); // same for 2nd RGB value
    return DistanceCIEAdaptive(L1, a1, b1, L2, a2, b2, k_L, k_C, k_H, 15); // CIELab distance
}

double DistanceRGBOKLAB(const double &R1, const double &G1, const double &B1,
                        const double &R2, const double &G2, const double &B2) // OKLAB distance between 2 RGB values
{
    double L1, a1, b1, L2, a2, b2;
    RGBtoOKLAB(R1, G1, B1, L1, a1, b1); // convert RGB to OKLab
    RGBtoOKLAB(R2, G2, B2, L2, a2, b2); // same for 2nd RGB value
    return DistanceCIEDE76(L1, a1, b1, L2, a2, b2); // OKLAB distance
}

double DistanceRGBOKLAB(const int &R1, const int &G1, const int &B1,
                        const int &R2, const int &G2, const int &B2) // OKLAB distance between 2 RGB values - RGB is 8-bit !
{
    double L1, a1, b1, L2, a2, b2;
    RGBtoOKLAB(R1, G1, B1, L1, a1, b1); // convert RGB to OKLab
    RGBtoOKLAB(R2, G2, B2, L2, a2, b2); // same for 2nd RGB value
    return DistanceCIEDE76(L1, a1, b1, L2, a2, b2); // OKLAB distance
}

///////////////////////////////////////////////////////////
//// Color utils
///////////////////////////////////////////////////////////
// All input values are in range [0..1]

void RGBtoStandard(const double &r, const double &g, const double &b, int &R, int &G, int &B) // convert RGB [0..1] to RGB [0..255]
{
    R = round(r * 255.0);
    G = round(g * 255.0);
    B = round(b * 255.0);
}

void RGBMean(const double &R1, const double &G1, const double &B1, const double W1,
             const double &R2, const double &G2, const double &B2, const double W2,
             double &R, double &G, double &B) // mean RGB value of 2 RGB values with transition to linear RGB
{
    double r1, g1, b1, r2, g2, b2;
    // gamma correction - conversion to linear space - better for means
    RGBtoLinear(R1, G1, B1, r1, g1, b1); // first RGB value
    RGBtoLinear(R2, G2, B2, r2, g2, b2); // second RGB value

    // mean for RGB values in a linear space
    R = (r1 * W1 + r2 * W2) / (W1 + W2);
    G = (g1 * W1 + g2 * W2) / (W1 + W2);
    B = (b1 * W1 + b2 * W2) / (W1 + W2);

    // invert gamma return to RGB
    LinearToRGB(R, G, B, R, G, B); // gamma correction to linear sRGB
}

void RGBMean(const int &R1, const int &G1, const int &B1, const double W1,
                    const int &R2, const int &G2, const int &B2, const double W2,
                    int &R, int &G, int &B) // mean RGB value of 2 RGB values with transition to linear RGB - RGB input and output are 8-bit !
{
    double r1, g1, b1, r2, g2, b2;
    // gamma correction - conversion to linear space - better for means
    RGBtoLinear_LUT(R1, G1, B1, r1, g1, b1); // first RGB value
    RGBtoLinear_LUT(R2, G2, B2, r2, g2, b2); // second RGB value

    // mean for RGB values in a linear space
    R = 255.0 * (r1 * W1 + r2 * W2) / (W1 + W2);
    G = 255.0 * (g1 * W1 + g2 * W2) / (W1 + W2);
    B = 255.0 * (b1 * W1 + b2 * W2) / (W1 + W2);

    // invert gamma return to RGB
    double Rfinal, Gfinal, Bfinal;
    LinearToRGB(R, G, B, Rfinal, Gfinal, Bfinal); // gamma correction to linear sRGB

    R = round(Rfinal * 255.0);
    G = round(Gfinal * 255.0);
    B = round(Bfinal * 255.0);
}

void RGBMeanWithCIELab(const double &R1, const double &G1, const double &B1, const double W1,
                       const double &R2, const double &G2, const double &B2, const double W2,
                       double &R, double &G, double &B) // mean RGB value of 2 RGB values with transition to CIELab
{
    double L, a, b, L1, L2, a1, a2, b1, b2;

    RGBtoCIELab(R1, G1, B1, L1, a1, b1);
    RGBtoCIELab(R2, G2, B2, L2, a2, b2);

    L = (L1 * W1 + L2 * W2) / (W1 + W2);
    a = (a1 * W1 + a2 * W2) / (W1 + W2);
    b = (b1 * W1 + b2 * W2) / (W1 + W2);

    CIELabToRGB(L, a, b, R, G, B);
}

void RGBMeanWithCIELab(const int &R1, const int &G1, const int &B1, const double W1,
                       const int &R2, const int &G2, const int &B2, const double W2,
                       int &R, int &G, int &B) // mean RGB value of 2 RGB values with transition to CIELab - RGB is 8-bit !
{
    double L, a, b, L1, L2, a1, a2, b1, b2;

    RGBtoCIELab(R1, G1, B1, L1, a1, b1);
    RGBtoCIELab(R2, G2, B2, L2, a2, b2);

    L = (L1 * W1 + L2 * W2) / (W1 + W2);
    a = (a1 * W1 + a2 * W2) / (W1 + W2);
    b = (b1 * W1 + b2 * W2) / (W1 + W2);

    double Rfinal, Gfinal, Bfinal;
    CIELabToRGB(L, a, b, Rfinal, Gfinal, Bfinal);

    R = round(Rfinal * 255.0);
    G = round(Gfinal * 255.0);
    B = round(Bfinal * 255.0);
}

void RGBMeanWithOKLAB(const double &R1, const double &G1, const double &B1, const double W1,
                       const double &R2, const double &G2, const double &B2, const double W2,
                       double &R, double &G, double &B) // mean RGB value of 2 RGB values with transition to OKLAB
{
    double L, a, b, L1, L2, a1, a2, b1, b2;

    RGBtoOKLAB(R1, G1, B1, L1, a1, b1);
    RGBtoOKLAB(R2, G2, B2, L2, a2, b2);

    L = (L1 * W1 + L2 * W2) / (W1 + W2);
    a = (a1 * W1 + a2 * W2) / (W1 + W2);
    b = (b1 * W1 + b2 * W2) / (W1 + W2);

    OKLABtoRGB(L, a, b, R, G, B);
}

void RGBMeanWithOKLAB(const int &R1, const int &G1, const int &B1, const double W1,
                       const int &R2, const int &G2, const int &B2, const double W2,
                       int &R, int &G, int &B) // mean RGB value of 2 RGB values with transition to OKLAB - RGB is 8-bit !
{
    double L, a, b, L1, L2, a1, a2, b1, b2;

    RGBtoOKLAB(R1, G1, B1, L1, a1, b1);
    RGBtoOKLAB(R2, G2, B2, L2, a2, b2);

    L = (L1 * W1 + L2 * W2) / (W1 + W2);
    a = (a1 * W1 + a2 * W2) / (W1 + W2);
    b = (b1 * W1 + b2 * W2) / (W1 + W2);

    double Rfinal, Gfinal, Bfinal;
    OKLABtoRGB(L, a, b, Rfinal, Gfinal, Bfinal);

    R = round(Rfinal * 255.0);
    G = round(Gfinal * 255.0);
    B = round(Bfinal * 255.0);
}

void CIELabMean(const double &L1, const double &A1, const double &B1, const double W1,
                const double &L2, const double &A2, const double &B2, const double W2,
                double &L, double &A, double &B) // mean CIELab value of 2 CIELab values
{
    L = (L1 * W1 + L2 * W2) / (W1 + W2);
    A = (A1 * W1 + A2 * W2) / (W1 + W2);
    B = (B1 * W1 + B2 * W2) / (W1 + W2);
}

void CIELabMeanToRGB(const double &L1, const double &A1, const double &B1, const double W1,
                     const double &L2, const double &A2, const double &B2, const double W2,
                     double &R, double &G, double &B) // mean CIELab value of 2 CIELab values to RGB
{
    double L, a, b;
    CIELabMean(L1, A1, B1, W1, L2, A2, B2, W2, L, a, b);
    CIELabToRGB(L, a, b, R, G, B);
}

void OKLABMean(const double &L1, const double &A1, const double &B1, const double W1,
                const double &L2, const double &A2, const double &B2, const double W2,
                double &L, double &A, double &B) // mean OKLAB value of 2 OKLAB values
{
    L = (L1 * W1 + L2 * W2) / (W1 + W2);
    A = (A1 * W1 + A2 * W2) / (W1 + W2);
    B = (B1 * W1 + B2 * W2) / (W1 + W2);
}

void OKLABMeanToRGB(const double &L1, const double &A1, const double &B1, const double W1,
                     const double &L2, const double &A2, const double &B2, const double W2,
                     double &R, double &G, double &B) // mean OKLAB value of 2 OKLAB values to RGB
{
    double L, a, b;
    OKLABMean(L1, A1, B1, W1, L2, A2, B2, W2, L, a, b);
    OKLABtoRGB(L, a, b, R, G, B);
}

void RGBMixSubtractiveWithRYB(const double &R1, const double &G1, const double &B1, const double &W1,
                                const double &R2, const double &G2, const double &B2, const double &W2,
                                double &R, double &G, double &B) // substractive mean of two RGB colors with transition to RYB
{
    double Rryb, Yryb, Bryb, Rryb1, Yryb1, Bryb1, Rryb2, Yryb2, Bryb2;

    RGBtoRYB(R1, G1, B1, Rryb1, Yryb1, Bryb1);
    RGBtoRYB(R2, G2, B2, Rryb2, Yryb2, Bryb2);

    Rryb = (Rryb1 * W1 + Rryb2 * W2) / (W1 + W2);
    Yryb = (Yryb1 * W1 + Yryb2 * W2) / (W1 + W2);
    Bryb = (Bryb1 * W1 + Bryb2 * W2) / (W1 + W2);

    RYBtoRGB(Rryb, Yryb, Bryb, R, G, B);
}

void RGBMixSubtractive(const double &R1, const double &G1, const double &B1, const double W1,
                       const double &R2, const double &G2, const double &B2, const double W2,
                       double &R, double &G, double &B,
                       const bool &testZero) // use color reflectance to mix two RGB colors - accurate but slow
    // if testZero is false, be sure to use RGB values that don't contain zeros !
{
    // convert colors to linear RGB
    double r1, g1, b1, r2, g2, b2;
    RGBtoLinear(R1, G1, B1, r1, g1, b1);
    RGBtoLinear(R2, G2, B2, r2, g2, b2);

    // 0 is a bad value ( = infinite !), bad for pure black mixing
    if (testZero) {
        if (r1 == 0)
            r1 = 0.000005;
        if (r2 == 0)
            r2 = 0.000005;
        if (g1 == 0)
            g1 = 0.000005;
        if (g2 == 0)
            g2 = 0.000005;
        if (b1 == 0)
            b1 = 0.000005;
        if (b2 == 0)
            b2 = 0.000005;
    }

    // convert RGB linear colors to reflectance curve, and mix the 2 reflectance curves using weighted geometric mean
    // see Scott Burn's page : http://scottburns.us/fast-rgb-to-spectrum-conversion-for-reflectances/
    double reflectance1[36]; // reflectance curve of color 1
    double reflectance2[36]; // reflectance curve of color 2
    double mixed[36]; // mixed reflectance curve
    for (int col = 0; col < 36; col++) { // for each element of rho reflectance matrix
        reflectance1[col] = r1 * reflectance_Rho[0][col] + g1 * reflectance_Rho[1][col] + b1 * reflectance_Rho[2][col]; // compute sum of RGB-weighted reflectance curves for color 1
        reflectance2[col] = r2 * reflectance_Rho[0][col] + g2 * reflectance_Rho[1][col] + b2 * reflectance_Rho[2][col]; // ... and color 2
        mixed[col] = pow(pow(reflectance1[col], W1) * pow(reflectance2[col], W2), 1.0 / (W1 + W2)); // mix the 2 colors using weighted geometric mean
    }

    // now that we have the reflectance curve of the mixed color, convert it back to RGB linear
    // see Scott Burn's page : http://scottburns.us/subtractive-color-mixture/
    double RGB[3] = {0, 0, 0}; // initialize RGB values
    for (int col = 0; col < 36; col++) { // for each element of T matrix ("inverse" rho)
        RGB[0] += reflectance_T[0][col] * mixed[col]; // red is the sum of each tiny discretized part
        RGB[1] += reflectance_T[1][col] * mixed[col]; // now for green
        RGB[2] += reflectance_T[2][col] * mixed[col]; // and for blue
    }

    // final result
    // pb with initial RGB values near 0 or 1 : linear values can be outside of gamut - see Scott burn's "Update 2/15/2020: A Word of Caution Regarding RGB Gamut Violations" from same page as above
    LinearToRGB(GetValueRangeZeroOne(RGB[0]), GetValueRangeZeroOne(RGB[1]), GetValueRangeZeroOne(RGB[2]), R, G, B); // convert RGB linear to RGB
}

void RGBMixSubtractive(const int &R1, const int &G1, const int &B1, const double W1,
                            const int &R2, const int &G2, const int &B2, const double W2,
                            int &R, int &G, int &B,
                            const bool &testZero) // use color reflectance to mix two RGB colors - accurate but slow - RGB input and output is 8-bit !
    // if testZero is false, be sure to use RGB values that don't contain zeros !
{
    // convert colors to linear RGB
    double r1, g1, b1, r2, g2, b2;
    RGBtoLinear_LUT(R1, G1, B1, r1, g1, b1);
    RGBtoLinear_LUT(R2, G2, B2, r2, g2, b2);

    // 0 is a bad value ( = infinite !), bad for pure black mixing
    if (testZero) {
        if (r1 == 0)
            r1 = 0.000005;
        if (r2 == 0)
            r2 = 0.000005;
        if (g1 == 0)
            g1 = 0.000005;
        if (g2 == 0)
            g2 = 0.000005;
        if (b1 == 0)
            b1 = 0.000005;
        if (b2 == 0)
            b2 = 0.000005;
    }

    // convert RGB linear colors to reflectance curve, and mix the 2 reflectance curves using weighted geometric mean
    // see Scott Burn's page : http://scottburns.us/fast-rgb-to-spectrum-conversion-for-reflectances/
    double reflectance1[36]; // reflectance curve of color 1
    double reflectance2[36]; // reflectance curve of color 2
    double mixed[36]; // mixed reflectance curve
    for (int col = 0; col < 36; col++) { // for each element of rho reflectance matrix
        reflectance1[col] = r1 * reflectance_Rho[0][col] + g1 * reflectance_Rho[1][col] + b1 * reflectance_Rho[2][col]; // compute sum of RGB-weighted reflectance curves for color 1
        reflectance2[col] = r2 * reflectance_Rho[0][col] + g2 * reflectance_Rho[1][col] + b2 * reflectance_Rho[2][col]; // ... and color 2
        mixed[col] = pow(pow(reflectance1[col], W1) * pow(reflectance2[col], W2), 1.0 / (W1 + W2)); // mix the 2 colors using weighted geometric mean
    }

    // now that we have the reflectance curve of the mixed color, convert it back to RGB linear
    // see Scott Burn's page : http://scottburns.us/subtractive-color-mixture/
    double RGB[3] = {0, 0, 0}; // initialize RGB values
    for (int col = 0; col < 36; col++) { // for each element of T matrix ("inverse" rho)
        RGB[0] += reflectance_T[0][col] * mixed[col]; // red is the sum of each tiny discretized part
        RGB[1] += reflectance_T[1][col] * mixed[col]; // now for green
        RGB[2] += reflectance_T[2][col] * mixed[col]; // and for blue
    }

    // final result
    // pb with initial RGB values near 0 or 1 : linear values can be outside of gamut - see Scott burn's "Update 2/15/2020: A Word of Caution Regarding RGB Gamut Violations" from same page as above
    double Rfinal, Gfinal, Bfinal;
    LinearToRGB(GetValueRangeZeroOne(RGB[0]), GetValueRangeZeroOne(RGB[1]), GetValueRangeZeroOne(RGB[2]), Rfinal, Gfinal, Bfinal); // convert RGB linear to RGB
    R = round(Rfinal * 255.0);
    G = round(Gfinal * 255.0);
    B = round(Bfinal * 255.0);
    /*R = GetValueRangeZeroOne(R); // clamp values to [0..1]
    G = GetValueRangeZeroOne(G);
    B = GetValueRangeZeroOne(B);*/
}

void RGBtoLinear(const double &R, const double &G, const double &B, double &r, double &g, double &b) // Apply linear RGB gamma correction to sRGB
{
    /*if (isFast) {
        // see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
        r = 0.012522878 * R + 0.682171111 * R * R + 0.305306011 * R * R * R;
        g = 0.012522878 * G + 0.682171111 * G * G + 0.305306011 * G * G * G;
        b = 0.012522878 * B + 0.682171111 * B * B + 0.305306011 * B * B * B;
    }*/

    // Gamma correction - conversion to linear space - source http://www.brucelindbloom.com/index.html?Eqn_RGB_to_XYZ.html
    // most correct approximation but it is SLOW
    if (R > 0.04045)
        r = pow((R + 0.055) / 1.055, 2.4);
    else
        r = R / 12.92;
    if (G > 0.04045)
        g = pow((G + 0.055) / 1.055, 2.4);
    else
        g = G / 12.92;
    if (B > 0.04045)
        b = pow((B + 0.055) / 1.055, 2.4);
    else
        b = B / 12.92;
}

void LinearToRGB(const double &R, const double &G, const double &B, double &r, double &g, double &b) // Apply linear gamma correction from sRGB
{
    /*if (isFast) {
        // see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
        r = GetValueRangeZeroOne(0.585122381 * sqrt(R) + 0.783140355 * sqrt(sqrt(R)) - 0.368262736 * sqrt(sqrt(sqrt(R))));
        g = GetValueRangeZeroOne(0.585122381 * sqrt(G) + 0.783140355 * sqrt(sqrt(G)) - 0.368262736 * sqrt(sqrt(sqrt(G))));
        b = GetValueRangeZeroOne(0.585122381 * sqrt(B) + 0.783140355 * sqrt(sqrt(B)) - 0.368262736 * sqrt(sqrt(sqrt(B))));
    }*/

    // Gamma profile - source http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    // most correct approximation but it is SLOW
    double q = 1.0 / 2.4;

    if (R > 0.0031308)
        r = 1.055 * pow(R, q) - 0.055;
    else
        r = R * 12.92;

    if (G > 0.0031308)
        g = 1.055 * pow(G, q) - 0.055;
    else
        g = G * 12.92;

    if (B > 0.0031308)
        b = 1.055 * pow(B, q) - 0.055;
    else
        b = B * 12.92;
}

void RGBtoLinear_LUT(const int &R, const int &G, const int &B, double &r, double &g, double &b) // Apply linear RGB gamma correction to sRGB from LUT - RGB is 8-bit !
    // all [0..255] values were computed with function RGBtoLinear : they are used as LUT
{
    r = RGBlinearLUT[R];
    g = RGBlinearLUT[G];
    b = RGBlinearLUT[B];

    //RGBtoLinear(R / 255.0, G / 255.0, B / 255.0, r, g, b);
}

double PerceivedBrightnessRGB(const double &R, const double &G, const double &B) // perceived brightness of RGB value (RGB in [0..255])
{
    return sqrt(R * R * 65025.0 * 0.299 + G * G * 65025.0 * 0.587 + B * B * 65025.0 * 0.114) / 255.0; // Perceived brightness=sqrt(0.299*R² + 0.587*G² + 0.114*B²)
}

void CIEHSLChfromRGB(const double &R, const double &G, const double &B,
                double &Hhsl, double &S, double&L, double &C, double &Hlab, double &a, double &b) // get HSL values from RGB using HSL, CIELab and CIELCHab
    // C, L and S from LCHab are more perceptive than S and L from HSL (which is directly and poorly derived from RGB)
    // Hue Hhsl = -1.0 if color is a gray
{
    double X, Y, Z; // for XYZ values

    // lightness, chroma and HLab
    RGBtoXYZ(R, G, B, X, Y, Z); // first step to get values in CIE spaces : convert RGB to XYZ
    XYZtoCIELab(X, Y, Z, L, a, b); // then to CIELAb
    CIELabToCIELCHab(a, b, C, Hlab); // then to CIELCHab

    if (L == 0.0) { // black is a particular value
        S = -1.0; // no saturation
        C = -1.0; // ... and no chroma
    }
    else if (C == 0)
        S = 0;
    else // if not black and has chroma > 0
        S = C / sqrt(C * C + L * L); // saturation S from LCHab - it's a distance !
    if (S > 1.0) // clip saturation ?
        S = 1.0;

    // hue
    double s, l, c;
    RGBtoHSL(R, G, B, Hhsl, s, l, c); // getting Hue from HSL

    if ((abs(a) < 0.01) and (abs(b) < 0.01)) // particular case of grays (including white and black)
        Hhsl = -1; // no hue for HSL
}

void CIEHSLChfromRGB(const int &R, const int &G, const int &B,
                double &Hhsl, double &S, double&L, double &C, double &Hlab, double &a, double &b) // get HSL values from RGB using HSL, CIELab and CIELCHab - RGB is 8-bit !
    // C, L and S from LCHab are more perceptive than S and L from HSL (which is directly and poorly derived from RGB)
    // Hue Hhsl = -1.0 if color is a gray
{
    double X, Y, Z; // for XYZ values

    // lightness, chroma and HLab
    RGBtoXYZ(R, G, B, X, Y, Z); // first step to get values in CIE spaces : convert RGB to XYZ
    XYZtoCIELab(X, Y, Z, L, a, b); // then to CIELAb
    CIELabToCIELCHab(a, b, C, Hlab); // then to CIELCHab

    if (L == 0.0) { // black is a particular value
        S = -1.0; // no saturation
        C = -1.0; // ... and no chroma
    }
    else if (C == 0)
        S = 0;
    else // if not black and has chroma > 0
        S = C / sqrt(C * C + L * L); // saturation S from LCHab - it's a distance !
    if (S > 1.0) // clip saturation ?
        S = 1.0;

    // hue
    double s, l, c;
    RGBtoHSL(R, G, B, Hhsl, s, l, c); // getting Hue from HSL

    if ((abs(a) < 0.01) and (abs(b) < 0.01)) // particular case of grays (including white and black)
        Hhsl = -1; // no hue for HSL
}

void OKLABHSLChfromRGB(const double &R, const double &G, const double &B,
                double &Hhsl, double &S, double&L, double &C, double &Hlab, double &a, double &b) // get HSL values from RGB using HSL, OKLAB
    // C, L and S from OKLCH are more perceptive than S and L from HSL (which is directly and poorly derived from RGB)
    // Hue Hhsl = -1.0 if color is a gray
{
    double X, Y, Z; // for XYZ values

    // lightness, chroma and HLab
    RGBtoXYZ(R, G, B, X, Y, Z); // first step to get values in OKLAB spaces : convert RGB to XYZ
    XYZtoOKLAB(X, Y, Z, L, a, b); // then to OKLAB
    OKLABtoOKLCH(a, b, C, Hlab); // then to OKLCH

    if (L == 0.0) { // black is a particular value
        S = -1.0; // no saturation
        C = -1.0; // ... and no chroma
    }
    else if (C == 0)
        S = 0;
    else // if not black and has chroma > 0
        S = C / sqrt(C * C + L * L); // saturation S from LCHab - it's a distance !
    if (S > 1.0) // clip saturation ?
        S = 1.0;

    // hue
    double s, l, c;
    RGBtoHSL(R, G, B, Hhsl, s, l, c); // getting Hue from HSL

    if ((abs(a) < 0.001) and (abs(b) < 0.001)) // particular case of grays (including white and black)
        Hhsl = -1; // no hue for HSL
}

void OKLABHSLChfromRGB(const int &R, const int &G, const int &B,
                double &Hhsl, double &S, double&L, double &C, double &Hlab, double &a, double &b) // get HSL values from RGB using HSL, OKLAB - RGB is 8-bit !
    // C, L and S from OKLCH are more perceptive than S and L from HSL (which is directly and poorly derived from RGB)
    // Hue Hhsl = -1.0 if color is a gray
{
    double X, Y, Z; // for XYZ values

    // lightness, chroma and HLab
    RGBtoXYZ(R, G, B, X, Y, Z); // first step to get values in OKLAB spaces : convert RGB to XYZ
    XYZtoOKLAB(X, Y, Z, L, a, b); // then to OKLAB
    OKLABtoOKLCH(a, b, C, Hlab); // then to OKLCH

    if (L == 0.0) { // black is a particular value
        S = -1.0; // no saturation
        C = -1.0; // ... and no chroma
    }
    else if (C == 0)
        S = 0;
    else // if not black and has chroma > 0
        S = C / sqrt(C * C + L * L); // saturation S from LCHab - it's a distance !
    if (S > 1.0) // clip saturation ?
        S = 1.0;

    // hue
    double s, l, c;
    RGBtoHSL(R, G, B, Hhsl, s, l, c); // getting Hue from HSL

    if ((abs(a) < 0.001) and (abs(b) < 0.001)) // particular case of grays (including white and black)
        Hhsl = -1; // no hue for HSL
}

bool IsRGBColorDark(int red, int green, int blue) // tell if a RGB color is dark or not - input is in bytes, not [0..1]
{
  double brightness;
  brightness = (red * 299) + (green * 587) + (blue * 114);
  brightness = brightness / 255000;

  // values range from 0 to 1 : anything greater than 0.5 should be bright enough for dark text
  return (brightness <= 0.5);
}

void AddLightnessToRGB(const int &inR, const int &inG, const int &inB, const double &Lpercent, int &toR, int &toG, int &toB) // modify lightness of RGB values - input is in bytes, not [0..1]
{
    double H, S, L, C, R, G, B;

    RGBtoHSL(double(inR) / 255.0, double(inG) / 255.0, double(inB) / 255.0, H, S, L, C); // convert RGB to HSL
    L *= 1.0 + Lpercent; // add percent to L
    L = GetValueRangeZeroOne(L); // L must stay in [0..1]
    HSLtoRGB(H, S, L, R, G, B); // convert new HSL value to RGB

    toR = round(R * 255.0); // return values
    toG = round(G * 255.0);
    toB = round(B * 255.0);
}

void AddSaturationToRGB(const int &inR, const int &inG, const int &inB, const double &Spercent, int &toR, int &toG, int &toB) // modify saturation of RGB values - input is in bytes, not [0..1]
{
    double H, S, L, C, R, G, B;
    RGBtoHSL(double(inR) / 255.0, double(inG) / 255.0, double(inB) / 255.0, H, S, L, C); // convert RGB to HSL
    S *= 1.0 + Spercent; // add percent to S
    S = GetValueRangeZeroOne(S); // S must stay in [0..1]
    HSLtoRGB(H, S, L, R, G, B); // conveert new HSL value to RGB

    toR = round(R * 255.0); // return values
    toG = round(G * 255.0);
    toB = round(B * 255.0);
}

///////////////////////////////////////////////////////////
//// Color spaces conversions
///////////////////////////////////////////////////////////
    // All values are in range [0..1] except when indicated

//// Spectral colors
//// see https://en.wikipedia.org/wiki/Spectral_color

void WavelengthToXYZ(const double &w, double &X, double &Y, double &Z) // wavelength to XYZ color space
{
    int n = -1; // index of array
    for (int W = 0; W < wavelength_XYZ_nb; W++) // all values
        if (int(w) == wavelength_XYZ[W][0]) { // wavelength in array ?
            n = W; // index of line
            break; // found
        }

    if (n == -1) { // not found ?
        X = 0.0;
        Y = 0.0;
        Z = 0.0;
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

    if      ((L >= 400.0) and (L < 410.0))  { t = (L - 400.0) / (410.0 - 400.0); R = 0.33 * t - 0.20 * t * t; }
    else if ((L >= 410.0) and (L < 475.0))  { t = (L - 410.0) / (475.0 - 410.0); R = 0.14 - 0.13 * t * t; }
    else if ((L >= 545.0) and (L < 595.0))  { t = (L - 545.0) / (595.0 - 545.0); R = 1.98 * t - t * t; }
    else if ((L >= 595.0) and (L < 650.0))  { t = (L - 595.0) / (650.0 - 595.0); R = 0.98 + 0.06 * t - 0.40 * t * t; }
    else if ((L >= 650.0) and (L < 700.0))  { t = (L - 650.0) / (700.0 - 650.0); R = 0.65 - 0.84 * t + 0.20 * t * t; }

    if      ((L >= 415.0) and (L < 475.0))  { t = (L - 415.0) / (475.0 - 415.0); G = 0.80 * t * t; }
    else if ((L >= 475.0) and (L < 590.0))  { t = (L - 475.0) / (590.0 - 475.0); G = 0.8 + 0.76 * t - 0.80 * t * t; }
    else if ((L >= 585.0) and (L < 639.0))  { t = (L - 585.0) / (639.0 - 585.0); G = 0.84 - 0.84 * t; }

    if      ((L >= 400.0) and (L < 475.0))  { t = (L - 400.0) / (475.0 - 400.0); B = 2.20 * t - 1.50 * t * t; }
    else if ((L >= 475.0) and (L < 560.0))  { t = (L - 475.0) / (560.0 - 475.0); B = 0.7 - t + 0.30 * t * t; }

    R = GetValueRangeZeroOne(R); // values must stay in [0..1]
    G = GetValueRangeZeroOne(G);
    B = GetValueRangeZeroOne(B);
}

//// HSV
//// see https://en.wikipedia.org/wiki/HSL_and_HSV
//// All values [0..1]
//// Common range : H [0..359] S [0..100] V [0..100]

void RGBtoHSV(const double &R, const double &G, const double &B, double& H, double& S, double &V, double &C) // convert RGB value to HSV+C
{ // function checked OK with other calculators
    double cmax = std::max(std::max(R, G), B);    // maximum of RGB
    double cmin = std::min(std::min(R, G), B);    // minimum of RGB
    double diff = cmax-cmin;       // diff of cmax and cmin.

    if (diff > 0.0) { // not particular case of diff = 0 -> find if R G or B is dominant
        if (cmax == R) // R is dominant
            H = 60.0 * (fmod(((G - B) / diff), 6.0)); // compute H
        else if (cmax == G) // G is dominant
            H = 60 * (((B - R) / diff) + 2.0); // compute H
        else if (cmax == B) // B is dominant
            H = 60 * (((R - G) / diff) + 4.0); // compute H

        if (cmax > 0.0) // compute S
            S = diff / cmax;
        else
            S = 0.0;

        V = cmax; // compute V
    }
    else { // particular case -> H = red (convention)
        H = 0.0;
        S = 0.0;
        V = cmax;
    }

    while (H < 0.0) // H must be in [0..360] range
        H += 360.0;
    while (H >= 360.0)
        H -= 360.0;

    // Final results are in range [0..1]
    H = H / 360.0; // was in degrees
    C = diff; // chroma
}

void HSVtoRGB(const double &H, const double &S, const double &V, double &R, double &G, double &B) // convert HSV value to RGB
{ // function checked OK with other calculators
  double C = V * S; // chroma
  double HPrime = H * 6.0; // dominant 6th part of H - H must be in [0..360]
  double X = C * (1 - abs(fmod(HPrime, 2.0) - 1.0)); // intermediate value
  double M = V - C; // difference to add at the end

  // for each part its calculus
  if (0.0 <= HPrime && HPrime < 1) {
    R = C;
    G = X;
    B = 0;
  }
  else if (1 <= HPrime && HPrime < 2) {
    R = X;
    G = C;
    B = 0.0;
  }
  else if (2 <= HPrime && HPrime < 3) {
    R = 0.0;
    G = C;
    B = X;
  }
  else if (3 <= HPrime && HPrime < 4) {
    R = 0.0;
    G = X;
    B = C;
  }
  else if (4 <= HPrime && HPrime < 5) {
    R = X;
    G = 0.0;
    B = C;
  }
  else if(5 <= HPrime && HPrime < 6) {
    R = C;
    G = 0.0;
    B = X;
  } else {
    R = 0.0;
    G = 0.0;
    B = 0.0;
  }

  R += M; // final results
  G += M;
  B += M;
}

void HSVtoStandard(const double &h, const double &s, const double &v, int &H, int &S, int &V) // convert HSV [0..1] to HSV H [0..359] S and V [0..100]
{
    H = round(h * 360.0);
    S = round(s * 100.0);
    V = round(v * 100.0);
}

//// HSL
//// see https://en.wikipedia.org/wiki/HSL_and_HSV
//// All values [0..1]
//// Common range : H [0..359] S [0..100] L [0..100]

void RGBtoHSL(const double &R, const double &G, const double &B, double &H, double &S, double &L, double &C) // convert RGB value to HSL
{ // function checked OK with other calculators
    double cmax = std::max(std::max(R, G), B);    // maximum of RGB
    double cmin = std::min(std::min(R, G), B);    // minimum of RGB
    double diff = cmax - cmin;       // diff of cmax and cmin.

    L = (cmax + cmin) / 2.0; // middle of range

    if (cmax == cmin) { // particular case : color is a gray
        S = 0.0;
        H = 0.0;
    }
    else {
        if (L < 0.5) // result depends on Lightness
            S = diff / (cmax + cmin); // compute S
        else
            S = diff / (2.0 - cmax - cmin); // compute S

        // which is the dominant in R, G, B
        if (cmax == R) // red
            H = (G - B) / diff; // compute H
        if (cmax == G) // green
            H = 2.0 + (B - R) / diff; // compute H
        if (cmax == B) // blue
            H = 4.0 + (R - G) / diff; // compute H
    }

    H = H * 60.0; // H in degrees

    while (H < 0.0) // H in [0..360]
        H += 360.0;
    while (H >= 360.0)
        H -= 360.0;

    // Final results in range [0..1]
    H = H / 360.0; // was in degrees
    C = diff; // Chroma
}

double HueToRGB(const double &v1, const double &v2, const double &H) // Convert Hue to R, G or B value for HSLtoRGB function
{
    double vH = H;

    while (vH < 0) // H must be in range [0..1]
        vH += 1.0;
    while (vH >= 1)
        vH -= 1.0;

    if ((6.0 * vH) < 1.0) // which component (R, G, B) to compute ?
        return v1 + (v2 - v1) * 6.0 * vH;
    if ((2.0 * vH) < 1.0 )
        return v2;
    if ((3.0 * vH) < 2.0 )
        return (v1 + (v2 - v1) * ((twoThird) - vH) * 6.0);
    return (v1);
}

void HSLtoRGB(const double &H, const double &S, const double &L, double &R, double &G, double &B) // convert HSL to RGB value - H is in degrees
{ // function checked OK with other calculators
    if ( S == 0.0 ) { // color is a gray
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

        R = HueToRGB(var_1, var_2, H + oneThird); // compute R, G, B
        G = HueToRGB(var_1, var_2, H);
        B = HueToRGB(var_1, var_2, H - oneThird);
    }
}

void HSLtoStandard(const double &h, const double &s, const double &l, int &H, int &S, int &L) // convert HSL [0..1] to HSL H [0..359] S and L [0..100]
{
    H = round(h * 360.0);
    S = round(s * 100.0);
    L = round(l * 100.0);
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
    // calculus is simple ! This is a direct relation
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

void HWBtoStandard(const double &h, const double &w, const double &b, int &H, int &W, int &B) // convert HWB [0..1] to HWB H [0..359] W and B [0..100]
{
    H = round(h * 360.0);
    W = round(w * 100.0);
    B = round(b * 100.0);
}

//// XYZ
//// See https://en.wikipedia.org/wiki/CIE_1931_color_space
////   and https://fr.wikipedia.org/wiki/CIE_XYZ
//// All values [0..1]
//// Common range for XYZ : [0..100]

void RGBtoXYZ(const double &R, const double &G, const double &B, double &X, double &Y, double &Z) // convert RGB (in fact sRGB) value to CIE XYZ
{ // function checked OK with other calculators
    double r, g, b;

    RGBtoLinear(R, G, B, r, g, b); // gamma correction to linear sRGB

    // Gammut conversion to sRGB - source http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    X = r * 0.4124564 + g * 0.3575761 + b * 0.1804375;
    Y = r * 0.2126729 + g * 0.7151522 + b * 0.0721750;
    Z = r * 0.0193339 + g * 0.1191920 + b * 0.9503041;
}

void RGBtoXYZ(const int &R, const int &G, const int &B, double &X, double &Y, double &Z) // convert RGB (in fact sRGB) value to CIE XYZ - RGB is 8-bit !
{ // function checked OK with other calculators
    double r, g, b;

    RGBtoLinear_LUT(R, G, B, r, g, b); // gamma correction to linear sRGB

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

    LinearToRGB(R, G, B, R, G, B); // gamma correction from linear sRGB

    // clipping : R, G and B must be in [0..1]
    R = GetValueRangeZeroOne(R);
    G = GetValueRangeZeroOne(G);
    B = GetValueRangeZeroOne(B);
}

void XYZtoRGBNoClipping(const double &X, const double &Y, const double &Z, double &R, double &G, double &B) // convert from XYZ to RGB without clipping to [0..1]
{ // function checked OK with other calculators
    // Gammut conversion - source http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    R = X *  3.2404542 + Y * -1.5371385 + Z * -0.4985314;
    G = X * -0.9692660 + Y *  1.8760108 + Z *  0.0415560;
    B = X *  0.0556434 + Y * -0.2040259 + Z *  1.0572252;

    LinearToRGB(R, G, B, R, G, B); // gamma correction from linear sRGB

    if ((R > 1.0) or (R < 0.0) or (G > 1.0) or (G < 0.0) or (B > 1.0) or (B < 0.0)) { // R, G and B must be in [0..1] - else return black (0,0,0) - no clipping
        R = 0.0;
        G = 0.0;
        B = 0.0;
    }
}

void XYZtoStandard(const double &x, const double &y, const double &z, int &X, int &Y, int &Z) // convert XYZ [0..1] to XYZ [0..100]
{
    X = round(x * 100.0);
    Y = round(y * 100.0);
    Z = round(z * 100.0);
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
        x = X / (X + Y + Z);
        y = Y / (X + Y + Z);
    }
}

void xyYtoXYZ(const double &x, const double &y, const double &Y, double &X, double &Z) // convert CIE xyY value to CIE XYZ
{ // function checked OK with other calculators
    if (Y == 0.0) { // Y is lightness so if 0 => no color
        X = 0.0;
        Z = 0.0;
    }
    else { // from formula X + Y + Z = 1
        X = x * Y / y;
        Z = (1.0 - x - y) * Y / y;
    }
}

//// CIE L*a*b*
//// See https://en.wikipedia.org/wiki/CIELAB_color_space for LAB
//// All values [0..1]
//// Common range for L*a*b*: L [0..100] a [-128..127] b [-128..127]

void XYZtoCIELab(const double &X, const double &Y, const double &Z, double &L, double &A, double &B) // convert CIE XYZ value to CIE L*A*B*
{ // function checked OK with other calculators
    double Xr = X / CIE_ref_White_X;
    double Yr = Y / CIE_ref_White_Y;
    double Zr = Z / CIE_ref_White_Z;

    double fX, fY, fZ;
    if (Xr > CIE_E)
        fX = pow(Xr, oneThird);
    else
        fX = (CIE_K * Xr + 16.0) / 116.0;
    if (Yr > CIE_E)
        fY = pow(Yr, oneThird);
    else
        fY = (CIE_K * Yr + 16.0) / 116.0;
    if (Zr > CIE_E)
        fZ = pow(Zr, oneThird);
    else
        fZ = (CIE_K * Zr + 16.0) / 116.0;

    L = 116.0 *  fY - 16.0;
    A = 500.0 * (fX - fY);
    B = 200.0 * (fY - fZ);

    L = L / 100.0; // to stay in [0..1] range
    A = A / 127.0;
    B = B / 127.0;
}

void CIELabToXYZ(const double &L, const double &A, const double &B, double &X, double &Y, double &Z) // convert CIE L*A*B* to CIE XYZ
{ // function checked OK with other calculators
    double fY = (L * 100.0 + 16.0) / 116.0;
    double fZ = fY - B * 127.0 / 200.0;
    double fX = A * 127.0 / 500.0 + fY;

    double Xr, Yr, Zr;

    if (fX * fX * fX > CIE_E)
        Xr = fX * fX * fX;
    else
        Xr = (116.0 * fX - 16.0) / CIE_K;
    if (L * 100.0 > CIE_KE)
        Yr = (L * 100.0 + 16.0) / 116.0 * (L * 100.0 + 16.0) / 116.0 * (L * 100.0 + 16.0) / 116.0;
    else
        Yr = L * 100.0 / CIE_K;
    if (fZ * fZ * fZ > CIE_E)
        Zr = fZ * fZ * fZ;
    else
        Zr = (116.0 * fZ - 16.0) / CIE_K;

    X = Xr * CIE_ref_White_X;
    Y = Yr * CIE_ref_White_Y;
    Z = Zr * CIE_ref_White_Z;
}

void RGBtoCIELab(const double &R, const double &G, const double &B, double &L, double &a, double &b) // convert RGB to CIELab
{
    double X, Y, Z;
    RGBtoXYZ(R, G, B, X, Y, Z);
    XYZtoCIELab(X, Y, Z, L, a, b);
}

void CIELabToRGB(const double &L, const double &a, const double &b, double &R, double &G, double &B) // convert CIELab to RGB
{
    if (L == 0.0) {
        R = 0.0;
        G = 0.0;
        B = 0.0;
        return;
    }

    double X, Y, Z;
    CIELabToXYZ(L, a, b, X, Y, Z);
    XYZtoRGB(X, Y, Z, R, G, B);
}

void RGBtoCIELab(const int &R, const int &G, const int &B, double &L, double &a, double &b) // convert RGB to CIELab - RGB is 8-bit !
{
    double X, Y, Z;
    RGBtoXYZ(R, G, B, X, Y, Z);
    XYZtoCIELab(X, Y, Z, L, a, b);
}

void CIELabToStandard(const double &l, const double &a, const double &b, int &L, int &A, int &B) // convert CIELab [0..1] to CIELab L [0..100] a and b [-128..127]
{
    L = round(l * 100.0);
    A = round(a * 127.0);
    B = round(b * 127.0);
}

//// CIE LCHab
//// See https://en.wikipedia.org/wiki/CIELAB_color_space#Cylindrical_representation:_CIELCh_or_CIEHLC
//// All values [0..1] except C which can overlap 1 by ~30%
//// Common range : L [0..100] C [0..100+] h [0..360]
//// Note that L is exactly equal to L of CIE L*a*b*

void CIELabToCIELCHab(const double &A, const double &B, double &C, double &H) // convert from LAB to HLC - L is the same so no need to convert
{ // function checked OK with other calculators
    C = sqrt(A * A + B * B); // chroma

    H = atan2(B, A) / pi_rad; // Hue - cartesian to polar
    while (H < 0.0) // Hue in range [0..1]
        H += 1.0;
    while (H >= 1.0)
        H -= 1.0;
}

void CIELCHabToCIELab(const double &C, const double &H, double &A, double &B) // convert from HLC to LAB - L is the same so no need to convert
{ // function checked OK with other calculators
    A = C * cos(H * pi_rad); // polar to cartesian
    B = C * sin(H * pi_rad);
}

void CIELCHabToStandard(const double &l, const double &c, const double &h, int &L, int &C, int &H) // convert CIE LCHab [0..1] to CIE LCHab L [0..100] C [0..100+] H [0..359]
{
    L = round(l * 100.0);
    C = round(c * 127.0); // because a and b from CIELab are in [-128..127]
    H = round(h * 360.0);
}

void RGBtoCIELCHab(const double &R, const double &G, const double &B, double &L, double &C, double &H) // convert RGB to LCHab
{
    double a, b;
    RGBtoCIELab(R, G, B, L, a, b);
    CIELabToCIELCHab(a, b, C, H);
}

void CIELCHabToRGB(const double &L, const double &C, const double &H, double &R, double &G, double &B) // convert LCHab to RGB
{
    double a, b;
    CIELCHabToCIELab(C, H, a, b);
    CIELabToRGB(L, a, b, R, G, B);
}

void RGBtoCIELCHab(const int &R, const int &G, const int &B, double &L, double &C, double &H) // convert RGB to LCHab - RGB is 8-bit !
{
    double a, b;
    RGBtoCIELab(R, G, B, L, a, b);
    CIELabToCIELCHab(a, b, C, H);
}

//// CIE L*u*v*
//// see https://en.wikipedia.org/wiki/CIELUV
//// All values [0..1]
//// Common range for L*u*v*: L* [0..100] u* [-100..100] v* [-100..100]

void XYZtoCIELuv(const double &X, const double &Y, const double &Z, double &L, double &u, double &v) // convert CIE XYZ value to CIE L*u*v*
{ // function checked OK with other calculators
    if (Y / CIE_ref_White_Y > CIE_E) // two-part equation
        L = 116.0 * pow(Y / CIE_ref_White_Y, oneThird) - 16.0;
    else
        L = CIE_K * Y / CIE_ref_White_Y;

    double u_prime = 4.0 * X / (X + 15.0 * Y + 3.0 * Z); // intermediate calculus
    double u_ref =   4.0 * CIE_ref_White_X / (CIE_ref_White_X + 15.0 * CIE_ref_White_Y + 3.0 * CIE_ref_White_Z);
    double v_prime = 9.0 * Y / (X + 15.0 * Y + 3.0 * Z);
    double v_ref =   9.0 * CIE_ref_White_Y / (CIE_ref_White_X + 15.0 * CIE_ref_White_Y + 3.0 * CIE_ref_White_Z);

    u = 13.0 * L * (u_prime - u_ref); // final values
    v = 13.0 * L * (v_prime - v_ref);

    L = L / 100.0; // to stay in range [0..1]
    u = u / 100.0;
    v = v / 100.0;
    if (isnan(u)) // division by zero is bad so default value
        u = 0.0;
    if (isnan(v))
        v = 0.0;
}

void CIELuvToXYZ(const double &L, const double &U, const double &V, double &X, double &Y, double &Z) // convert CIE L*u*v* value to CIE XYZ
{ // function checked OK with other calculators
    double l = L * 100.0;
    double u = U * 100.0;
    double v = V * 100.0;

    double u0 = 4.0 * CIE_ref_White_X / (CIE_ref_White_X + 15.0 * CIE_ref_White_Y + 3.0 * CIE_ref_White_Z); // white point intermediate values
    double v0 = 9.0 * CIE_ref_White_Y / (CIE_ref_White_X + 15.0 * CIE_ref_White_Y + 3.0 * CIE_ref_White_Z);

    double u_prime = u / (13.0 * l) + u0;
    double v_prime = v / (13.0 * l) + v0;

    if (l > CIE_KE)
        Y = CIE_ref_White_Y * (l + 16.0) / 116.0 * (l + 16.0) / 116.0 * (l + 16.0) / 116.0;
    else
        Y = CIE_ref_White_Y * l * 3.0 / 29.0 * 3.0 / 29.0 * 3.0 / 29.0;

    X = Y * 9.0 * u_prime / 4.0 / v_prime;
    Z = Y * (12.0 - 3.0 * u_prime - 20.0 * v_prime) / 4.0 / v_prime;
}

void CIELuvToStandard(const double &l, const double &u, const double &v, int &L, int &U, int &V) // convert CIELab [0..1] to CIELab L [0..100] u and v [-100..100]
{
    L = round(l * 100.0);
    U = round(u * 100.0);
    V = round(v * 100.0);
}

//// CIE LCHuv
//// See https://en.wikipedia.org/wiki/CIELUV#Cylindrical_representation_(CIELCH)
//// All values [0..1]
//// Common range : L [0..100] C [0..100+] h [0..360]
//// Note that L is exactly equal to L of CIE Luv

void CIELuvToCIELCHuv(const double &u, const double &v, double &C, double &H) // convert from Luv to LCHuv - L is the same so no need to convert
{ // function checked OK with other calculators
    C = sqrt(u * 100.0 * u * 100.0 + v * 100.0 * v * 100.0) / 100.0; // Chroma

    H = atan2(v, u) / pi_rad; // Hue - cartesian to polar
    while (H < 0.0) // Hue in range [0..1]
        H += 1.0;
    while (H >= 1.0)
        H -= 1.0;
}

void CIELCHuvToCIELUV(const double &C, const double &H, double &u, double &v) // convert from LCHuv to LUV - L is the same so no need to convert
{ // function checked OK with other calculators
    u = C * cos(H * pi_rad); // polar to cartesian
    v = C * sin(H * pi_rad);
}

void CIELCHuvtoStandard(const double &l, const double &c, const double &h, int &L, int &C, int &H) // convert CIE LCHuv [0..1] to CIE LCHuv L [0..100] C [0..100+] H [0..359]
{
    L = round(l * 100.0);
    C = round(c * 100.0);
    H = round(h * 360.0);
}

//// Hunter Lab (HLAB)
//// See https://en.wikipedia.org/wiki/CIELAB_color_space#Hunter_Lab
//// All values [0..1]
//// Common range : L [0..100] a [-100..100] b [-100..100]

void XYZtoHLAB(const double &X, const double &Y, const double &Z, double &L, double &A, double &B) // convert from XYZ to Hunter Lab
{ // function checked OK with other calculators
    if (Y == 0.0) { // lightness is 0 => no color
        L = 0.0;
        A = 0.0;
        B = 0.0;
    }
    else {
        double Ka = (175.0 / 198.04) * (CIE_ref_White_X + CIE_ref_White_Y); // CIE standard values VS white point
        double Kb = ( 70.0 / 218.11) * (CIE_ref_White_Y + CIE_ref_White_Z);

        L = sqrt(Y / CIE_ref_White_Y); // final values
        A = Ka * (((X / CIE_ref_White_X) - (Y / CIE_ref_White_Y)) / sqrt(Y / CIE_ref_White_Y));
        B = Kb * (((Y / CIE_ref_White_Y) - (Z / CIE_ref_White_Z)) / sqrt(Y / CIE_ref_White_Y));
    }
}

void HLABtoXYZ(const double &L, const double &A, const double &B, double &X, double &Y, double &Z) // convert from Hunter Lab to XYZ
{ // function checked OK with other calculators
    double Ka = (175.0 / 198.04) * (CIE_ref_White_Y + CIE_ref_White_X); // CIE standard values VS white point
    double Kb = ( 70.0 / 218.11) * (CIE_ref_White_Y + CIE_ref_White_Z);

    Y =   L / CIE_ref_White_Y * L / CIE_ref_White_Y; // final values
    X =  (A / Ka * sqrt(Y / CIE_ref_White_Y) + (Y / CIE_ref_White_Y)) * CIE_ref_White_X;
    Z = -(B / Kb * sqrt(Y / CIE_ref_White_Y) - (Y / CIE_ref_White_Y)) * CIE_ref_White_Z;

}

void HLABtoStandard(const double &l, const double &a, const double &b, int &L, int &A, int &B) // convert Hunter Lab [0..1] to Hunter Lab L [0..100] a and b [-100..100]
{
    L = round(l * 100.0);
    A = round(a * 100.0);
    B = round(b * 100.0);
}

//// CIE CAM02 LMS
//// See https://en.wikipedia.org/wiki/LMS_color_space
//// All values [0..1]
//// Common range : no range specified

void XYZtoLMS(const double &X, const double &Y, const double &Z, double &L, double &M, double &S) // convert from XYZ to LMS
{ // couldn't find any online calculator to check this function, but it is pretty straightforward
    // CIECAM02 is the successor to CIECAM97s - the best matrix so far, just coordinates change
    L =  0.7328 * X + 0.4296 * Y - 0.1624 * Z;
    M = -0.7036 * X + 1.6975 * Y + 0.0061 * Z;
    S =  0.0030 * X + 0.0136 * Y + 0.9834 * Z;
}

//// CMYK
//// See https://en.wikipedia.org/wiki/CMYK_color_model
//// All values [0..1]
//// Common range : C [0..100] M [0..100] Y [0..100] K [0..100]

double ClampCMYK(const double &value) // don't divide by 0 !
{
    if (value < 0.0 or isnan(value))
        return 0.0;
    else
        return value;
}

void RGBtoCMYK(const double &R, const double &G, const double &B, double &C, double &M, double &Y, double &K) // convert from RGB to CYMK
{
    K = ClampCMYK( 1.0 - std::max(std::max(R, G), B));
    C = ClampCMYK((1.0 - R - K) / (1.0 - K));
    M = ClampCMYK((1.0 - G - K) / (1.0 - K));
    Y = ClampCMYK((1.0 - B - K) / (1.0 - K));
}

void CMYKtoRGB(const double &C, const double &M, const double &Y, const double &K, double &R, double &G, double &B) // convert from CMYK to RGB
{
    R = (1.0 - C) * (1.0 - K);
    G = (1.0 - M) * (1.0 - K);
    B = (1.0 - Y) * (1.0 - K);

    // R, G and B must be in [0..1]
    R = GetValueRangeZeroOne(R);
    G = GetValueRangeZeroOne(G);
    B = GetValueRangeZeroOne(B);
}

void CMYKtoStandard(const double &c, const double &m, const double &y, const double &k, int &C, int &M, int &Y, int &K) // convert CMYK [0..1] to CMYK [0..100]
{
    C = round(c * 100.0);
    M = round(m * 100.0);
    Y = round(y * 100.0);
    K = round(k * 100.0);
}

//// RYB
//// from "Paint-like Compositing Based on RYB Color Model" by Junichi Sugit and Tokiichiro Takahashi (2015)
//// All values [0..1]
//// Common range : [0..255]
// Additional information :
//      Additive color mixing : just use C = C1 + C2 - e.g. : blue + yellow = green --- weighted additive mixing also works
//      Alpha blending : just use the good old formula - cALPHA = α * cFOREGROUND + (1 - α) * cBACKGROUND --- note that it is much less accurate than RGB alpha compositing
//                          watercolor : use α = 0.6
//                          gouache and thin acrylic : α = 0.8
//      Modified compositing : taking in account the thickness (weight) of the foreground layer, use the formula : cMODIFIED = cFOREGROUND + w * cBACKGROUND, which is about the same as weighted additive color mixing !
//                          watercolor : use w = 0.4
//                          gouache and thin acrylic : w = 0.2 to 0.3

void RGBtoRYBnaive(const double &Rrgb, const double &Grgb, const double &Brgb, double &Rryb, double &Yryb, double &Bryb) // convert RGB to RYB
{
    // special case : grays
    if ((Rrgb == Grgb) and (Grgb == Brgb)) {
        Rryb = 1.0 - Rrgb;
        Yryb = 1.0 - Grgb;
        Bryb = 1.0 - Brgb;
        return;
    }

    double rrgb, grgb, brgb, rryb, yryb, bryb, rrybp, yrybp, brybp;

    // remove whiteness
    double white = std::min(std::min(Rrgb, Grgb), Brgb); // "white" component
    rrgb = Rrgb - white;
    grgb = Grgb - white;
    brgb = Brgb - white;

    // ryb non-normalized values
    rryb = rrgb - std::min(rrgb, grgb); // remove yellow from red
    yryb = (grgb + std::min(rrgb, grgb)) / 2.0; // add green to red -> yellow
    bryb = (brgb + grgb - std::min(rrgb, grgb)) / 2.0; // add green to blue -> blue

    // normalize values
    double n = std::max(std::max(rryb, yryb), bryb) / std::max(std::max(rrgb, grgb), brgb); // normalization factor
    // n=0 means a gray value, case already treated at the beginning of the function
    rrybp = rryb / n;
    yrybp = yryb / n;
    brybp = bryb / n;

    // add blackness
    double black = std::min(std::min(1.0 - Rrgb, 1.0 - Grgb), 1.0 - Brgb); // "black" component
    Rryb = rrybp + black;
    Yryb = yrybp + black;
    Bryb = brybp + black;
}

void RYBtoRGBnaive(const double &Rryb, const double &Yryb, const double &Bryb, double &Rrgb, double &Grgb, double &Brgb) // convert RYB to RGB
{
    // special case : grays
    if ((Rryb == Yryb) and (Yryb == Bryb)) {
        Rrgb = 1.0 - Rryb;
        Grgb = 1.0 - Yryb;
        Brgb = 1.0 - Bryb;
        return;
    }

    double rrgb, grgb, brgb, rryb, yryb, bryb, rrgbp, grgbp, brgbp;

    // remove blackness
    double black = std::min(std::min(Rryb, Yryb), Bryb);
    rryb = Rryb - black;
    yryb = Yryb - black;
    bryb = Bryb - black;

    // rgb non-normalized values
    rrgb = rryb + yryb - std::min(yryb, bryb);
    grgb = yryb + std::min(yryb, bryb);
    brgb = 2.0 * (bryb - std::min(yryb, bryb));

    /* the 2017 source by the same authors is only "gRGB = yRYB + min(yRYB + bRYB)" ? But there is a factor of 2 in RGB to RYB conversion for yellow !
       this seems corroborated by https://web.archive.org/web/20130525061042/www.insanit.net/tag/rgb-to-ryb :

    # Get the green out of the yellow and blue
    g = min(y, b)
    y -= g
    b -= g

    if b and g:
        b *= 2.0
        g *= 2.0

    # Redistribute the remaining yellow.
    r += y
    g += y*/

    double n = std::max(std::max(rrgb, grgb), brgb) / std::max(std::max(rryb, yryb), bryb);
    // n=0 means a gray value, case already treated at the beginning of the function
    rrgbp = rrgb / n;
    grgbp = grgb / n;
    brgbp = brgb / n;

    // add whiteness
    double white = std::min(std::min(1.0 - Rryb, 1.0 - Yryb), 1.0 - Bryb);
    Rrgb = rrgbp + white;
    Grgb = grgbp + white;
    Brgb = brgbp + white;
}

void RGBtoRYB(const double &Rrgb, const double &Grgb, const double &Brgb, double &Rryb, double &Yryb, double &Bryb) // convert RGB to RYB
{
    /*// special case : grays
    if ((Rrgb == Grgb) and (Grgb == Brgb)) {
        Rryb = 1.0 - Rrgb;
        Yryb = 1.0 - Grgb;
        Bryb = 1.0 - Brgb;
        return;
    }*/

    // remove whiteness from color
    double w = std::min(std::min(Rrgb, Grgb), Brgb);
    double r = Rrgb - w;
    double g = Grgb - w;
    double b = Brgb - w;

    double mg = std::max(std::max(r, g), b);

    // get yellow out of red and green
    double y = std::min(r, g);
    r -= y;
    g -= y;

    // if this unfortunate conversion combines blue and green, then cut each in half to preserve the value's maximum range
    if ((b != 0) and (g != 0)) {
        b /= 2.0;
        g /= 2.0;
    }

    // redistribute remaining green
    y += g;
    b += g;

    // normalize to values
    double my = std::max(r, std::max(y, b));
    if (my != 0) {
        double n = mg / my;
        r *= n;
        y *= n;
        b *= n;
    }

    // add back in white
    r += w;
    y += w;
    b += w;

    Rryb = r;
    Yryb = y;
    Bryb = b;
}

void RYBtoRGB(const double &Rryb, const double &Yryb, const double &Bryb, double &Rrgb, double &Grgb, double &Brgb) // convert RYB to RGB
{
    /*// special case : grays
    if ((Rryb == Yryb) and (Yryb == Bryb)) {
        Rrgb = 1.0 - Rryb;
        Grgb = 1.0 - Yryb;
        Brgb = 1.0 - Bryb;
        return;
    }*/

    // remove whiteness from color
    double w = std::min(std::min(Rryb, Yryb), Bryb);
    double r = Rryb - w;
    double y = Yryb - w;
    double b = Bryb - w;

    double my = std::max(std::max(r, y), b);

    // get green out of yellow and blue
    double g = std::min(y, b);
    y -= g;
    b -= g;

    if ((b != 0) and (g != 0)) {
        b *= 2.0;
        g *= 2.0;
    }

    // redistribute remaining yellow
    r += y;
    g += y;

    // normalize to values
    double mg = std::max(std::max(r, g), b);
    if(mg != 0){
        double n = my / mg;
        r *= n;
        g *= n;
        b *= n;
    }

    // add white back in
    r += w;
    g += w;
    b += w;

    Rrgb = r;
    Grgb = g;
    Brgb = b;
}

//// OKLAB
//// See https://bottosson.github.io/posts/oklab/
//// Ranges L [0..1] a [-1..1] b [-1..1]
//// RGB space min and max : L [0 -> 1.0]    a [-0.233888 -> 0.276213]   b [-0.311528 -> 0.19857]
//// RGB colors for : aMin=green RGB(0,255,0) - aMax=magenta RGB(255,0,255) - bMin=blue RGb(0,0,255) - bMax=yellow RGB(255,255,0)
//// Common range for OKLab: not specified
// Does CIEDE2000 distance formula work with this space ???
// OKLAB is defined for D65 illumination, so it is compatible with all other conversion functions

void RGBtoOKLAB(const double &R, const double &G, const double &B, double &L, double &a, double &b) // convert RGB to OKLAB
{ // function not checked but seems to give good results
    double Rl, Gl, Bl; // for linear values of RGB

    RGBtoLinear(R, G, B, Rl, Gl, Bl); // gamma correction to linear sRGB

    // convert RGB to linear LMS - this matrix comes from : XYZ = M0 x RGB and LMS = M1 * XYZ, so M = M1 x M0
    double l = 0.4122214708 * Rl + 0.5363325363 * Gl + 0.0514459929 * Bl;
    double m = 0.2119034982 * Rl + 0.6806995451 * Gl + 0.1073969566 * Bl;
    double s = 0.0883024619 * Rl + 0.2817188376 * Gl + 0.6299787005 * Bl;

    // apply non-linearity : LMS to cubic root values
    l = std::cbrt(l);
    m = std::cbrt(m);
    s = std::cbrt(s);

    // transformation matrix from non-linear LMS to OKLAB
    L = 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s;
    a = 1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s;
    b = 0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s;
}

void OKLABtoLinearRGB(const double &L, const double &a, const double &b, double &Rl, double &Gl, double &Bl) // convert OKLAB to linear RGB
{ // function not checked
    // tranformation matrix gives cubic root of l, m and s
    double l = L + 0.3963377774 * a + 0.2158037573 * b;
    double m = L - 0.1055613458 * a - 0.0638541728 * b;
    double s = L - 0.0894841775 * a - 1.2914855480 * b;

    // real values of l, m and s are just the cube of values above
    l = l * l * l;
    m = m * m * m;
    s = s * s * s;

    // convert from LMS
    Rl = +4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s;
    Gl = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s;
    Bl = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s;
}

void RGBtoOKLAB(const int &R, const int &G, const int &B, double &L, double &a, double &b) // convert RGB to OKLAB - RGB is 8-bit !
{ // function not checked but seems to give good results
    double Rl, Gl, Bl; // for linear values of RGB

    RGBtoLinear_LUT(R, G, B, Rl, Gl, Bl); // gamma correction to linear sRGB

    // convert RGB to linear LMS - this matrix comes from : XYZ = M0 x RGB and LMS = M1 * XYZ, so M = M1 x M0
    double l = 0.4122214708 * Rl + 0.5363325363 * Gl + 0.0514459929 * Bl;
    double m = 0.2119034982 * Rl + 0.6806995451 * Gl + 0.1073969566 * Bl;
    double s = 0.0883024619 * Rl + 0.2817188376 * Gl + 0.6299787005 * Bl;

    // apply non-linearity : LMS to cubic root values
    l = std::cbrt(l);
    m = std::cbrt(m);
    s = std::cbrt(s);

    // transformation matrix from non-linear LMS to OKLAB
    L = 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s;
    a = 1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s;
    b = 0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s;
}

double compute_max_saturation(const double &a, const double &b)
{
    // Max saturation will be when one of r, g or b goes below zero

    // Select different coefficients depending on which component goes below zero first
    double k0, k1, k2, k3, k4, wl, wm, ws;

    if (-1.88170328 * a - 0.80936493 * b > 1) {
        // Red component
        k0 = +1.19086277;
        k1 = +1.76576728;
        k2 = +0.59662641;
        k3 = +0.75515197;
        k4 = +0.56771245;
        wl = +4.0767416621;
        wm = -3.3077115913;
        ws = +0.2309699292;
    }
    else if (1.81444104 * a - 1.19445276 * b > 1)
    {
        // Green component
        k0 = +0.73956515;
        k1 = -0.45954404;
        k2 = +0.08285427;
        k3 = +0.12541070;
        k4 = +0.14503204;
        wl = -1.2681437731;
        wm = +2.6097574011;
        ws = -0.3413193965;
    }
    else
    {
        // Blue component
        k0 = +1.35733652;
        k1 = -0.00915799;
        k2 = -1.15130210;
        k3 = -0.50559606;
        k4 = +0.00692167;
        wl = -0.0041960863;
        wm = -0.7034186147;
        ws = +1.7076147010;
    }

    // Approximate max saturation using a polynomial:
    double S = k0 + k1 * a + k2 * b + k3 * a * a + k4 * a * b;

    // Do one step Halley's method to get closer
    // this gives an error less than 10e6, except for some blue hues where the dS/dh is close to infinite
    // this should be sufficient for most applications, otherwise do two/three steps

    double k_l = +0.3963377774 * a + 0.2158037573 * b;
    double k_m = -0.1055613458 * a - 0.0638541728 * b;
    double k_s = -0.0894841775 * a - 1.2914855480 * b;

    {
        double l_ = 1.0 + S * k_l;
        double m_ = 1.0 + S * k_m;
        double s_ = 1.0 + S * k_s;

        double l = l_ * l_ * l_;
        double m = m_ * m_ * m_;
        double s = s_ * s_ * s_;

        double l_dS = 3.0 * k_l * l_ * l_;
        double m_dS = 3.0 * k_m * m_ * m_;
        double s_dS = 3.0 * k_s * s_ * s_;

        double l_dS2 = 6.0 * k_l * k_l * l_;
        double m_dS2 = 6.0 * k_m * k_m * m_;
        double s_dS2 = 6.0 * k_s * k_s * s_;

        double f  = wl * l     + wm * m     + ws * s;
        double f1 = wl * l_dS  + wm * m_dS  + ws * s_dS;
        double f2 = wl * l_dS2 + wm * m_dS2 + ws * s_dS2;

        S = S - f * f1 / (f1 * f1 - 0.5 * f * f2);
    }

    return S;
}

void find_cusp(const double &a, const double &b, double &L_cusp, double &C_cusp)
{
    // First, find the maximum saturation (saturation S = C/L)
    double S_cusp = compute_max_saturation(a, b);

    // Convert to linear sRGB to find the first point where at least one of r,g or b >= 1:
    double R, G, B;
    OKLABtoLinearRGB(1.0, S_cusp * a, S_cusp * b, R, G, B);
    L_cusp = std::cbrt(1.0 / std::max(std::max(R, G), B));
    C_cusp = L_cusp * S_cusp;
}

double find_gamut_intersection(const double &a, const double &b, const double &L1, const double &C1, const double &L0)
{
    // Find the cusp of the gamut triangle
    double L_cusp, C_cusp;
    find_cusp(a, b, L_cusp, C_cusp);

    // Find the intersection for upper and lower half seprately
    double t;
    if (((L1 - L0) * C_cusp - (L_cusp - L0) * C1) <= 0.0) {
        // Lower half
        t = C_cusp * L0 / (C1 * L_cusp + C_cusp * (L0 - L1));
    }
    else {
        // Upper half

        // First intersect with triangle
        t = C_cusp * (L0 - 1.0) / (C1 * (L_cusp - 1.0) + C_cusp * (L0 - L1));

        // Then one step Halley's method
        {
            double dL = L1 - L0;
            double dC = C1;

            double k_l = +0.3963377774 * a + 0.2158037573 * b;
            double k_m = -0.1055613458 * a - 0.0638541728 * b;
            double k_s = -0.0894841775 * a - 1.2914855480 * b;

            double l_dt = dL + dC * k_l;
            double m_dt = dL + dC * k_m;
            double s_dt = dL + dC * k_s;


            // If higher accuracy is required, 2 or 3 iterations of the following block can be used:
            {
                double L = L0 * (1.0 - t) + t * L1;
                double C = t * C1;

                double l_ = L + C * k_l;
                double m_ = L + C * k_m;
                double s_ = L + C * k_s;

                double l = l_ * l_ * l_;
                double m = m_ * m_ * m_;
                double s = s_ * s_ * s_;

                double ldt = 3.0 * l_dt * l_ * l_;
                double mdt = 3.0 * m_dt * m_ * m_;
                double sdt = 3.0 * s_dt * s_ * s_;

                double ldt2 = 6.0 * l_dt * l_dt * l_;
                double mdt2 = 6.0 * m_dt * m_dt * m_;
                double sdt2 = 6.0 * s_dt * s_dt * s_;

                double r  = 4.0767416621 * l    - 3.3077115913 * m    + 0.2309699292 * s - 1.0;
                double r1 = 4.0767416621 * ldt  - 3.3077115913 * mdt  + 0.2309699292 * sdt;
                double r2 = 4.0767416621 * ldt2 - 3.3077115913 * mdt2 + 0.2309699292 * sdt2;

                double u_r = r1 / (r1 * r1 - 0.5 * r * r2);
                double t_r = -r * u_r;

                double g  = -1.2681437731 * l    + 2.6097574011 * m    - 0.3413193965 * s - 1.0;
                double g1 = -1.2681437731 * ldt  + 2.6097574011 * mdt  - 0.3413193965 * sdt;
                double g2 = -1.2681437731 * ldt2 + 2.6097574011 * mdt2 - 0.3413193965 * sdt2;

                double u_g = g1 / (g1 * g1 - 0.5 * g * g2);
                double t_g = -g * u_g;

                double b  = -0.0041960863 * l    - 0.7034186147 * m    + 1.7076147010 * s - 1.0;
                double b1 = -0.0041960863 * ldt  - 0.7034186147 * mdt  + 1.7076147010 * sdt;
                double b2 = -0.0041960863 * ldt2 - 0.7034186147 * mdt2 + 1.7076147010 * sdt2;

                double u_b = b1 / (b1 * b1 - 0.5 * b * b2);
                double t_b = -b * u_b;

                t_r = u_r >= 0.0 ? t_r : DBL_MAX;
                t_g = u_g >= 0.0 ? t_g : DBL_MAX;
                t_b = u_b >= 0.0 ? t_b : DBL_MAX;

                t += std::min(t_r, std::min(t_g, t_b));
            }
        }
    }

    return t;
}

double sgn(const double &x)
{
    return (double)(0.0 < x) - (double)(x < 0.0);
}

void OKLABtoRGB(const double &L, const double &a, const double &b, double &R, double &G, double &B, const bool &clip, const float &alpha) // convert OKLAB to RGB
    // clip=true means the best value for R, G and B are searched with gamut clipping, using alpha value - this is slower but more accurate
{ // function not checked
    // tranformation matrix gives cubic root of l, m and s
    double l = L + 0.3963377774 * a + 0.2158037573 * b;
    double m = L - 0.1055613458 * a - 0.0638541728 * b;
    double s = L - 0.0894841775 * a - 1.2914855480 * b;

    // real values of l, m and s are just the cube of values above
    l = l * l * l;
    m = m * m * m;
    s = s * s * s;

    double Rl, Gl, Bl; // for linear values of RGB

    // convert from LMS
    Rl = +4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s;
    Gl = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s;
    Bl = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s;

    // now we have values in sRGB

    if ((clip) and ((Rl < 0) or (Rl > 1.0) or (Gl < 0) or (Gl > 1.0) or (Bl < 0) or (Bl > 1.0))) {
        double C = std::max(0.00001, sqrt(a * a + b * b)); // chroma
        double a_ = a / C;
        double b_ = b / C;

        // gamut_clip_adaptive_L0_0_5 - https://bottosson.github.io/posts/gamutclipping/
        double Ld = L - 0.5;
        double e1 = 0.5 + fabs(Ld) + alpha * C;
        double L0 = 0.5 * (1.0 + sgn(Ld) * (e1 - sqrt(e1 * e1 - 2.0 * fabs(Ld))));

        double t = find_gamut_intersection(a_, b_, L, C, L0);
        double L_clipped = L0 * (1.0 - t) + t * L;
        double C_clipped = t * C;

        OKLABtoLinearRGB(L_clipped, C_clipped * a_, C_clipped * b_, Rl, Gl, Bl);

        /*// gamut_clip_adaptive_L0_L_cusp - https://bottosson.github.io/posts/gamutclipping/
        // The cusp is computed here and in find_gamut_intersection, an optimized solution would only compute it once
        double L_cusp, C_cusp;
        find_cusp(a_, b_, L_cusp, C_cusp);

        double Ld = L - L_cusp;
        double k = 2.0 * (Ld > 0 ? 1.0 - L_cusp : L_cusp);

        double e1 = 0.5 * k + fabs(Ld) + alpha * C / k;
        double L0 = L_cusp + 0.5 * (sgn(Ld) * (e1 - sqrt(e1 * e1 - 2.0 * k * fabs(Ld))));

        double t = find_gamut_intersection(a_, b_, L, C, L0);
        double L_clipped = L0 * (1.0 - t) + t * L;
        double C_clipped = t * C;

        OKLABtoLinearRGB(L_clipped, C_clipped * a_, C_clipped * b_, Rl, Gl, Bl);*/
    }

    // clipped or not, we have sRGB values now

    LinearToRGB(Rl, Gl, Bl, R, G, B); // gamma correction from linear sRGB

    if (!clip) {
        R = GetValueRangeZeroOne(R);
        G = GetValueRangeZeroOne(G);
        B = GetValueRangeZeroOne(B);
    }
}

void OKLABtoStandard(const double &l, const double &a, const double &b, int &L, int &A, int &B) // convert OKLAB [0..1] to OKLAB L [0..100] A and B [-128..127]
{
    L = round(l * 100.0);
    A = round(a * 127.0);
    B = round(b * 127.0);
}

void XYZtoOKLAB(const double &X, const double &Y, const double &Z, double &L, double &a, double &b) // convert from XYZ to OKLAB
    // see https://bottosson.github.io/posts/oklab/
{
    // convert XYZ to linear LMS
    double l = +0.8189330101 * X + 0.3618667424 * Y - 0.1288597137 * Z;
    double m = +0.0329845436 * X + 0.9293118715 * Y + 0.0361456387 * Z;
    double s = +0.0482003018 * X + 0.2643662691 * Y + 0.6338517070 * Z;

    // apply non-linearity : LMS to cubic root values
    l = std::cbrt(l);
    m = std::cbrt(m);
    s = std::cbrt(s);

    // transformation matrix from non-linear LMS to OKLAB
    L = +0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s;
    a = +1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s;
    b = +0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s;
}

void OKLABtoXYZ(const double &L, const double &a, const double &b, double &X, double &Y, double &Z) // convert OKLAB to XYZ
    // see https://bottosson.github.io/posts/oklab/
{
    // tranformation matrix gives cubic root of l, m and s (non-linear)
    double l = L + 0.3963377774 * a + 0.2158037573 * b;
    double m = L - 0.1055613458 * a - 0.0638541728 * b;
    double s = L - 0.0894841775 * a - 1.2914855480 * b;

    // real values of l, m and s are just the cube of values above -> linear
    l = l * l * l;
    m = m * m * m;
    s = s * s * s;

    // convert from LMS to XYZ - inverse matrix calculated from https://matrix.reshish.com/fr/inverCalculation.php
    X = +1.227013851103521026    * l - 0.5577999806518222383  * m + 0.28125614896646780758  * s;
    Y = -0.040580178423280593977 * l + 1.1122568696168301049  * m - 0.071676678665601200577 * s;
    Z = -0.076381284505706892869 * l - 0.42148197841801273055 * m + 1.5861632204407947575   * s;
}

//// OKLCH
//// See https://bottosson.github.io/posts/oklab/
//// All values [0..1]
//// RGB space min and max : L [0 -> 1.0]    C [0 -> 0.322491]   H [0 -> 1.0]
//// RGB colors for : CMin=black RGB(0,0,0) - CMax=magenta RGB(255,0,255)
//// Common range : not specified
//// Note that L is exactly equal to L of OKLAB

void OKLABtoOKLCH(const double &A, const double &B, double &C, double &H) // convert from OKLAB to OKLCH - L is the same so no need to convert
{
    C = sqrt(A * A + B * B); // chroma

    H = atan2(B, A) / pi_rad; // Hue - cartesian to polar
    while (H < 0.0) // Hue in range [0..1]
        H += 1.0;
    while (H >= 1.0)
        H -= 1.0;
}

void OKLCHtoOKLAB(const double &C, const double &H, double &A, double &B) // convert from OKLCH to OKLAB - L is the same so no need to convert
{
    A = C * cos(H * pi_rad); // polar to cartesian
    B = C * sin(H * pi_rad);
}

void OKLCHtoStandard(const double &l, const double &c, const double &h, int &L, int &C, int &H) // convert OKLCH [0..1] to OKLCH L [0..100] C [0..100+] H [0..359]
{
    L = round(l * 100.0);
    C = round(c * 127.0); // because a and b from OKLAB are in [-128..127]
    H = round(h * 360.0);
}

void RGBtoOKLCH(const double &R, const double &G, const double &B, double &L, double &C, double &H) // convert RGB to OKLCH
{
    double a, b;
    RGBtoOKLAB(R, G, B, L, a, b);
    OKLABtoOKLCH(a, b, C, H);
}

void OKLCHtoRGB(const double &L, const double &C, const double &H, double &R, double &G, double &B, const bool &clip, const float &alpha) // convert OKLCH to RGB
{
    double a, b;
    OKLCHtoOKLAB(C, H, a, b);
    OKLABtoRGB(L, a, b, R, G, B, clip, alpha);
}

void RGBtoOKLCH(const int &R, const int &G, const int &B, double &L, double &C, double &H) // convert RGB to OKLCH - RGB is 8-bit !
{
    double a, b;
    RGBtoOKLAB(R, G, B, L, a, b);
    OKLABtoOKLCH(a, b, C, H);
}


