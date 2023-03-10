/*#-------------------------------------------------
#
#     Color images tools library with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v2.0 - 2022/12/12
#
#   - color spaces conversions for images
#   - Gradients
#   - Means for images
#   - Dominant colors for images
#
#-------------------------------------------------*/

#include <fstream>

#include "image-color.h"


///////////////////////////////////////////////////////////
//// Color mean
///////////////////////////////////////////////////////////

cv::Vec3b ColorMean(const cv::Mat &source, const int &sX, const int &sY, const int &border) // computes the mean of one pixel in BGR image, using border pixels around it - uses OKLAB
{
    int R, G, B;
    cv::Vec3b color = source.at<cv::Vec3b>(sY, sX);
    //R = color[2] / 255.0;
    //G = color[1] / 255.0;
    //B = color[0] / 255.0;

    for (int j = sY - border; j <= sY + border; j++) {
        if ((j > 0) and (j < source.rows)) {
            const cv::Vec3b* sourceP = source.ptr<cv::Vec3b>(j);
            for (int i = sX - border; i <= sX + border; i++) {
                if (((i != sX) and (j != sY)) and ((i > 0) and (i < source.cols))) {
                    RGBMeanWithOKLAB(color[2], color[1], color[0], 1, sourceP[i][2], sourceP[i][1], sourceP[i][0], 1, R, G, B);
                }
            }
        }
    }
    return cv::Vec3b(B, G, R);
}

cv::Vec3b ColorMeanCross(const cv::Mat &source, const int &sX, const int &sY) // computes the mean of one pixel in BGR image, using pixels around it, cross-shaped - uses OKLAB
{
    double R, G, B;
    cv::Vec3b color = source.at<cv::Vec3b>(sY, sX);
    R = color[2] / 255.0;
    G = color[1] / 255.0;
    B = color[0] / 255.0;

    int i, j;

    // up
    i = sX;
    j = sY - 1;
    if ((j > 0) and (j < source.rows) and (i > 0) and (i < source.cols)) {
        color = source.at<cv::Vec3b>(j, i);
        RGBMeanWithOKLAB(R, G, B, 1, color[2] / 255.0, color[1] / 255.0, color[0] / 255.0, 1, R, G, B);
    }

    // down
    i = sX;
    j = sY + 1;
    if ((j > 0) and (j < source.rows) and (i > 0) and (i < source.cols)) {
        color = source.at<cv::Vec3b>(j, i);
        RGBMeanWithOKLAB(R, G, B, 1, color[2] / 255.0, color[1] / 255.0, color[0] / 255.0, 1, R, G, B);
    }

    // left
    i = sX - 1;
    j = sY;
    if ((j > 0) and (j < source.rows) and (i > 0) and (i < source.cols)) {
        color = source.at<cv::Vec3b>(j, i);
        RGBMeanWithOKLAB(R, G, B, 1, color[2] / 255.0, color[1] / 255.0, color[0] / 255.0, 1, R, G, B);
    }

    // right
    i = sX + 1;
    j = sY;
    if ((j > 0) and (j < source.rows) and (i > 0) and (i < source.cols)) {
        color = source.at<cv::Vec3b>(j, i);
        RGBMeanWithOKLAB(R, G, B, 1, color[2] / 255.0, color[1] / 255.0, color[0] / 255.0, 1, R, G, B);
    }

    return cv::Vec3b(round(B * 255.0), round(G * 255.0), round(R * 255.0));
}

///////////////////////////////////////////////////////////
//// Dominant colors
///////////////////////////////////////////////////////////

cv::Vec3d ImageDominantColor(const cv::Mat &lab) // get dominant color from OKLab or CIELab image
{
    cv::Mat quantized;
    std::vector<cv::Vec3d> paletteDominant = DominantColorsEigen(lab, 1, quantized);

    return paletteDominant[0]; // return OKLab or CIELab value
}

///////////////////////////////////////////////////////////
//// Image color utils
///////////////////////////////////////////////////////////

cv::Mat ConvertImageLabToGray(const cv::Mat &source) // get a gray image from CV_64FC3 OKLab or CIELab image
{
    cv::Mat result;
    result.create(source.rows, source.cols, CV_8UC1);

    const cv::Vec3d *sourceP = source.ptr<cv::Vec3d>(0);
    uchar *resultP = result.ptr<uchar>(0);

    int total = source.total();
    for (int n = 0; n < total; n++)
        resultP[n] = GetByteInRange(sourceP[n][0] * 255.0);

    return result;
}


///////////////////////////////////////////////////////////
//// Conversion of images to other colors spaces
///////////////////////////////////////////////////////////

