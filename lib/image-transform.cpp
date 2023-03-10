/*#-------------------------------------------------
#
#     Image transforms library with OpenCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v2.0 - 2022/05/02
#
#   - Scaling methods :
#       * with aspect ratio
#       * double upscale or downscale
#       * better quality resizing than OpenCV
#   - Transforms :
#       * Mirror
#       * Warping from curves types
#       * Random warping on both axes
#       * Rotation
#       * Paste rotated alpha image on background
#
#-------------------------------------------------*/


#include "image-transform.h"


///////////////////////////////////////////////////////////
//// Transforms
///////////////////////////////////////////////////////////

cv::Mat MirrorImageColorAlphaVertical(const cv::Mat4b &source) // BGRA image mirror horizontal and/or vertical
{
    cv::Mat4b dst = cv::Mat(source.rows, source.cols, CV_8UC4);

    for (int j = 0; j < source.rows; j++) {
        const cv::Vec4b* imageP = source.ptr<cv::Vec4b>(source.rows - 1 - j);
        cv::Vec4b* dstP = dst.ptr<cv::Vec4b>(j);
        for (int i = 0; i < source.cols; i++) {
            dstP[i] = imageP[i];
        }
    }

    return dst;
}

cv::Mat MirrorImageColorAlphaHorizontal(const cv::Mat4b &source) // BGRA image mirror horizontal and/or vertical
{
    cv::Mat4b dst = cv::Mat(source.rows, source.cols, CV_8UC4);

    for (int j = 0; j < source.rows; j++) {
        const cv::Vec4b* imageP = source.ptr<cv::Vec4b>(j);
        cv::Vec4b* dstP = dst.ptr<cv::Vec4b>(j);
        for (int i = 0; i < source.cols / 2 + 1; i++) {
            dstP[i] = imageP[source.cols - 1 - i];
            dstP[source.cols - 1 - i] = imageP[i];
        }
    }

    return dst;
}

cv::Mat MirrorImageColorVertical(const cv::Mat3b &source) // BGR image mirror horizontal and/or vertical
{
    cv::Mat3b dst = cv::Mat(source.rows, source.cols, CV_8UC3);

    for (int j = 0; j < source.rows; j++) {
        const cv::Vec3b* imageP = source.ptr<cv::Vec3b>(source.rows - 1 - j);
        cv::Vec3b* dstP = dst.ptr<cv::Vec3b>(j);
        for (int i = 0; i < source.cols; i++)
                dstP[i] = imageP[i];
    }

    return dst;
}

cv::Mat MirrorImageColorHorizontal(const cv::Mat3b &source) // BGR image mirror horizontal and/or vertical
{
    cv::Mat3b dst = cv::Mat(source.rows, source.cols, CV_8UC3);

    for (int j = 0; j < source.rows; j++) {
        const cv::Vec3b* imageP = source.ptr<cv::Vec3b>(j);
        cv::Vec3b* dstP = dst.ptr<cv::Vec3b>(j);
        for (int i = 0; i < source.cols / 2 + 1; i++) {
                dstP[i] = imageP[source.cols - 1 - i];
                dstP[source.cols - 1 - i] = imageP[i];
        }
    }

    return dst;
}

cv::Mat MirrorImageGrayVertical(const cv::Mat1b &source) // gray image mirror vertical
{
    cv::Mat1b dst = cv::Mat(source.rows, source.cols, CV_8UC1);

    for (int j = 0; j < source.rows; j++) {
        const uchar* imageP = source.ptr<uchar>(source.rows - 1 - j);
        uchar* dstP = dst.ptr<uchar>(j);
        for (int i = 0; i < source.cols; i++)
                dstP[i] = imageP[i];
    }

    return dst;
}

cv::Mat MirrorImageGrayHorizontal(const cv::Mat1b &source) // gray image mirror horizontal
{
    cv::Mat1b dst = cv::Mat(source.rows, source.cols, CV_8UC1);

    for (int j = 0; j < source.rows; j++) {
        const uchar* imageP = source.ptr<uchar>(j);
        uchar* dstP = dst.ptr<uchar>(j);
        for (int i = 0; i < source.cols / 2 + 1; i++) {
            dstP[i] = imageP[source.cols - 1 - i];
            dstP[source.cols - 1 - i] = imageP[i];
        }
    }

    return dst;
}

