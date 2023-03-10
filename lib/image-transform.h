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

#ifndef IMAGETRANSFORM_H
#define IMAGETRANSFORM_H

#include "opencv2/opencv.hpp"

#include "angles.h"
#include "image-utils.h"


static const int warpMax = 8;
enum warpType {warp_undulate, warp_undulate_decreasing, warp_comma_inflated, warp_comma_deflated,
               warp_curve_inflated, warp_curve_inflated_fat, warp_curve_deflated, warp_curve_apostrophe,
               warp_s};


//// Scaling
cv::Mat QualityUpCcaleImageDCCI(const cv::Mat &source); // resize image to double the size of original using DCCI algorithm
cv::Mat QualityDownScaleImage(const cv::Mat &source, const double &initialRatio); // good quality image downscaling algorithm, better than direct downscaling
cv::Mat QualityUpScaleImage(const cv::Mat &source, const double &initialRatio); // good quality upscaling algorithm, better than direct upscaling
cv::Mat QualityResizeImageAspectRatio(const cv::Mat &source, const cv::Size &frame); // Resize image keeping aspect ratio
cv::Mat ResizeImageAspectRatio(const cv::Mat &source, const cv::Size &frame); // Resize image keeping aspect ratio, fast with bilinear interpolation
double GetScaleToResize(const int &sourceWidth, const int &sourceHeight, const int &destination_size); // compute scale for an image to be resized to destination_size pixels

//// Transforms
cv::Mat MirrorImage(const cv::Mat &source, const bool &horizontal, const bool &vertical); // non-alpha or alpha image mirror horizontal and/or vertical
double WarpCurve(const int &type, const double &value, const double &rangeX, const double &rangeY, const double &pixels); // "warp" a value using mathematical functions and ranges
cv::Mat WarpImage(const cv::Mat &source, const bool &horizontal, const int &typeH, const int &pixelsX, const bool &vertical, const int &typeV, const int &pixelsY); // warp image on horizontal and / or vertical axis - decX and decY in pixels
cv::Mat RotateImage(const cv::Mat &source, const double &angleRad, const int &centerX, const int &centerY, int &dX, int &dY); // rotate alpha or non-alpha image by angle in degrees
void PasteImageAlphaRotated(cv::Mat &destination, const cv::Mat &source, const double &angle, const int &destX, const int &destY, const int &centerX, const int &centerY); // paste foreground with alpha to background without alpha


#endif // IMAGETRANSFORM_H