cv::Mat ConvertImageRGBtoCIELab(const cv::Mat &source) // convert RGB image to CIELab
{
    cv::Mat dest(source.rows, source.cols, CV_64FC3); // CIELab "image" values

    int total = source.cols * source.rows;
    const cv::Vec3b* sourceP = source.ptr<cv::Vec3b>(0);
    cv::Vec3d* destP = dest.ptr<cv::Vec3d>(0);
    for (int n = 0; n < total; n++)
        RGBtoCIELab(sourceP[n][2], sourceP[n][1], sourceP[n][0], destP[n][0], destP[n][1], destP[n][2]); // convert RGB to CIELab

    return dest;
}

cv::Mat ConvertImageRGBtoCIELCHab(const cv::Mat &source) // convert RGB image to CIE LCHab
{
    cv::Mat dest(source.rows, source.cols, CV_64FC3); // CIELab "image" values

    int total = source.cols * source.rows;
    const cv::Vec3b* sourceP = source.ptr<cv::Vec3b>(0);
    cv::Vec3d* destP = dest.ptr<cv::Vec3d>(0);
    for (int n = 0; n < total; n++)
        RGBtoCIELCHab(sourceP[n][2], sourceP[n][1], sourceP[n][0], destP[n][0], destP[n][1], destP[n][2]); // convert RGB to LCHab

    return dest;
}

cv::Mat ConvertImageRGBtoOKLAB(const cv::Mat &source) // convert RGB image to OKLAB
{
    cv::Mat dest(source.rows, source.cols, CV_64FC3); // OKLAB "image" values

    int total = source.cols * source.rows;
    const cv::Vec3b* sourceP = source.ptr<cv::Vec3b>(0);
    cv::Vec3d* destP = dest.ptr<cv::Vec3d>(0);
    #pragma omp parallel for
    for (int n = 0; n < total; n++)
        RGBtoOKLAB(sourceP[n][2], sourceP[n][1], sourceP[n][0], destP[n][0], destP[n][1], destP[n][2]); // convert RGB to OKLAB

    return dest;
}

cv::Mat ConvertImageCIELabToRGB(const cv::Mat &source) // convert Lab image to RGB
{
    double R, G, B;
    cv::Mat dest(source.rows, source.cols, CV_8UC3); // this will be the final RGB image

    int total = source.cols * source.rows;
    const cv::Vec3d* sourceP = source.ptr<cv::Vec3d>(0);
    cv::Vec3b* destP = dest.ptr<cv::Vec3b>(0);
    for (int n = 0; n < total; n++) {
        CIELabToRGB(sourceP[n][0], sourceP[n][1], sourceP[n][2], R, G, B); // convert CIELab to RGB value
        destP[n][2] = round(R * 255.0); // write RGB values to dest image
        destP[n][1] = round(G * 255.0);
        destP[n][0] = round(B * 255.0);
    }

    return dest;
}

cv::Mat ConvertImageCIELCHabToRGB(const cv::Mat &source) // convert CIE LCHab image to RGB
{
    double R, G, B;
    cv::Mat dest(source.rows, source.cols, CV_8UC3); // this will be the final RGB image

    int total = source.cols * source.rows;
    const cv::Vec3d* sourceP = source.ptr<cv::Vec3d>(0);
    cv::Vec3b* destP = dest.ptr<cv::Vec3b>(0);
    for (int n = 0; n < total; n++) {
        CIELCHabToRGB(sourceP[n][0], sourceP[n][1], sourceP[n][2], R, G, B); // convert LCHab to RGB value
        destP[n][2] = round(R * 255.0); // write RGB values to dest image
        destP[n][1] = round(G * 255.0);
        destP[n][0] = round(B * 255.0);
    }

    return dest;
}

cv::Mat ConvertImageOKLABtoRGB(const cv::Mat &source, const bool &clip, const double &alpha) // convert OKLAB image to RGB
    // clip=true means the best value for R, G and B are searched with gamut clipping, using alpha value - this is slower but more accurate
{
    double R, G, B;
    cv::Mat dest(source.rows, source.cols, CV_8UC3); // this will be the final RGB image

    int total = source.cols * source.rows;
    const cv::Vec3d* sourceP = source.ptr<cv::Vec3d>(0);
    cv::Vec3b* destP = dest.ptr<cv::Vec3b>(0);
    int n;
    #pragma omp parallel for private(n, R, G, B)
    for (n = 0; n < total; n++) {
        OKLABtoRGB(sourceP[n][0], sourceP[n][1], sourceP[n][2], R, G, B, clip, alpha); // convert OKLAB to RGB value
        destP[n][2] = round(R * 255.0); // write RGB values to dest image
        destP[n][1] = round(G * 255.0);
        destP[n][0] = round(B * 255.0);
    }

    return dest;
}