cv::Mat MirrorImage(const cv::Mat &source, const bool &horizontal, const bool &vertical) // non-alpha or alpha image mirror horizontal and/or vertical
{
    cv::Mat dest;
    source.copyTo(dest);

    if ((!horizontal) and (!vertical))
        return dest;

    if (source.channels() == 1) {
        if (horizontal)
            dest = MirrorImageGrayHorizontal(dest);
        if (vertical)
            dest = MirrorImageGrayVertical(dest);
    }
    else if (source.channels() == 3) {
        if (horizontal)
            dest = MirrorImageColorHorizontal(dest);
        if (vertical)
            dest = MirrorImageColorVertical(dest);
    }
    else if (source.channels() == 4) {
        if (horizontal)
            dest = MirrorImageColorAlphaHorizontal(dest);
        if (vertical)
            dest = MirrorImageColorAlphaVertical(dest);
    }

    return dest;
}

double WarpCurve(const int &type, const double &value, const double &rangeX, const double &rangeY, const double &pixels) // "warp" a value using mathematical functions and ranges
    // value is hopefuly in rangeX or rangeY
    // rangeX and rangeY are used to get a percentage, value in [0..1] : used functions all give results in [0..1] then this result is multiplied by "pixels"
{
    switch (type) {
        case warp_undulate: return pixels * sin(value * 2.0 * Pi / 180.0); // undulate
        case warp_undulate_decreasing: return pixels * cos(pow(value / rangeY / 2.0 * Pi * Pi, 2.0)); // undulate dcreasing
        case warp_comma_inflated: return pixels * pow(value / rangeY, 3.0); // comma-shaped inflated in the middle
        case warp_comma_deflated: return pixels * log(1.0 + value / rangeY * (exp(1.0) - 1.0)); // comma-shaped deflated
        case warp_curve_inflated: return pixels * pow((value / rangeY) * 2.0 - 1.0, 2.0); // comma-shaped but inflated in the middle
        case warp_curve_inflated_fat: return pixels * pow((value / rangeY) * 2.0 - 1.0, 4.0); // round-shaped but very inflated in the middle
        case warp_curve_deflated: return pixels * sin(2.0 * Pi * value / (2.0 * rangeX)); // round-shaped deflated in the middle
        case warp_curve_apostrophe: return pixels * pow(value / rangeY, 1.0 / 4.0); // apostrophe-shaped deflated in the middle
        case warp_s: return pixels * pow((value / rangeY) * 2.0 - 1.0, 3.0); // S-shaped - with x and y not having the same sign, the result is a quasi-ellipse !
    }

    return 0; // if nothing else was found => image unchanged
}

cv::Mat WarpImageColorAlphaHorizontal(const cv::Mat4b &source, const int &typeH, const int &pixelsX) // warp BGR source on horizontal axis - pixelsX pixels
{
    cv::Mat4b dst = cv::Mat::zeros(source.rows, source.cols, CV_8UC4);
    int offsetX;

    for (int j = 0; j < source.rows; j++) {
        const cv::Vec4b* sourceP = source.ptr<cv::Vec4b>(j);
        cv::Vec4b* dstP = dst.ptr<cv::Vec4b>(j);
        for (int i = 0; i < source.cols; i++) {
            offsetX = 0;
            offsetX = WarpCurve(typeH, j, source.cols, source.rows, pixelsX);
            int x = (i + offsetX);

            if ((x < source.cols) and (x >= 0)) {
                dstP[i] = sourceP[x];
            }
        }
    }

    return dst;
}

cv::Mat WarpImageColorAlphaVertical(const cv::Mat4b &source, const int &typeV, const int &pixelsY) // warp gray source on vertical axis - pixelsY in pixels
{
    cv::Mat4b dst = cv::Mat::zeros(source.rows, source.cols, CV_8UC4);
    int offsetY;

    for (int j = 0; j < source.rows; j++) {
        cv::Vec4b* dstP = dst.ptr<cv::Vec4b>(j);
        for (int i = 0; i < source.cols; i++) {
            offsetY = 0;
            offsetY = WarpCurve(typeV, i, source.rows, source.cols, pixelsY);
            int y = (j + offsetY);

            if ((y < source.rows) and (y >= 0)) {
                dstP[i] = source.at<cv::Vec4b>(y, i);
            }
        }
    }

    return dst;
}

cv::Mat WarpImageColorAlpha(const cv::Mat4b &source, const bool &horizontal, const int &typeH, const int &pixelsX, const bool &vertical, const int &typeV, const int &pixelsY) // warp gray source on horizontal and / or vertical axis - decX and decY in pixels
{
    cv::Mat res;

    if (horizontal)
        res = WarpImageColorAlphaHorizontal(source, typeH, pixelsX);

    if (vertical) {
        if (res.empty())
            res = WarpImageColorAlphaVertical(source, typeV, pixelsY);
        else
            res = WarpImageColorAlphaVertical(res, typeV, pixelsY);
    }

    if (res.empty())
        source.copyTo(res);

    return res;
}

