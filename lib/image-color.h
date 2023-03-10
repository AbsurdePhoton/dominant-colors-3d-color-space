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
#   - Means  for images
#   - Dominant colors for images
#
#-------------------------------------------------*/

#ifndef IMAGECOLOR_H
#define IMAGECOLOR_H

#include "opencv2/opencv.hpp"

#include "dominant-colors.h"
#include "image-utils.h"
#include "randomizer.h"


enum gradientType {gradient_flat, gradient_linear, gradient_doubleLinear, gradient_radial}; // gradient types

enum curveType {curve_linear, curve_cosinus2, curve_sigmoid, curve_cosinus, curve_cos2sqrt,
                curve_power2, curve_cos2power2, curve_power3, curve_undulate, curve_undulate2, curve_undulate3}; // gray curve types

enum noiseType {noise_regular, noise_regular_shifted, noise_gaussian, noise_uniform, noise_blue}; // noise types

//// Image color utils
cv::Mat ConvertImageLabToGray(const cv::Mat &source); // get a gray image from CV_64FC3 OKLab or CIELab image

//// Conversion of images to other colors spaces
cv::Mat ConvertImageRGBtoCIELab(const cv::Mat &source); // convert RGB image to CIELab
cv::Mat ConvertImageRGBtoCIELCHab(const cv::Mat &source); // convert RGB image to CIE LCHab
cv::Mat ConvertImageRGBtoOKLAB(const cv::Mat &source); // convert RGB image to OKLAB
cv::Mat ConvertImageCIELabToRGB(const cv::Mat &source); // convert CIELab image to RGB
cv::Mat ConvertImageCIELCHabToRGB(const cv::Mat &source); // convert CIE LCHab image to RGB
cv::Mat ConvertImageRGBtoLinear(const cv::Mat &source); // convert RGB image [0..1] to linear [0..1]
cv::Mat ConvertImageLinearToRGB(const cv::Mat &source); // convert RGB image to linear (source is CV_64FC3)
cv::Mat ConvertImageOKLABtoRGB(const cv::Mat &source, const bool &clip=false, const double &alpha=0.05); // convert OKLAB image to RGB
cv::Mat ConvertImageRGBtoCIEHSLC(const cv::Mat &source, const bool &clampValues); // convert RGB image to HSLC (H from HSL, S L and C from CIELab) - useful for filtering
cv::Mat ConvertImageRGBtoOKLABHSLC(const cv::Mat &source, const bool &clampValues); // convert RGB image to HSLC (H from HSL, S L and C from OKLAB)
void CreateCIELabPalettefromRGB(const int &Rvalue, const int &Gvalue, const int &Bvalue, const int &paletteSize, const int &sections,
                                const std::string filename, const bool &grid, const int &gap, const bool &invertCL); // test : create Lightness * Chroma palette image for one RGB color with CIELab
void CreateOKLABpalettefromRGB(const int &Rvalue, const int &Gvalue, const int &Bvalue, const int &paletteSize, const int &sections,
                                const std::string filename, const bool &grid, const int &gap, const bool &invertCL); // test : create Lightness * Chroma palette image for one RGB color with OKLAB
cv::Vec3b RGBtoLinear(const cv::Vec3b &color); // Apply linear RGB gamma correction to sRGB with RGB in [0..255] (faster)
cv::Vec3b LinearToRGB(const cv::Vec3b &color); // Apply linear gamma correction from sRGB with RGB in [0..255] (faster)
void AnalyzeCIELabCurveImage(const int &sections, const std::string filename); // create CSV file of maximum Chroma values for each Lighness step from Lightness * Chroma palette image (CIELAB)
void AnalyzeOKLABcurveImage(const int &sections, const std::string filename); // create CSV file of maximum Chroma values for each Lighness step from Lightness * Chroma palette image (OKLAB)

//// Gradients
double GrayCurve(const int &color, const int &type, const int &begin, const int &range); // return a value transformed by a function, all values and results are in [0..1]
void GradientFillGray(const int &gradient_type, cv::Mat &img, const cv::Mat &msk,
                      const cv::Point &beginPoint, const cv::Point &endPoint,
                      const int &beginColor, const int &endColor,
                      const int &curve, cv::Rect area = cv::Rect(0, 0, 0, 0)); // fill a 1-channel image with the mask converted to gray gradients
void GradientFillColor(const int &gradient_type, cv::Mat &img, const cv::Mat &msk, const cv::Point &beginPoint,
                      const cv::Point &endPoint, const cv::Vec3b &beginColor, const cv::Vec3b &endColor,
                      const int &curve, cv::Rect area, const int &noise); // fill a 3-channel image with the mask converted to color gradients - use 'curve_cosinus2' as 'curve' to get the best smooth transition

//// Color mean
cv::Vec3b ColorMean(const cv::Mat &source, const int &sX, const int &sY, const int &border); // image is BGR
cv::Vec3b ColorMeanCross(const cv::Mat &source, const int &sX, const int &sY); // computes the mean of one pixel in BGR image, using pixels around it, cross-shaped

//// Dominant colors
cv::Vec3d ImageDominantColor(const cv::Mat &lab); // get dominant color from OKLab or CIELab image


#endif // IMAGECOLOR_H