cv::Mat ConvertImageRGBtoCIEHSLC(const cv::Mat &source, const bool &clampValues) // convert RGB image to HSLC (H from HSL, S L and C from CIELab)
{
    cv::Mat dest(source.rows, source.cols, CV_64FC4); //
    double h, a, b;

    int total = source.cols * source.rows;
    const cv::Vec3b* sourceP = source.ptr<cv::Vec3b>(0);
    cv::Vec4d* destP = dest.ptr<cv::Vec4d>(0);
    for (int n = 0; n < total; n++) {
        CIEHSLChfromRGB(sourceP[n][2], sourceP[n][1], sourceP[n][0], destP[n][0], destP[n][1], destP[n][2], destP[n][3], h, a, b); //

        if (clampValues) {
            for (int c = 0; c < 4; c++) {
                if (destP[n][c] != -1)
                    destP[n][c] = GetValueRangeZeroOne(destP[n][c]);
            }
        }
    }

    return dest;
}

cv::Mat ConvertImageRGBtoOKLABHSLC(const cv::Mat &source, const bool &clampValues) // convert RGB image to HSLC (H from HSL, S L and C from OKLAB)
{
    cv::Mat dest(source.rows, source.cols, CV_64FC4); //
    double h, a, b;

    int total = source.cols * source.rows;
    const cv::Vec3b* sourceP = source.ptr<cv::Vec3b>(0);
    cv::Vec4d* destP = dest.ptr<cv::Vec4d>(0);
    for (int n = 0; n < total; n++) {
        OKLABHSLChfromRGB(sourceP[n][2], sourceP[n][1], sourceP[n][0], destP[n][0], destP[n][1], destP[n][2], destP[n][3], h, a, b); //

        if (clampValues) {
            for (int c = 0; c < 4; c++) {
                if (destP[n][c] != -1)
                    destP[n][c] = GetValueRangeZeroOne(destP[n][c]);
            }
        }
    }

    return dest;
}

void CreateCIELabPalettefromRGB(const int &Rvalue, const int &Gvalue, const int &Bvalue, const int &paletteSize, const int &sections,
                                const std::string filename, const bool &grid, const int &gap, const bool &invertCL) // test : create Lightness * Chroma palette image for one RGB color with CIELab
{
    cv::Mat palette = cv::Mat::zeros(cv::Size(paletteSize + 100, paletteSize + 100), CV_8UC3); // create blank image
    double unit = 1.0 / sections;
    double L, a, b, X, Y, Z, R, G, B, C, H;
    RGBtoXYZ(Rvalue, Gvalue, Bvalue, X, Y, Z);
    XYZtoCIELab(X, Y, Z, L, a, b);
    CIELabToCIELCHab(a, b, C, H);
    for (int l = 0; l <= sections; l++) { // for each L section
        for (int c = 0; c <= sections; c++) { // for each C section
            if (!invertCL) {
                C = unit * c;
                L = unit * l;
            }
            else {
                L = unit * c;
                C = unit * l;
            }

            CIELCHabToCIELab(C, H, a, b);
            CIELabToXYZ(L, a, b, X, Y, Z);
            XYZtoRGBNoClipping(X, Y, Z, R, G, B); // when RGB are out of gamut, return black (0,0,0)

            if (grid) { // draw grid ?
                if ((l % 10) == 0) {
                    cv::line(palette, cv::Point(round(l * unit * paletteSize), 0), cv::Point(round(l * unit * paletteSize), paletteSize), cv::Vec3b(32, 32, 32), 1);
                }
                else
                    if ((l % 5) == 0) {
                        cv::line(palette, cv::Point(round(l * unit * paletteSize), 0), cv::Point(round(l * unit * paletteSize), paletteSize), cv::Vec3b(8, 8, 8), 1);
                }
                if ((c % 10) == 0) {
                    cv::line(palette, cv::Point(0, round(c * unit * paletteSize)), cv::Point(paletteSize, round(c * unit * paletteSize)), cv::Vec3b(32, 32, 32), 1);
                }
                else
                    if ((c % 5) == 0) {
                        cv::line(palette, cv::Point(0, round(c * unit * paletteSize)), cv::Point(paletteSize, round(c * unit * paletteSize)), cv::Vec3b(8, 8, 8), 1);
                    }
            }
            cv::rectangle(palette, cv::Rect(round(l * unit * paletteSize), paletteSize - round(c * unit * paletteSize),
                                            round(unit * paletteSize) - gap, round(unit * paletteSize) - gap),
                                        cv::Vec3b(round(B * 255.0), round(G * 255.0), round(R * 255.0)), -1); // rectangle of current color
        }
    }
    cv::imwrite("LAB-palette-" + filename + ".png", palette); // write palette to file
}