cv::Mat WarpImageColorHorizontal(const cv::Mat3b &source, const int &typeH, const int &pixelsX) // warp BGR source on horizontal axis - pixelsX pixels
{
    cv::Mat3b dst = cv::Mat::zeros(source.rows, source.cols, CV_8UC3);
    int offsetX;

    for (int j = 0; j < source.rows; j++) {
        const cv::Vec3b* sourceP = source.ptr<cv::Vec3b>(j);
        cv::Vec3b* dstP = dst.ptr<cv::Vec3b>(j);
        for (int i = 0; i < source.cols; i++) {
            offsetX = 0;
            offsetX = WarpCurve(typeH, j, source.cols, source.rows, pixelsX);
            int x = (i + offsetX);

            if ((x < source.cols) and (x >= 0)) {
                dstP[i] = sourceP[x];
            }
        }
    }

    return dst;
}

cv::Mat WarpImageColorVertical(const cv::Mat3b &source, const int &typeV, const int &pixelsY) // warp gray source on vertical axis - pixelsY in pixels
{
    cv::Mat3b dst = cv::Mat::zeros(source.rows, source.cols, CV_8UC3);
    int offsetY;

    for (int j = 0; j < source.rows; j++) {
        cv::Vec3b* dstP = dst.ptr<cv::Vec3b>(j);
        for (int i = 0; i < source.cols; i++) {
            offsetY = 0;
            offsetY = WarpCurve(typeV, i, source.rows, source.cols, pixelsY);
            int y = (j + offsetY);

            if ((y < source.rows) and (y >= 0)) {
                dstP[i] = source.at<cv::Vec3b>(y, i);
            }
        }
    }

    return dst;
}

cv::Mat WarpImageColor(const cv::Mat3b &source, const bool &horizontal, const int &typeH, const int &pixelsX, const bool &vertical, const int &typeV, const int &pixelsY) // warp gray source on horizontal and / or vertical axis - decX and decY in pixels
{
    cv::Mat res;

    if (horizontal)
        res = WarpImageColorHorizontal(source, typeH, pixelsX);

    if (vertical) {
        if (res.empty())
            res = WarpImageColorVertical(source, typeV, pixelsY);
        else
            res = WarpImageColorVertical(res, typeV, pixelsY);
    }

    if (res.empty())
        source.copyTo(res);

    return res;
}

cv::Mat WarpImageGrayHorizontal(const cv::Mat1b &source, const int &typeH, const int &pixelsX) // warp gray source on horizontal axis - pixelsX pixels
{
    cv::Mat1b dst = cv::Mat::zeros(source.rows, source.cols, CV_8UC1);
    int offsetX;

    for (int j = 0; j < source.rows; j++) {
        const uchar* sourceP = source.ptr<uchar>(j);
        uchar* dstP = dst.ptr<uchar>(j);
        for (int i = 0; i < source.cols; i++) {
            offsetX = 0;
            offsetX = WarpCurve(typeH, j, source.cols, source.rows, pixelsX);
            int x = (i + offsetX);

            if ((x < source.cols) and (x >= 0)) {
                dstP[i] = sourceP[x];
            }
        }
    }

    return dst;
}

cv::Mat WarpImageGrayVertical(const cv::Mat1b &source, const int &typeV, const int &pixelsY) // warp gray source on vertical axis - pixelsY in pixels
{
    cv::Mat1b dst = cv::Mat::zeros(source.rows, source.cols, CV_8UC1);
    int offsetY;

    for (int j = 0; j < source.rows; j++) {
        uchar* dstP = dst.ptr<uchar>(j);
        for (int i = 0; i < source.cols; i++) {
            offsetY = 0;
            offsetY = WarpCurve(typeV, i, source.rows, source.cols, pixelsY);
            int y = (j + offsetY);

            if ((y < source.rows) and (y >= 0)) {
                dstP[i] = source.at<uchar>(y, i);
            }
        }
    }

    return dst;
}

cv::Mat WarpImageGray(const cv::Mat1b &source, const bool &horizontal, const int &typeH, const int &pixelsX, const bool &vertical, const int &typeV, const int &pixelsY) // warp gray source on horizontal and / or vertical axis - decX and decY in pixels
{
    cv::Mat res;

    if (horizontal)
        res = WarpImageGrayHorizontal(source, typeH, pixelsX);

    if (vertical) {
        if (res.empty())
            res = WarpImageGrayVertical(source, typeV, pixelsY);
        else
            res = WarpImageGrayVertical(res, typeV, pixelsY);
    }

    if (res.empty())
        source.copyTo(res);

    return res;
}

