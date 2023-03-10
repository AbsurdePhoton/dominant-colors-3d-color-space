/*#-------------------------------------------------
#
#        Image utils library with OpenCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v2.0 - 2022/12/12
#
#   - Utilities :
#       * Range values utils for int and uchar
#       * Count number of colors (RGB unique values)
#       * Normalize to and from 1.0 <--> 255
#       * cv::Mat equality (not obvious !)
#       * Is the image a solid color block ?
#   - Conversions OpenCV <--> Qt (QPixmap and QImage)
#   - Copy and paste (also with alpha)
#   - Alpha images :
#       * add alpha channel
#       * load alpha image to BGR + transparency grid
#       * split channels with adding alpha
#   - Image analysis, get :
#       * Sobel gradients
#       * Gray histogram (option with mask)
#       * Color mean using histograms
#       * Color palette
#       * Reflectance and Illumination
#   - Save PNG image with transparency
#
#-------------------------------------------------*/

#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H


#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"

#include <QImage>
#include <QPixmap>


//// General
int GetByteInRange(const int &byte); // get a byte in range [0..255]

template <typename anyValue> bool IsValueInRange(anyValue param, anyValue min, anyValue max)
{
    return (param >= min) and (param <= max);
}

template <typename anyValue> anyValue GetValueInRange(anyValue param, anyValue min, anyValue max)
{
    if (param < min)
        return min;
    else if (param > max)
        return max;
    else
        return param;
}

//// Utils
int CountRGBUniqueValues(const cv::Mat &source); // count number of RGB colors in image
cv::Mat NormalizeImage(const cv::Mat & source); // normalize [0..255] image to [0..1] - returns a CV_64F image
cv::Mat DeNormalizeImage(const cv::Mat & source); // denormalize [0..1] image to [0..255] - needs a CV_64F input
bool MatEqual(const cv::Mat &one, const cv::Mat &two); // compare two Mat
bool IsImageSolidColor(const cv::Mat &source); // tells if BGR image consists of a unique color

//// Conversions of cv::Mat to and from Qt images
cv::Mat QImage2Mat(const QImage &source); // convert QImage to Mat
cv::Mat QPixmap2Mat(const QPixmap &source); // convert QPixmap to Mat
QImage  Mat2QImage(const cv::Mat &source); // convert Mat to QImage
QPixmap Mat2QPixmap(const cv::Mat &source); // convert Mat to QPixmap
QPixmap Mat2QPixmapResized(const cv::Mat &source, const int &width, const int &height, const bool &smooth); // convert Mat to resized QPixmap
QImage  cvMatToQImage(const cv::Mat &source); // another implementation Mat type wise

//// Copy and paste
cv::Mat CopyFromImage(const cv::Mat &source, const cv::Rect &frame); // copy part of image - if parts of frame are outside source image, non-copied pixels are black
void PasteImageColor(cv::Mat &destination, const cv::Mat &source, const int &originX, const int &originY, const bool transparency=true, const cv::Vec3b transparentColor=cv::Vec3b(0,0,0)); // copy non-alpha image onto another
void PasteImageGray(cv::Mat &background, const cv::Mat &foreground, const int &originX, const int &originY, const bool transparency=true, const uchar transparentColor=0); // copy gray image onto another
void PasteImageAlpha(cv::Mat &background, const cv::Mat &foreground, const cv::Point pos=cv::Point(0, 0)); // paste alpha BGR image on non-alpha background
void PasteImagePlusAlpha(cv::Mat &background, const cv::Mat &foreground, const cv::Mat &alpha, const cv::Point pos=cv::Point(0, 0)); // paste (gray or color) + alpha image on non-alpha BGR background
void PasteImageAlphaOnAlpha(cv::Mat &background, const cv::Mat &foreground, const cv::Point pos); // paste alpha BGR image on alpha BGR background

//// Alpha channel
cv::Mat AddAlphaToImage(const cv::Mat &source, const cv::Mat &mask); // add alpha channel to BGR image - if mask is empty, transparency = black (0,0,0)
cv::Mat ImageAlphaWithGrid(const cv::Mat &source, const int &interval=32, const cv::Vec3b color1=cv::Vec3b(192,192,192), const cv::Vec3b color2=cv::Vec3b(127,127,127)); // get BGR image from BGRA with gray blocks where there is transparency
void ImageToBGRplusAlpha(const cv::Mat &source, cv::Mat &dest, cv::Mat &alpha); // split an image with n channels to BGR + alpha
std::vector<cv::Mat> SplitImage(const cv::Mat source); // split image to channels

//// Analysis
void SobelGradientAngle(const cv::Mat &source, const int &kernelSize, const bool blur, cv::Mat &gradient, cv::Mat &angle); // get Sobel gradients and angles from image
cv::Mat SobelizeImage(const cv::Mat &source, const int &kernelSize=3, const bool blur=true); // get Sobel image
std::vector<double> HistogramImageGray(const cv::Mat &source); // compute histogram of gray image
std::vector<double> HistogramImageGrayWithMask(const cv::Mat &source, const cv::Mat &mask); // compute histogram of gray image using a mask
cv::Vec3d MeanWeightedColor(const cv::Mat &source, const cv::Mat &mask); // use an histogram to get mean weighted color of an image area
double MeanWeightedGray(const cv::Mat &source, const cv::Mat &mask); // use an histogram to get mean weighted gray of an image area
cv::Mat CreatePaletteImageFromImage(const cv::Mat3b &source); // parse BGR image and create a one-line RGB palette image from all colors - super-fast !
void ImageLuminanceReflectance(const cv::Mat &source, cv::Mat &reflectance, cv::Mat &illumination); // get reflectance and illumination from RGB image using retinex theory

//// Image files
void SavePNG(const std::string &filename, const cv::Mat &source, const bool &transparency); // save PNG with or without transparency

//// MATLAB equivalents
enum Conv2DShape {
    conv2d_full,
    conv2d_same,
    conv2d_valid
};
cv::Mat conv2d(const cv::Mat &img, const cv::Mat &kernel, const Conv2DShape &shape); // equivalent to MATLAB conv2d
cv::Mat filter2(const cv::Mat &img, const cv::Mat &kernel); // equivalent to MATLAB filter2

#endif // IMAGEUTILS_H