void CreateOKLABpalettefromRGB(const int &Rvalue, const int &Gvalue, const int &Bvalue, const int &paletteSize, const int &sections,
                                const std::string filename, const bool &grid, const int &gap, const bool &invertCL) // test : create Lightness * Chroma palette image for one RGB color with OKLAB
{
    cv::Mat palette = cv::Mat::zeros(cv::Size(paletteSize + 100, paletteSize + 100), CV_8UC3); // create blank image
    double unit = 1.0 / sections;
    double L, a, b, X, Y, Z, R, G, B, C, H;
    RGBtoXYZ(Rvalue, Gvalue, Bvalue, X, Y, Z);
    XYZtoOKLAB(X, Y, Z, L, a, b);
    OKLABtoOKLCH(a, b, C, H);
    for (int l = 0; l <= sections; l++) { // for each L section
        for (int c = 0; c <= sections; c++) { // for each C section
            if (!invertCL) {
                C = unit * c;
                L = unit * l;
            }
            else {
                L = unit * c;
                C = unit * l;
            }

            OKLCHtoOKLAB(C, H, a, b);
            OKLABtoXYZ(L, a, b, X, Y, Z);
            XYZtoRGBNoClipping(X, Y, Z, R, G, B); // when RGB are out of gamut, return black (0,0,0)

            if (grid) { // draw grid ?
                if ((l % 10) == 0) {
                    cv::line(palette, cv::Point(round(l * unit * paletteSize), 0), cv::Point(round(l * unit * paletteSize), paletteSize), cv::Vec3b(32, 32, 32), 1);
                }
                else
                    if ((l % 5) == 0) {
                        cv::line(palette, cv::Point(round(l * unit * paletteSize), 0), cv::Point(round(l * unit * paletteSize), paletteSize), cv::Vec3b(8, 8, 8), 1);
                }
                if ((c % 10) == 0) {
                    cv::line(palette, cv::Point(0, round(c * unit * paletteSize)), cv::Point(paletteSize, round(c * unit * paletteSize)), cv::Vec3b(32, 32, 32), 1);
                }
                else
                    if ((c % 5) == 0) {
                        cv::line(palette, cv::Point(0, round(c * unit * paletteSize)), cv::Point(paletteSize, round(c * unit * paletteSize)), cv::Vec3b(8, 8, 8), 1);
                    }
            }
            cv::rectangle(palette, cv::Rect(round(l * unit * paletteSize), paletteSize - round(c * unit * paletteSize),
                                            round(unit * paletteSize) - gap, round(unit * paletteSize) - gap),
                                        cv::Vec3b(round(B * 255.0), round(G * 255.0), round(R * 255.0)), -1); // rectangle of current color
        }
    }
    cv::imwrite("OKLAB-palette-" + filename + ".png", palette); // write palette to file
}

void AnalyzeCIELabCurveImage(const int &sections, const std::string filename) // create CSV file of maximum Chroma values for each Lighness step from Lightness * Chroma palette image (CIELAB)
{
    cv::Mat palette = cv::imread(filename + ".png");
    int size = palette.cols / sections;

    std::ofstream saveCSV; // file to save
    saveCSV.open(filename + ".csv"); // save data file
    if (saveCSV) { // if successfully open
        saveCSV << "L-orig;C-orig;H;S;L;C;h\n"; //header

        for (int x = 0; x < sections; x++) { // parse image horizontaly
            int y = 0;
            while ((y < sections) and (palette.at<cv::Vec3b>(y * size + size / 2, x * size + size / 2) == cv::Vec3b(0, 0, 0))) // find first non-zero value
                y++;
            if (y == 100) // not found ?
                y = 99;

            cv::Vec3b color = palette.at<cv::Vec3b>(y * size + size / 2, x * size + size / 2); // get "maximum" pixel value

            double H, S, L, C, h, a, b;
            CIEHSLChfromRGB(color[2] / 255.0, color[1] / 255.0, color[0] / 255.0, H, S, L, C, h, a, b); // convert RGB

            saveCSV << x << ";" << 99 - y << ";" << H * 360 << ";" << S * 100 << ";" << L * 100 << ";" << C * 100 << ";" << h * 360 << "\n"; // write result to file
        }

        saveCSV.close(); // close text file
    }
}