cv::Mat WarpImage(const cv::Mat &source, const bool &horizontal, const int &typeH, const int &pixelsX, const bool &vertical, const int &typeV, const int &pixelsY) // warp image on horizontal and / or vertical axis - decX and decY in pixels
{
    cv::Mat res;

    if (source.channels() == 1)
        res = WarpImageGray(source, horizontal, typeH, pixelsX, vertical, typeV, pixelsY);
    else if (source.channels() == 3)
        res = WarpImageColor(source, horizontal, typeH, pixelsX, vertical, typeV, pixelsY);
    else if (source.channels() == 4)
        res = WarpImageColorAlpha(source, horizontal, typeH, pixelsX, vertical, typeV, pixelsY);

    if (res.empty())
        source.copyTo(res);

    return res;
}

cv::Mat RotateImage(const cv::Mat &source, const double &angleRad, const int &centerX, const int &centerY, int &dX, int &dY) // rotate alpha or non-alpha source by angle in rads
{
    double cosAngle = cos(angleRad); // pre-compute cos and sin
    double sinAngle = sin(angleRad);
    double cX =  round( centerX * cosAngle + centerY * sinAngle); // rotated center
    double cY = round(-centerX * sinAngle + centerY * cosAngle);

    cv::Point vertices[4]; // 4 corners of bounding rectangle
    vertices[0] = cv::Point(              0, 0);
    vertices[1] = cv::Point(source.cols - 1, 0);
    vertices[2] = cv::Point(source.cols - 1, source.rows - 1);
    vertices[3] = cv::Point(              0, source.rows - 1);

    double xMin = 0;
    double yMin = 0;
    for (int i = 0; i < 4; i++) { // for each corner
        int x =   round(vertices[i].x * cosAngle + vertices[i].y * sinAngle); // compute rotated corner
        int y =  round(-vertices[i].x * sinAngle + vertices[i].y * cosAngle);
        vertices[i] = cv::Point(x, y);
        if (x < xMin) // find negative coordinates if any, this will be the shift for the image
            xMin = x;
        if (y < yMin)
            yMin = y;
    }

    double angleDeg = RadToDeg(angleRad); // some functions need the angle in degrees !
    cv::Point2f center(source.cols / 2.0, source.rows / 2.0); // center of image
    cv::Mat rot = cv::getRotationMatrix2D(center, angleDeg, 1.0); // get affine rotation matrix from angle and center
    cv::Rect2f bbox = cv::RotatedRect(center, source.size(), angleDeg).boundingRect2f(); // determine rotated bounding rectangle, center not relevant
    // adjust transformation matrix
    rot.at<double>(0, 2) += (bbox.width - source.cols) / 2.0;
    rot.at<double>(1, 2) += (bbox.height - source.rows) / 2.0;

    cv::Mat dst;
    cv::warpAffine(source, dst, rot, bbox.size(), cv::INTER_CUBIC); // rotation of image

    dX = round(-cX + xMin); // shift needed for pasting the rotated image
    dY = round(-cY + yMin);

    return dst; // return rotated image
}

void PasteImageAlphaRotated(cv::Mat &background, const cv::Mat &foreground, const double &angleRad, const int &destX, const int &destY, const int &centerX, const int &centerY) // paste foreground with alpha to background without alpha
{
    int dX = centerX; // shift for pasting rotated image
    int dY = centerY;
    cv::Mat dst = RotateImage(foreground, angleRad, destX, destY, dX, dY); // get rotated image + shift

    PasteImageAlpha(background, dst, cv::Point(destX + dX, destY + dY)); // paste image on background
}

///////////////////////////////////////////////////////////
//// Scaling
///////////////////////////////////////////////////////////

double Attract(const double &x, const double &y, const double &z, const double &t)
{
    double sqrtX = -2.0 * sqrt(x);
    double sqrtY = 10.0 * sqrt(y);
    double sqrtZ = 10.0 * sqrt(z);
    double sqrtT = -2.0 * sqrt(t);
    return (sqrtX + sqrtY + sqrtZ + sqrtT) * (sqrtX + sqrtY + sqrtZ + sqrtT) / 256.0;
}