void AnalyzeOKLABcurveImage(const int &sections, const std::string filename) // create CSV file of maximum Chroma values for each Lighness step from Lightness * Chroma palette image (OKLAB)
{
    cv::Mat palette = cv::imread(filename + ".png");
    int size = palette.cols / sections;

    std::ofstream saveCSV; // file to save
    saveCSV.open(filename + ".csv"); // save data file
    if (saveCSV) { // if successfully open
        saveCSV << "L-orig;C-orig;H;S;L;C;h\n"; //header

        for (int x = 0; x < sections; x++) { // parse image horizontaly
            int y = 0;
            while ((y < sections) and (palette.at<cv::Vec3b>(y * size + size / 2, x * size + size / 2) == cv::Vec3b(0, 0, 0))) // find first non-zero value
                y++;
            if (y == 100) // not found ?
                y = 99;

            cv::Vec3b color = palette.at<cv::Vec3b>(y * size + size / 2, x * size + size / 2); // get "maximum" pixel value

            double H, S, L, C, h, a, b;
            OKLABHSLChfromRGB(color[2] / 255.0, color[1] / 255.0, color[0] / 255.0, H, S, L, C, h, a, b); // convert RGB
            //RGBtoOKLCH(color[2] / 255.0, color[1] / 255.0, color[0] / 255.0, L, C, h);

            saveCSV << x << ";" << 99 - y << ";" << H * 360 << ";" << S * 100 << ";" << L * 100 << ";" << C * 100 << ";" << h * 360 << "\n"; // write result to file
        }

        saveCSV.close(); // close text file
    }
}

cv::Vec3b RGBtoLinear(const cv::Vec3b &color) // Apply linear RGB gamma correction to sRGB with RGB in [0..255] (faster)
{
    return cv::Vec3b(round(RGBlinearLUT[color[0]] * 255.0), round(RGBlinearLUT[color[1]] * 255.0), round(RGBlinearLUT[color[2]] * 255.0));
}

cv::Vec3b LinearToRGB(const cv::Vec3b &color) // Apply linear gamma correction from sRGB with RGB in [0..255] (faster)
{
    double r, g, b;
    LinearToRGB(color[2] / 255.0, color[1] / 255.0, color[0] / 255.0, r, g, b);

    return cv::Vec3b(round(b * 255.0), round(g * 255.0), round(r * 255.0));
}

cv::Mat ConvertImageRGBtoLinear(const cv::Mat &source) // convert CV_64FC3 RGB image [0..1] to linear [0..1]
{
    cv::Mat dest(source.rows, source.cols, CV_64FC3); // this will be the delinearized image
    const double* sourceP = source.ptr<double>(0);
    double* destP = dest.ptr<double>(0);
    int total = source.cols * source.rows * 3;

    for (int n = 0; n < total; n++)
        destP[n] = RGBlinearLUT[int(round(sourceP[n] * 255.0))];

    return dest;
}

cv::Mat ConvertImageLinearToRGB(const cv::Mat &source) // convert RGB image to linear (source is CV_64FC3)
{
    cv::Mat dest(source.rows, source.cols, CV_64FC3); // this will be the delinearized image
    const double* sourceP = source.ptr<double>(0);
    double* destP = dest.ptr<double>(0);
    int total = source.cols * source.rows * 3;

    double q = 1.0 / 2.4;
    for (int n = 0; n < total; n++) {
        if (sourceP[n] > 0.0031308)
                destP[n] = 1.055 * pow(sourceP[n], q) - 0.055;
            else
                destP[n] = sourceP[n] * 12.92;
    }

    return dest;
}

///////////////////////////////////////////////////////////
//// Gradients
///////////////////////////////////////////////////////////

double GrayCurve(const int &color, const int &type, const int &begin, const int &range) // return a value transformed by a function, all values and results are in [0..1]
{
    // all functions must have continuous results in [0..1] range and have this shape : f(x) * range + begin

    double x = double(color - begin) / range; // x of f(x)

    if (range == 0) return color; // faster this way

    switch (type) {
        // good spread
        case curve_linear: return color; // linear -> the same color !
        // S-shaped
        case curve_cosinus2: return pow(cos((1.0 - x) * Pi / 2.0), 2.0) * range + begin; // cosinus² (better color gradient): f(x)=cos((1-x)*pi/2)²
        case curve_sigmoid: return 1.0 / (1.0 + exp(-5.0 * (2.0 * (x) - 1.0))) * range + begin; // sigmoid (S-shaped): f(x)=1/(1 + e(-5*(2x -1))
        // fast beginning
        case curve_cosinus: return cos((1.0 - x) * Pi / 2.0) * range + begin; // cosinus (more of end color): f(x)=cos((1-x)*pi/2)
        case curve_cos2sqrt: return pow(cos((1.0 - sqrt(x)) * Pi / 2.0), 2.0) * range + begin; // cos²sqrt: f(x)=cos((1−sqrt(x))*pi/2)²
        // fast ending
        case curve_power2: return pow(x, 2.0) * range + begin; // power2 (more of begin color): f(x)=x²
        case curve_cos2power2: return pow(cos((1.0 - pow(x, 2.0)) * Pi / 2.0), 2.0) * range + begin; // cos²power2: f(x)=cos((1−x²)*pi/2)²
        case curve_power3: return pow(x, 3.0) * range + begin; // power3 (even more of begin color): f(x)=x³
        // undulate
        case curve_undulate: return cos(double(color - begin) / 4.0 * Pi) * range + begin; // undulate: f(x)=cos(x/4*pi)
        case curve_undulate2: return cos(pow(double(color - begin) * Pi + 0.5, 2.0)) * range + begin; // undulate²: f(x)=cos((x*2*pi)²) / 2 + 0.5
        case curve_undulate3: return (cos(Pi * Pi * pow(x + 2.085, 2.0)) / (pow(x + 2.085, 3.0) + 8.0) + (x + 2.085) - 2.11) * range + begin; // undulate3: f(x) = cos(pi²∙(x+2.085)²) / ((x+2.085)³+10) + (x+2.085) − 2.11
    }

    return color;
}

double EuclideanDistanceRadius(cv::Point center, cv::Point point, int radius){ // return distance between 2 points
    double distance = sqrt(std::pow(center.x - point.x, 2) + std::pow(center.y - point.y, 2));

    if (distance > radius) return radius; // no value beyond radius
        else return distance;
}

void GradientFillGray(const int &gradient_type, cv::Mat &img, const cv::Mat &msk, const cv::Point &beginPoint,
                      const cv::Point &endPoint, const int &beginColor, const int &endColor,
                      const int &curve, cv::Rect area) // fill a 1-channel image with the mask converted to gray gradients
    // img must be 1-channel and msk 1-channel
    // img is input-output
    // area = rectangle containing the mask (speeds up the image scan) - no area given = entire image scanned
{
    if (area == cv::Rect(0, 0, 0, 0)) // default area = 0
        area = cv::Rect(0,0, img.cols, img.rows); // set it to image dimensions

    switch (gradient_type) {
        case (gradient_flat): { // flat = same color everywhere
            img.setTo(beginColor, msk); // fill the mask with this color
            return;
        }
        case (gradient_linear): { // grayscale is spread along the line
            int A = (endPoint.x - beginPoint.x); // horizontal difference
            int B = (endPoint.y - beginPoint.y); // vertical difference
            int C1 = A * beginPoint.x + B * beginPoint.y; // vectors
            int C2 = A * endPoint.x + B * endPoint.y;

            double CO; // will contain color values

            for (int row = area.y; row < area.y + area.height; row++) { // scan mask
                const uchar* mskP = msk.ptr<uchar>(row);
                uchar* imgP = img.ptr<uchar>(row);
                for (int col = area.x; col < area.x + area.width; col++)
                    if (mskP[col] != 0) { // non-zero pixel in mask
                        int C = A * col + B * row; // "distance" for this pixel

                        if (C <= C1) CO = beginColor; // before begin point : begin color
                            else if (C >= C2) CO = endColor; // after end point : end color
                                else CO = round(GrayCurve(double(beginColor * (C2 - C) + endColor * (C - C1))/(C2 - C1),
                                                    curve, beginColor, endColor - beginColor)); // C0 = percentage between begin and end colors, "shaped" by gray curve
                        imgP[col] = CO; // set pixel to image
                    }
            }
            return; // done ! -> exit
        }
        case (gradient_doubleLinear): { // double linear = 2 times linear, just invert the vector the second time
            int A = (endPoint.x - beginPoint.x); // same comments as linear
            int B = (endPoint.y - beginPoint.y);
            int C1 = A * beginPoint.x + B * beginPoint.y;
            int C2 = A * endPoint.x + B * endPoint.y;

            double CO;

            for (int row = area.y; row < area.y + area.height; row++) {
                const uchar* mskP = msk.ptr<uchar>(row);
                uchar* imgP = img.ptr<uchar>(row);
                for (int col = area.x; col < area.x + area.width; col++)
                    if (mskP[col] != 0) {
                        int C = A * col + B * row;
                        if (((C > C1) & (C < C2)) | (C >= C2) | (C == C1))  { // the only difference is we don't fill "before" the begin point
                            if (C == C1) CO = beginColor;
                                else if (C >= C2) CO = endColor;
                                    else CO = round(GrayCurve(double(beginColor * (C2 - C) + endColor * (C - C1))/(C2 - C1),
                                                        curve, beginColor, endColor - beginColor));
                            imgP[col] = CO;
                        }
                    }
            }

            cv::Point newEndPoint; // invert vector
            newEndPoint.x = 2 * beginPoint.x - endPoint.x;
            newEndPoint.y = 2 * beginPoint.y - endPoint.y;

            A = (newEndPoint.x - beginPoint.x); // same as before, but with new inverted vector
            B = (newEndPoint.y - beginPoint.y);
            C1 = A * beginPoint.x + B * beginPoint.y;
            C2 = A * newEndPoint.x + B * newEndPoint.y;

            for (int row = area.y; row < area.y + area.height; row++) {
                const uchar* mskP = msk.ptr<uchar>(row);
                uchar* imgP = img.ptr<uchar>(row);
                for (int col = area.x; col < area.x + area.width; col++)
                    if (mskP[col] != 0) {
                        int C = A * col + B * row;
                        if (((C > C1) & (C < C2)) | (C >= C2) | (C == C1)) { // once again don't fill "before" begin point
                            if (C == C1) CO = beginColor;
                                else if (C >= C2) CO = endColor;
                                    else CO = round(GrayCurve(double(beginColor * (C2 - C) + endColor * (C - C1))/(C2 - C1),
                                                        curve, beginColor, endColor - beginColor));
                            imgP[col] = CO;
                        }
                    }
            }
            return;
        }
        case (gradient_radial): { // radial = concentric circles
            double radius = std::sqrt(std::pow(beginPoint.x - endPoint.x, 2) + std::pow(beginPoint.y - endPoint.y, 2)); // maximum euclidian distance = vector length

            int CO; // will contain color values

            for (int row = area.y; row < area.y + area.height; row++) { // scan entire mask
                const uchar* mskP = msk.ptr<uchar>(row);
                uchar* imgP = img.ptr<uchar>(row);
                for (int col = area.x; col < area.x + area.width; col++)
                    if (mskP[col] != 0) { // non-zero pixel in mask
                        CO = round(GrayCurve(beginColor + EuclideanDistanceRadius(beginPoint, cv::Point(col, row), radius) / radius * (endColor - beginColor),
                                             curve, beginColor, endColor - beginColor)); // pixel in temp gradient mask = distance percentage, "shaped" by gray curve
                        imgP[col] = CO;
                    }
            }
            return;
        }
    }
}