cv::Mat QualityUpCcaleImageDCCI(const cv::Mat &source) // resize image to double the size of original using DCCI algorithm
    // see https://en.wikipedia.org/wiki/Directional_Cubic_Convolution_Interpolation
{
    int ws = 4; //window size
    double threshold = 1.15;
    int k = 5;
    cv::Mat X_mirrors;
    cv::Mat R_mirrors;

    //this is for the pixels at border
    cv::copyMakeBorder(source, X_mirrors, ws - 1, ws - 1, ws - 1, ws - 1, cv::BORDER_REFLECT);
    cv::Mat result = cv::Mat::zeros(2 * source.rows, 2 * source.cols, CV_8UC3);

    //remember to correct offset because you extend the border before
    for (int row = ws - 1; row < X_mirrors.rows - ws + 1; row++) {
        cv::Vec3b* X_mirrorsP = X_mirrors.ptr<cv::Vec3b>(row);
        cv::Vec3b* resultP = result.ptr<cv::Vec3b>(2 * (row - ws + 1));
        int max = X_mirrors.cols - ws + 1;
        for (int col = ws - 1; col < max; col++)
            resultP[2 * (col - ws + 1)] = X_mirrorsP[col];
    }

    //fill white square
    for (int i = ws - 1; i < X_mirrors.rows - ws + 1; i++) {
        cv::Vec3b* resultP = result.ptr<cv::Vec3b>(2 * (i - ws + 1) + 1);
        cv::Vec3b* X_mirrorsP = X_mirrors.ptr<cv::Vec3b>(i);
        int max = X_mirrors.cols - ws + 1;
        for (int j = ws - 1; j < max; j++) {
            //int ii = 2 * (i - ws + 1) + 1;
            int jj = 2 * (j - ws + 1) + 1;

            cv::Vec3b value;

            for (int col = 0; col < 3; col++) {
                double G1 = 0;
                double G2 = 0;
                //cv::Vec3b* X_mirrorsP = X_mirrors.ptr<cv::Vec3b>(i + x);
                for (int x = -1; x < 2; x++) {
                    for (int y = -1; y < 2; y++) {
                        int val = j + y + x * X_mirrors.cols;
                        //G1:
                        cv::Vec3b pix11 = X_mirrorsP[val + X_mirrors.cols];
                        cv::Vec3b pix12 = X_mirrorsP[val + 1];
                        G1 += abs(double(pix11[col]) - double(pix12[col]));
                        //G2:
                        cv::Vec3b pix21 = X_mirrorsP[val];
                        cv::Vec3b pix22 = X_mirrorsP[val + 1 + X_mirrors.cols];
                        G2 += abs(double(pix21[col]) - double(pix22[col]));
                    }
                }

                cv::Vec3b pix00 = X_mirrorsP[j - 1 - X_mirrors.cols];
                cv::Vec3b pix11 = X_mirrorsP[j];
                cv::Vec3b pix22 = X_mirrorsP[j + 1 + X_mirrors.cols];
                cv::Vec3b pix33 = X_mirrorsP[j + 2 + X_mirrors.cols + X_mirrors.cols];
                cv::Vec3b pix30 = X_mirrorsP[j - 1 + X_mirrors.cols + X_mirrors.cols];
                cv::Vec3b pix21 = X_mirrorsP[j + X_mirrors.cols];
                cv::Vec3b pix12 = X_mirrorsP[j + 1];
                cv::Vec3b pix03 = X_mirrorsP[j + 2 - X_mirrors.cols];

                if (100 * (1 + G1) > 100 * threshold*(1 + G2)) {
                    double val = Attract(pix00[col], pix11[col], pix22[col], pix33[col]);
                    if (val < 0)
                        val = 0;
                    if (val > 255)
                        val = 255;
                    value[col] = val;
                }
                else if (100 * (1 + G2) > 100 * threshold*(1 + G1)) {
                    double val = Attract(pix30[col], pix21[col], pix12[col], pix03[col]);
                    if (val < 0)
                        val = 0;
                    if (val > 255)
                        val = 255;
                    value[col] = val;
                }
                else
                {
                    double w1 = 1 / (1 + pow(G1, k));
                    double w2 = 1 / (1 + pow(G2, k));
                    double weight1 = w1 / (w1 + w2);
                    double weight2 = w2 / (w1 + w2);
                    double p1 = Attract(pix00[col], pix11[col], pix22[col], pix33[col]);
                    double p2 = Attract(pix30[col], pix21[col], pix12[col], pix03[col]);
                    double val = weight1 * p1 + weight2 * p2;
                    if (val < 0)
                        val = 0;
                    if (val > 255)
                        val = 255;
                    value[col] = val;
                }
            }

            resultP[jj] = value;
        }
    }

    cv::copyMakeBorder(result, R_mirrors, 4, 4, 4, 4, cv::BORDER_REFLECT);

    for (int i = ws - 1; i < X_mirrors.rows - ws + 1; i++) {
        cv::Vec3b* resultP = result.ptr<cv::Vec3b>(2 * (i - ws + 1));
        int max = X_mirrors.cols - ws + 1;
        for (int j = ws - 1; j < max; j++) {
            //grey circle
            int ii1 = 2 * (i - ws + 1) + 1;
            int jj1 = 2 * (j - ws + 1) + 0;
            //white circle
            int ii2 = 2 * (i - ws + 1) + 0;
            int jj2 = 2 * (j - ws + 1) + 1;


            cv::Vec3b value1;
            cv::Vec3b value2;

            //fill grey circle
            cv::Vec3b* R_mirrorsP = R_mirrors.ptr<cv::Vec3b>(ii1 + 4);
            for (int col = 0; col < 3; col++) {
                int y = jj1 + 4;

                cv::Vec3b square_left1   = R_mirrorsP[y - 2 - R_mirrors.cols];
                cv::Vec3b square_left2   = R_mirrorsP[y - 2 + R_mirrors.cols];
                cv::Vec3b circle_left1   = R_mirrorsP[y - 1 - 2 * R_mirrors.cols];
                cv::Vec3b circle_left2   = R_mirrorsP[y - 1];
                cv::Vec3b circle_left3   = R_mirrorsP[y - 1 + 2 * R_mirrors.cols];
                cv::Vec3b square_middle1 = R_mirrorsP[y - R_mirrors.cols   ];
                cv::Vec3b square_middle2 = R_mirrorsP[y + R_mirrors.cols   ];
                cv::Vec3b circle_right1  = R_mirrorsP[y + 1 - 2 * R_mirrors.cols];
                cv::Vec3b circle_right2  = R_mirrorsP[y + 1];
                cv::Vec3b circle_right3  = R_mirrorsP[y + 1 + 2 * R_mirrors.cols];
                cv::Vec3b square_right1  = R_mirrorsP[y + 2 - R_mirrors.cols];
                cv::Vec3b square_right2  = R_mirrorsP[y + 2 + R_mirrors.cols];
                cv::Vec3b more1          = R_mirrorsP[y - 3 * R_mirrors.cols   ];
                cv::Vec3b more2          = R_mirrorsP[y + 3 * R_mirrors.cols   ];
                cv::Vec3b more3          = R_mirrorsP[y + 3];
                cv::Vec3b more4          = R_mirrorsP[y - 3];

                double G1 = abs(square_middle1[col] - square_left1[col])   +
                            abs(square_right1[col]  - square_middle1[col]) +
                            abs(square_middle2[col] - square_left2[col])   +
                            abs(square_right2[col]  - square_middle2[col]) +
                            abs(more3[col]          - circle_right2[col])  +
                            abs(circle_left2[col]   - more4[col])          +
                            abs(circle_right1[col]  - circle_left1[col])   +
                            abs(circle_right2[col]  - circle_left2[col])   +
                            abs(circle_right3[col]  - circle_left3[col]);

                double G2 = abs(square_left1[col]   - square_left2[col])   +
                            abs(square_middle1[col] - square_middle2[col]) +
                            abs(square_right2[col]  - square_right1[col])  +
                            abs(more1[col]          - square_middle1[col]) +
                            abs(square_middle2[col] - more2[col])          +
                            abs(circle_left1[col]   - circle_left2[col])   +
                            abs(circle_left2[col]   - circle_left3[col])   +
                            abs(circle_right1[col]  - circle_right2[col])  +
                            abs(circle_right2[col]  - circle_right3[col]);

                if (100 * (1 + G1) > 100 * threshold * (1 + G2)) {
                    double val = Attract(more1[col], square_middle1[col], square_middle2[col], more2[col]);
                    if (val < 0)
                        val = 0;
                    if (val > 255.0)
                        val = 255;
                    value1[col] = val;
                }

                else if (100 * (1 + G2) > 100 * threshold * (1 + G1)) {
                    double val = Attract(more4[col], circle_left2[col], circle_right2[col], more3[col]);
                    if (val < 0)
                        val = 0;
                    if (val > 255)
                        val = 255;
                    value1[col] = val;
                }
                else
                {
                    double w1 = 1 / (1 + pow(G1, k));
                    double w2 = 1 / (1 + pow(G2, k));
                    double weight1 = w1 / (w1 + w2);
                    double weight2 = w2 / (w1 + w2);
                    double p1 = Attract(more1[col], square_middle1[col], square_middle2[col], more2[col]);
                    double p2 = Attract(more4[col], circle_left2[col], circle_right2[col], more3[col]);
                    double val = weight1 * p1 + weight2 * p2;
                    if (val < 0)
                        val = 0;
                    if (val > 255)
                        val = 255;
                    value1[col] = val;
                }
            }

            //fill white circle
            cv::Vec3b* R_mirrorsP2 = R_mirrors.ptr<cv::Vec3b>(ii2 + 4);
            for (int col = 0; col < 3; col++) {
                //int x = ii2 + 4;
                int y = jj2 + 4;

                cv::Vec3b circle_left1   = R_mirrorsP2[y - 2 - R_mirrors.cols];
                cv::Vec3b circle_left2   = R_mirrorsP2[y - 2 + R_mirrors.cols];
                cv::Vec3b square_left1   = R_mirrorsP2[y - 1 - 2 * R_mirrors.cols];
                cv::Vec3b square_left2   = R_mirrorsP2[y - 1];
                cv::Vec3b square_left3   = R_mirrorsP2[y - 1 + 2 * R_mirrors.cols];
                cv::Vec3b circle_middle1 = R_mirrorsP2[y - 0 - R_mirrors.cols];
                cv::Vec3b circle_middle2 = R_mirrorsP2[y - 0 + R_mirrors.cols];
                cv::Vec3b square_right1  = R_mirrorsP2[y + 1 - 2 * R_mirrors.cols];
                cv::Vec3b square_right2  = R_mirrorsP2[y + 1];
                cv::Vec3b square_right3  = R_mirrorsP2[y + 1 + 2 * R_mirrors.cols];
                cv::Vec3b circle_right1  = R_mirrorsP2[y + 2 - R_mirrors.cols];
                cv::Vec3b circle_right2  = R_mirrorsP2[y + 2 + R_mirrors.cols];
                cv::Vec3b more1          = R_mirrorsP2[y - 3 * R_mirrors.cols   ];
                cv::Vec3b more2          = R_mirrorsP2[y + 3 * R_mirrors.cols   ];
                cv::Vec3b more3          = R_mirrorsP2[y + 3];
                cv::Vec3b more4          = R_mirrorsP2[y - 3];

                double G1 = abs(circle_middle1[col] - circle_left1[col])   +
                            abs(circle_right1[col]  - circle_middle1[col]) +
                            abs(circle_middle2[col] - circle_left2[col])   +
                            abs(circle_right2[col]  - circle_middle2[col]) +
                            abs(more3[col]          - square_right2[col])  +
                            abs(square_left2[col]   - more4[col])          +
                            abs(square_right1[col]  - square_left1[col])   +
                            abs(square_right2[col]  - square_left2[col])   +
                            abs(square_right3[col]  - square_left3[col]);

                double G2 = abs(circle_left1[col]   - circle_left2[col])   +
                            abs(circle_middle1[col] - circle_middle2[col]) +
                            abs(circle_right2[col]  - circle_right1[col])  +
                            abs(more1[col]          - circle_middle1[col]) +
                            abs(circle_middle2[col] - more2[col])          +
                            abs(square_left1[col]   - square_left2[col])   +
                            abs(square_left2[col]   - square_left3[col])   +
                            abs(square_right1[col]  - square_right2[col])  +
                            abs(square_right2[col]  - square_right3[col]);

                if (1 + G1 > threshold * (1 + G2)) {
                    double val = Attract(more1[col], circle_middle1[col], circle_middle2[col], more2[col]);
                    if (val < 0)
                        val = 0;
                    if (val > 255)
                        val = 255;
                    value2[col] = val;
                }

                else if (1 + G2 > threshold * (1 + G1)) {
                    double val = Attract(more4[col], square_left2[col], square_right2[col], more3[col]);
                    if (val < 0)
                        val = 0;
                    if (val > 255)
                        val = 255;
                    value2[col] = val;
                }
                else
                {
                    double w1 = 1 / (1 + pow(G1, k));
                    double w2 = 1 / (1 + pow(G2, k));
                    double weight1 = w1 / (w1 + w2);
                    double weight2 = w2 / (w1 + w2);
                    double p1 = Attract(more1[col], circle_middle1[col], circle_middle2[col], more2[col]);
                    double p2 = Attract(more4[col], square_left2[col], square_right2[col], more3[col]);
                    double val = weight1 * p1 + weight2 * p2;
                    if (val < 0)
                        val = 0;
                    if (val > 255)
                        val = 255;
                    value2[col] = val;
                }
            }

            resultP[jj1 + result.cols] = value1;
            resultP[jj2] = value2;
        }
    }

    return result;
}

cv::Mat QualityDownScaleImage(const cv::Mat &source, const double &initialRatio) // good quality image downscaling algorithm, better than direct downscaling
    // image is downscaled several times by 2 (bilinear interpolation) then final resizing is done using bicubic interpolation
    // see http://gudok.xyz/thumbnail
{
    cv::Mat dest;
    source.copyTo(dest);
    double ratio = initialRatio; // ratio will change during several passes

    if (ratio >= 1.0) // nothing to do, return source image as is
        return dest;

    while (ratio < 0.5) { // can the image be downscaled by 2 - do it as much as needed
        cv::resize(dest, dest, cv::Size(dest.cols / 2.0, dest.rows / 2.0), 0, 0, cv::INTER_AREA); // INTER_AREA is moire-free for downscale
        ratio = ratio * 2.0; // update ratio
    }

    if (dest.cols * ratio != dest.cols) { // final step : downscale using AREA interpolation if needed
        cv::resize(dest, dest, cv::Size(dest.cols * ratio, dest.rows * ratio), 0, 0, cv::INTER_AREA);
    }

    return dest;
}

cv::Mat QualityUpScaleImage(const cv::Mat &source, const double &initialRatio) // good quality upscaling algorithm, better than direct upscaling
    // image is downscaled several times by 2 (bilinear interpolation) then final resizing is done using bicubic interpolation
{
    cv::Mat img;
    source.copyTo(img);
    double ratio = initialRatio;

    if (ratio <= 1.0)
        return img;

    while (ratio > 2) {
        img = QualityUpCcaleImageDCCI(img);
        ratio = ratio / 2.0;
    }

    if (img.cols * ratio != img.cols) {
        cv::resize(img, img, cv::Size(img.cols * ratio, img.rows * ratio), 0, 0, cv::INTER_CUBIC);
    }

    return img;
}