void GradientFillColor(const int &gradient_type, cv::Mat &img, const cv::Mat &msk, const cv::Point &beginPoint,
                      const cv::Point &endPoint, const cv::Vec3b &beginColor, const cv::Vec3b &endColor,
                      const int &curve, cv::Rect area, const int &noise) // fill a 3-channel image with the mask converted to color gradients - use 'curve_cosinus2' as 'curve' to get the best smooth transition
    // img must be 3-channel and msk 1-channel
    // img is input-output
    // area = rectangle containing the mask (speeds up the image scan) - no area given = entire image scanned
    // noise is for prevent banding by adding noise : amount is random between -noise < 0 < noise
{
    cv::Vec3b beginColorLinear = RGBtoLinear(beginColor);
    cv::Vec3b endColorLinear = RGBtoLinear(endColor);

    if (area == cv::Rect(0, 0, 0, 0)) // default area = 0
        area = cv::Rect(0,0, img.cols, img.rows); // set it to image dimensions

    switch (gradient_type) {
        case (gradient_flat): { // flat = same color everywhere
            img.setTo(beginColorLinear, msk); // fill the mask with this color
            return;
        }
        case (gradient_linear): { // grayscale is spread along the line
            int A = (endPoint.x - beginPoint.x); // horizontal difference
            int B = (endPoint.y - beginPoint.y); // vertical difference
            int C1 = A * beginPoint.x + B * beginPoint.y; // vectors
            int C2 = A * endPoint.x + B * endPoint.y;

            cv::Vec3b CO; // will contain color values

            for (int row = area.y; row < area.y + area.height; row++) { // scan mask
                const uchar* mskP = msk.ptr<uchar>(row);
                cv::Vec3b* imgP = img.ptr<cv::Vec3b>(row);
                for (int col = area.x; col < area.x + area.width; col++)
                    if (mskP[col] != 0) { // non-zero pixel in mask
                        int C = A * col + B * row; // "distance" for this pixel

                        if (C <= C1)
                            CO = beginColorLinear; // before begin point : begin color
                        else if (C >= C2)
                            CO = endColorLinear; // after end point : end color
                        else {
                            for (int channel = 0; channel < 3; channel++) {
                                CO[channel] = GrayCurve(double(beginColorLinear[channel] * (C2 - C) + endColorLinear[channel] * (C - C1))/(C2 - C1),
                                                    curve, beginColorLinear[channel], endColorLinear[channel] - beginColorLinear[channel]); // C0 = percentage between begin and end colors, "shaped" by gray curve
                            }
                        }
                        imgP[col] = LinearToRGB(CO); // set pixel to image
                        if (noise > 0)
                            for (int channel = 0; channel < 3; channel++)
                                if (imgP[col][channel] != 0)
                                    imgP[col][channel] = GetByteInRange(imgP[col][channel] + Randomize<int>(0, noise) - noise / 2);
                    }
            }
            return; // done ! -> exit
        }
        case (gradient_doubleLinear): { // double linear = 2 times linear, just invert the vector the second time
            int A = (endPoint.x - beginPoint.x); // same comments as linear
            int B = (endPoint.y - beginPoint.y);
            int C1 = A * beginPoint.x + B * beginPoint.y;
            int C2 = A * endPoint.x + B * endPoint.y;

            cv::Vec3b CO;

            for (int row = area.y; row < area.y + area.height; row++) {
                const uchar* mskP = msk.ptr<uchar>(row);
                cv::Vec3b* imgP = img.ptr<cv::Vec3b>(row);
                for (int col = area.x; col < area.x + area.width; col++)
                    if (mskP[col] != 0) {
                        int C = A * col + B * row;
                        if (((C > C1) & (C < C2)) | (C >= C2) | (C == C1)) { // the only difference is we don't fill "before" the begin point
                            if (C == C1)
                                CO = beginColorLinear;
                            else if (C >= C2)
                                CO = endColorLinear;
                            else {
                                for (int channel = 0; channel < 3; channel++) {
                                    CO[channel] = GrayCurve(double(beginColorLinear[channel] * (C2 - C) + endColorLinear[channel] * (C - C1))/(C2 - C1),
                                                            curve, beginColorLinear[channel], endColorLinear[channel] - beginColorLinear[channel]);
                                }
                            }
                            imgP[col] = LinearToRGB(CO); // set pixel to image
                            if (noise > 0)
                                for (int channel = 0; channel < 3; channel++)
                                    if (imgP[col][channel] != 0)
                                        imgP[col][channel] = GetByteInRange(imgP[col][channel] + Randomize<int>(0, noise) - noise / 2);
                        }
                    }
            }

            cv::Point newEndPoint; // invert vector
            newEndPoint.x = 2 * beginPoint.x - endPoint.x;
            newEndPoint.y = 2 * beginPoint.y - endPoint.y;

            A = (newEndPoint.x - beginPoint.x); // same as before, but with new inverted vector
            B = (newEndPoint.y - beginPoint.y);
            C1 = A * beginPoint.x + B * beginPoint.y;
            C2 = A * newEndPoint.x + B * newEndPoint.y;

            for (int row = area.y; row < area.y + area.height; row++) {
                const uchar* mskP = msk.ptr<uchar>(row);
                cv::Vec3b* imgP = img.ptr<cv::Vec3b>(row);
                for (int col = area.x; col < area.x + area.width; col++)
                    if (mskP[col] != 0) {
                        int C = A * col + B * row;
                        if (((C > C1) & (C < C2)) | (C >= C2) | (C == C1)) { // once again don't fill "before" begin point
                            if (C == C1)
                                CO = beginColorLinear;
                            else if (C >= C2)
                                CO = endColorLinear;
                            else {
                                for (int channel = 0; channel < 3; channel++) {
                                    CO[channel] = GrayCurve(double(beginColorLinear[channel] * (C2 - C) + endColorLinear[channel] * (C - C1))/(C2 - C1),
                                                            curve, beginColorLinear[channel], endColorLinear[channel] - beginColorLinear[channel]);
                                }
                            }
                            imgP[col] = LinearToRGB(CO); // set pixel to image
                            if (noise > 0)
                                for (int channel = 0; channel < 3; channel++)
                                    if (imgP[col][channel] != 0)
                                        imgP[col][channel] = GetByteInRange(imgP[col][channel] + Randomize<int>(0, noise) - noise / 2);
                        }
                    }
            }
            return;
        }
        case (gradient_radial): { // radial = concentric circles
            float radius = std::sqrt(std::pow(beginPoint.x - endPoint.x, 2) + std::pow(beginPoint.y - endPoint.y, 2)); // maximum euclidian distance = vector length

            cv::Vec3b CO; // will contain color values

            for (int row = area.y; row < area.y + area.height; row++) { // scan entire mask
                const uchar* mskP = msk.ptr<uchar>(row);
                cv::Vec3b* imgP = img.ptr<cv::Vec3b>(row);
                for (int col = area.x; col < area.x + area.width; col++)
                    if (mskP[col] != 0) { // non-zero pixel in mask
                        for (int channel = 0; channel < 3; channel++) {
                            CO[channel] = GrayCurve(beginColorLinear[channel] + EuclideanDistanceRadius(beginPoint, cv::Point(col, row), radius) / radius * (endColorLinear[channel] - beginColorLinear[channel]),
                                                 curve, beginColorLinear[channel], endColorLinear[channel] - beginColorLinear[channel]); // pixel in temp gradient mask = distance percentage, "shaped" by gray curve
                        }
                        imgP[col] = LinearToRGB(CO); // set pixel to image
                        /*if (dither > 0)
                            for (int channel = 0; channel < 3; channel++)
                                if (imgP[col][channel] != 0)
                                    imgP[col][channel] = GetByteInRange(imgP[col][channel] + Randomize<int>(0, dither) - dither / 2);*/
                    }
            }
            return;
        }
    }
}