cv::Mat QualityResizeImageAspectRatio(const cv::Mat &source, const cv::Size &frame) // Resize alpha or non-alpha image keeping aspect ratio
    // if frame width or height is 0 resize by the other value
{
    // find best ratio
    double zoom;

    if (frame.width == 0) // no width defined -> resize from height
        zoom = double(frame.height) / source.rows;
    else if (frame.height == 0) // no height defined -> resize from width
        zoom = double(frame.width) / source.cols;
    else { // full frame defined
        double zoomX = double(frame.width) / source.cols; // try vertical and horizontal ratios
        double zoomY = double(frame.height) / source.rows;

        if (zoomX < zoomY)
            zoom = zoomX; // the lowest fit the view
        else
            zoom = zoomY;
    }

    // resize image
    cv::Mat dest;

    if (zoom == 1) {
        source.copyTo(dest);
        return dest;
    }
    else if (zoom < 1)
        //method = cv::INTER_AREA;
        dest = QualityDownScaleImage(source, zoom);
    else
        dest = QualityUpScaleImage(source, zoom);

    return dest;
}

cv::Mat ResizeImageAspectRatio(const cv::Mat &source, const cv::Size &frame) // Resize image keeping aspect ratio, fast with bilinear interpolation
    // if frame width or height is 0 resize by the other value
{
    // find best ratio
    double zoom;

    if (frame.width == 0) // no width defined -> resize from height
        zoom = double(frame.height) / source.rows;
    else if (frame.height == 0) // no height defined -> resize from width
        zoom = double(frame.width) / source.cols;
    else { // full frame defined
        double zoomX = double(frame.width) / source.cols; // try vertical and horizontal ratios
        double zoomY = double(frame.height) / source.rows;

        if (zoomX < zoomY)
            zoom = zoomX; // the lowest fit the view
        else
            zoom = zoomY;
    }

    // resize image
    cv::Mat dest;
    cv::resize(source, dest, cv::Size(round(source.cols * zoom), round(source.rows * zoom)), 0, 0, cv::INTER_AREA); // resize

    return dest;
}

double GetScaleToResize(const int &sourceWidth, const int &sourceHeight, const int &destination_size) // compute scale for an image to be resized to destination_size pixels
{
    double zoomX = double(destination_size) / double(sourceWidth);
    double zoomY = double(destination_size) / double(sourceHeight);

    if (zoomX < zoomY)
        return zoomX;
    else
        return zoomY;
}
