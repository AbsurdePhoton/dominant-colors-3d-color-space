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


#include "image-utils.h"


///////////////////////////////////////////////////////////
//// General
///////////////////////////////////////////////////////////

int GetByteInRange(const int &byte) // get a byte in range [0..255]
{
    if (byte < 0)
        return 0;
    else if (byte > 255)
        return 255;

    // if other conditions fail
    return byte;
}

///////////////////////////////////////////////////////////
//// Utils
///////////////////////////////////////////////////////////

int CountRGBUniqueValues(const cv::Mat &source) // count number of RGB colors in BGR non-alpha image
{
    std::set<int> unique;

    for (cv::Vec3b &p : cv::Mat_<cv::Vec3b>(source)) // iterate over pixels (assummes CV_8UC3 !)
        unique.insert((p[2] << 16) | (p[1] << 8) | (p[0])); // "hash" representation of the pixel

    return unique.size();
}

cv::Mat NormalizeImage(const cv::Mat & source) // normalize [0..255] image to [0..1] - returns a CV_64F image
{
    cv::Mat result;

    switch (source.channels()) {
        case 1:
            result = cv::Mat(source.rows, source.cols, CV_64FC1); break;
        case 2:
            result = cv::Mat(source.rows, source.cols, CV_64FC2); break;
        case 3:
            result = cv::Mat(source.rows, source.cols, CV_64FC3); break;
        case 4:
            result = cv::Mat(source.rows, source.cols, CV_64FC4);
    }

    const uchar* sourceP = source.ptr<uchar>(0);
    double* resultP = result.ptr<double>(0);

    int total = source.cols * source.rows * source.channels();
    int n;
    #pragma omp parallel for private(n)
    for (n = 0; n < total; n++) {
        resultP[n] = sourceP[n] / 255.0;
    }

    return result;
}

cv::Mat DeNormalizeImage(const cv::Mat & source) // denormalize [0..1] image to [0..255] - needs a CV_64F input
{
    cv::Mat result;

    switch (source.channels()) {
        case 1:
            result = cv::Mat(source.rows, source.cols, CV_8UC1); break;
        case 2:
            result = cv::Mat(source.rows, source.cols, CV_8UC2); break;
        case 3:
            result = cv::Mat(source.rows, source.cols, CV_8UC3); break;
        case 4:
            result = cv::Mat(source.rows, source.cols, CV_8UC4); break;
    }

    const double* sourceP = source.ptr<double>(0);
    uchar* resultP = result.ptr<uchar>(0);

    int total = source.cols * source.rows * source.channels();
    int n;
    #pragma omp parallel for private(n)
    for (n = 0; n < total; n++) {
        resultP[n] = round(sourceP[n] * 255.0);
    }

    return result;
}

bool MatEqual(const cv::Mat &one, const cv::Mat &two) // compare two Mat
{
    // one image empty ?
    if ((one.empty()) or (two.empty()))
        return false;

    // image dimensions and channels different ?
    if ((one.channels() != two.channels()) or (one.cols != two.cols) or (one.rows != two.rows))
        return false;

    cv::Mat dif;
    cv::compare(one, two, dif, cv::CMP_NE);
    int nz = cv::countNonZero(dif);

    return nz == 0;
}

bool IsImageSolidColor(const cv::Mat &source) // tells if BGR image consists of a unique color
{
   cv::Vec3b value = source.at<cv::Vec3b>(0, 0);
   const cv::Vec3b *sourceP = source.ptr<cv::Vec3b>(0);
   bool same = true;
   int total = source.cols * source.rows;
   for (int n = 0; n < total; n++) {
       if (sourceP[n] != value) {
           same = false;
           break;
       }
   }

   return same;
}

///////////////////////////////////////////////////////////
//// Conversions from QImage and QPixmap to Mat
///////////////////////////////////////////////////////////

cv::Mat QImage2Mat(const QImage &source) // Convert QImage to Mat, every number of channels supported
{
    switch (source.format()) { // which forcv::Mat ?

        case QImage::Format_ARGB32: // 8-bit, 4 channel
        case QImage::Format_ARGB32_Premultiplied: {
            cv::Mat temp(source.height(), source.width(), CV_8UC4,
                        const_cast<uchar*>(source.bits()),
                        static_cast<size_t>(source.bytesPerLine()));
            return temp;
        }

        case QImage::Format_RGB32: { // 8-bit, 3 channel
            cv::Mat temp(source.height(), source.width(), CV_8UC4,
                        const_cast<uchar*>(source.bits()),
                        static_cast<size_t>(source.bytesPerLine()));
            cv::Mat tempNoAlpha;
            cv::cvtColor(temp, tempNoAlpha, cv::COLOR_BGRA2BGR);   // drop the all-white alpha channel
        return tempNoAlpha;
        }

        case QImage::Format_RGB888: { // 8-bit, 3 channel
            QImage swapped = source.rgbSwapped();
            return cv::Mat(swapped.height(), swapped.width(), CV_8UC3,
                       const_cast<uchar*>(swapped.bits()),
                       static_cast<size_t>(swapped.bytesPerLine())).clone();
        }

        case QImage::Format_Indexed8: { // 8-bit, 1 channel
            cv::Mat temp(source.height(), source.width(), CV_8UC1,
                        const_cast<uchar*>(source.bits()),
                        static_cast<size_t>(source.bytesPerLine()));
            return temp;
        }

        default:
            break;
    }

    return cv::Mat(); // return empty cv::Mat if type not found
}

cv::Mat QPixmap2Mat(const QPixmap &source) // Convert Pixmap to Mat
{
    return QImage2Mat(source.toImage()); // simple !
}

///////////////////////////////////////////////////////////
//// cv::Mat conversions to QImage and QPixmap
///////////////////////////////////////////////////////////

QImage Mat2QImage(const cv::Mat &source) // convert BGR cv::Mat to RGB QImage
{
    if (source.empty())
        return QImage();

     cv::Mat temp;
     cv::cvtColor(source, temp, cv::COLOR_BGR2RGB); // convert cv::Mat BGR to QImage RGB
     QImage dest((const uchar *) temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888); // conversion
     dest.bits(); // enforce deep copy of QImage::QImage (const uchar * data, int width, int height, Forcv::Mat format)

     return dest;
}

QPixmap Mat2QPixmap(const cv::Mat &source) // convert cv::Mat to QPixmap
{
    if (source.empty())
        return QPixmap();

    QImage i = Mat2QImage(source); // first step: convert to QImage
    QPixmap p = QPixmap::fromImage(i, Qt::AutoColor); // then convert QImage to QPixmap

    return p;
}

QPixmap Mat2QPixmapResized(const cv::Mat &source, const int &width, const int &height, const bool &smooth) // convert cv::Mat to resized QPixmap
{
    if (source.empty())
        return QPixmap();

    Qt::TransformationMode quality; // quality
    if (smooth) quality = Qt::SmoothTransformation;
        else quality = Qt::FastTransformation;

    QImage i = Mat2QImage(source); // first step: convert to QImage
    QPixmap p = QPixmap::fromImage(i, Qt::AutoColor); // then convert QImage to QPixmap

    return p.scaled(width, height, Qt::KeepAspectRatio, quality); // resize with high quality
}

QImage cvMatToQImage(const cv::Mat &source) // another implementation BGR to RGB, every number of channels supported
{
    if (source.empty())
        return QImage();

    switch (source.type()) { // which type ?

        case CV_8UC4: { // 8-bit, 4 channel
            QImage image(   source.data,
                            source.cols, source.rows,
                            static_cast<int>(source.step),
                            QImage::Format_ARGB32);
            image.bits(); // enforce deep copy of QImage::QImage (const uchar * data, int width, int height, Forcv::Mat format)
            return image;
        }


        case CV_8UC3: { // 8-bit, 3 channel
            QImage image(   source.data,
                            source.cols, source.rows,
                            static_cast<int>(source.step),
                            QImage::Format_RGB888);
            image.bits(); // enforce deep copy of QImage::QImage (const uchar * data, int width, int height, Forcv::Mat format)
            return image.rgbSwapped();
        }

        case CV_8UC1: { // 8-bit, 1 channel
            QImage image(source.data, source.cols, source.rows,
                         static_cast<int>(source.step),
                         QImage::Format_Grayscale8);
            image.bits(); // enforce deep copy of QImage::QImage (const uchar * data, int width, int height, Forcv::Mat format)
            return image;
        }
    }

    return QImage(); // return empty cv::Mat if type not found
}

///////////////////////////////////////////////////////////
//// Copy and Paste
///////////////////////////////////////////////////////////

cv::Mat CopyFromImage(const cv::Mat &source, const cv::Rect &frame) // copy part of an image - if parts of frame outside source image, non-copied pixels are black
{
    cv::Mat dest = cv::Mat::zeros(frame.height, frame.width, source.type()); // result is the same size as frame

    // compute limits inside source image
    int minX = std::max(0, frame.x);
    int minY = std::max(0, frame.y);
    int maxX = std::min(frame.x + frame.width, source.cols);
    int maxY = std::min(frame.y + frame.height, source.rows);

    cv::Rect sourceRect = cv::Rect(minX, minY, maxX - minX, maxY - minY); // source rect to copy from
    if ((sourceRect.width <= 0) or (sourceRect.height <= 0))
        return dest;

    cv::Rect destRect = sourceRect - frame.tl(); // destination rect to copy to

    source(sourceRect).copyTo(dest(destRect)); // copy (eventually resized) frame to destination

    return dest;
}

void PasteImageColor(cv::Mat &background, const cv::Mat &foreground, const int &originX, const int &originY, const bool transparency, const cv::Vec3b transparentColor) // copy non-alpha BGR image onto another
{
    // compute limits of image to paste
    int minX = std::max(0, -originX);
    int minY = std::max(0, -originY);
    int maxX = std::min(foreground.cols, background.cols - originX);
    int maxY = std::min(foreground.rows, background.rows - originY);

    cv::Vec3b pixel;

    for (int j = minY; j < maxY; j++) {
        cv::Vec3b* backgroundP = background.ptr<cv::Vec3b>(j + originY);
        const cv::Vec3b* foregroundP = foreground.ptr<cv::Vec3b>(j);
        for (int i = minX; i < maxX; i++) {
            //if ((j + originY < background.rows) and (i + originX < background.cols) and (j + originY >= 0) and (i + originX >= 0)) {
                if (!transparency) {
                    backgroundP[i + originX] = foregroundP[i];
                }
                else {
                    pixel = foregroundP[i]; // current pixel
                    if (pixel != transparentColor) // is it transparent ?
                        backgroundP[i + originX] = pixel; // copy non-zero values only
                }

            //}
        }
    }
}

void PasteImageGray(cv::Mat &background, const cv::Mat &foreground, const int &originX, const int &originY, const bool transparency, const uchar transparentColor) // copy gray image onto another
{
    // compute limits of image to paste
    int minX = std::max(0, -originX);
    int minY = std::max(0, -originY);
    int maxX = std::min(foreground.cols, background.cols - originX);
    int maxY = std::min(foreground.rows, background.rows - originY);

    uchar pixel;
    for (int j = minY; j < maxY; j++) {
        uchar* backgroundP = background.ptr<uchar>(j + originY);
        const uchar* foregroundP = foreground.ptr<uchar>(j);
        for (int i = minX; i < maxX; i++) {
            //if ((j + originY < background.rows) and (i + originX < background.cols) and (j + originY >= 0) and (i + originX >= 0)) {
                if (!transparency) {
                    backgroundP[i + originX] = foregroundP[i];
                }
                else {
                    pixel = foregroundP[i]; // current pixel
                    if (pixel != transparentColor) // is it transparent ?
                        backgroundP[i + originX] = pixel; // copy non-zero values only
                }

            //}
        }
    }
}

void PasteImageAlpha(cv::Mat &background, const cv::Mat &foreground, const cv::Point pos) // paste alpha BGR image on non-alpha background
{
    // compute limits of image to paste
    int minX = std::max(0, -pos.x);
    int minY = std::max(0, -pos.y);
    int maxX = std::min(foreground.cols, background.cols - pos.x);
    int maxY = std::min(foreground.rows, background.rows - pos.y);

    double alpha;
    for (int j = minY; j < maxY; j++) {
        const cv::Vec4b* foregroundP = foreground.ptr<cv::Vec4b>(j);
        cv::Vec3b* backgroundP = background.ptr<cv::Vec3b>(j + pos.y);
        for (int i = minX; i < maxX; i++) {
            alpha = foregroundP[i][3] / 255.0;
            for (int c = 0; c < 3; c++) {
                // formula : Intensity = alpha * Foreground + (1 - alpha) * Background (with alpha in [0..1])
                backgroundP[i + pos.x][c] = GetByteInRange(alpha * double(foregroundP[i][c]) + (1.0 - alpha) * double(backgroundP[i + pos.x][c]));
            }
        }
    }
}

void PasteImageGrayPlusAlpha(cv::Mat &background, const cv::Mat &foreground, const cv::Mat &alpha, const cv::Point pos) // paste gray + alpha image on non-alpha BGR background
{
    // compute limits of image to paste
    int minX = std::max(0, -pos.x);
    int minY = std::max(0, -pos.y);
    int maxX = std::min(foreground.cols, background.cols - pos.x);
    int maxY = std::min(foreground.rows, background.rows - pos.y);

    double alphaValue;
    for (int j = minY; j < maxY; j++) {
        const uchar* foregroundP = foreground.ptr<uchar>(j);
        const uchar* alphaP = alpha.ptr<uchar>(j);
        cv::Vec3b* backgroundP = background.ptr<cv::Vec3b>(j + pos.y);
        for (int i = minX; i < maxX; i++) {
            alphaValue = alphaP[i] / 255.0;
            for (int c = 0; c < 3; c++) {
                // formula : Intensity = alpha * Foreground + (1 - alpha) * Background (with alpha in [0..1])
                backgroundP[i + pos.x][c] = GetByteInRange(alphaValue * double(foregroundP[i]) + (1.0 - alphaValue) * double(backgroundP[i + pos.x][c]));
            }
        }
    }
}

void PasteImageColorPlusAlpha(cv::Mat &background, const cv::Mat &foreground, const cv::Mat &alpha, const cv::Point pos) // paste BGR + alpha image on non-alpha BGR background
{
    // compute limits of image to paste
    int minX = std::max(0, -pos.x);
    int minY = std::max(0, -pos.y);
    int maxX = std::min(foreground.cols, background.cols - pos.x);
    int maxY = std::min(foreground.rows, background.rows - pos.y);

    double alphaValue;
    for (int j = minY; j < maxY; j++) {
        const cv::Vec3b* foregroundP = foreground.ptr<cv::Vec3b>(j);
        const uchar* alphaP = alpha.ptr<uchar>(j);
        cv::Vec3b* backgroundP = background.ptr<cv::Vec3b>(j + pos.y);
        for (int i = minX; i < maxX; i++) {
            alphaValue = alphaP[i] / 255.0;
            for (int c = 0; c < 3; c++) {
                // formula : Intensity = alpha * Foreground + (1 - alpha) * Background (with alpha in [0..1])
                backgroundP[i + pos.x][c] = GetByteInRange(alphaValue * double(foregroundP[i][c]) + (1.0 - alphaValue) * double(backgroundP[i + pos.x][c]));
            }
        }
    }
}

void PasteImagePlusAlpha(cv::Mat &background, const cv::Mat &foreground, const cv::Mat &alpha, const cv::Point pos) // paste (gray or color) + alpha image on non-alpha BGR background
{
    if (foreground.channels() == 1)
        PasteImageGrayPlusAlpha(background, foreground, alpha, pos);
    else if (foreground.channels() == 3)
        PasteImageColorPlusAlpha(background, foreground, alpha, pos);
}

void PasteImageAlphaOnAlpha(cv::Mat &background, const cv::Mat &foreground, const cv::Point pos) // paste alpha BGR image on alpha BGR background
{
    // compute limits of image to paste
    int minX = std::max(0, -pos.x);
    int minY = std::max(0, -pos.y);
    int maxX = std::min(foreground.cols, background.cols - pos.x);
    int maxY = std::min(foreground.rows, background.rows - pos.y);

    double alpha;
    for (int j = minY; j < maxY; j++) {
        const cv::Vec4b* foregroundP = foreground.ptr<cv::Vec4b>(j);
        cv::Vec4b* backgroundP = background.ptr<cv::Vec4b>(j + pos.y);
        for (int i = minX; i < maxX; i++) {
            alpha = foregroundP[i][3] / 255.0;
            for (int c = 0; c < 3; c++) {
                // formula : Intensity = alpha * Foreground + (1 - alpha) * Background (with alpha in [0..1])
                backgroundP[i + pos.x][c] = GetByteInRange(alpha * double(foregroundP[i][c]) + (1.0 - alpha) * double(backgroundP[i + pos.x][c]));
            }
            backgroundP[i + pos.x][3] = (backgroundP[i][3] / 255.0) * (foregroundP[i][3] / 255.0);
        }
    }
}

///////////////////////////////////////////////////////////
//// Alpha channel
///////////////////////////////////////////////////////////

cv::Mat AddAlphaToImage(const cv::Mat &source, const cv::Mat &mask) // add alpha channel to BGR image - if mask is empty, transparency = black (0,0,0)
{
    std::vector<cv::Mat> matChannels;
    cv::split(source, matChannels); // split image in separate channels

    if (mask.empty()) { // no mask ? create one with transparency of source image = black (0,0,0)
        cv::Mat alpha;
        cv::cvtColor(source, alpha, cv::COLOR_BGR2GRAY); // image to gray
        cv::Mat mask = 255 - (alpha == 0); // compute mask
        alpha.setTo(255, mask); // alpha to max where pixels are
        matChannels.push_back(alpha); // add alpha channel to channels stack, contains transparency based on black color
    }
    else {
        matChannels.push_back(mask); // add alpha channel to channels stack, contains provided mask
    }

    cv::Mat result;
    cv::merge(matChannels, result); // merge back all channels + BGR + alpha

    return result;
}

cv::Mat ImageAlphaWithGrid(const cv::Mat &source, const int &interval, const cv::Vec3b color1, const cv::Vec3b color2) // get BGR image from BGRA with gray blocks where there is transparency
{
    cv::Mat result = cv::Mat(source.rows, source.cols, CV_8UC3);

    for (int j = 0; j < source.rows; j++) {
        cv::Vec3b* resultP = result.ptr<cv::Vec3b>(j);
        for (int i = 0; i < source.cols; i++) {
            if ((j % interval >= 0) and (j % interval < interval / 2)) {
                if ((i % interval >= 0) and (i % interval < interval / 2)) {
                    resultP[i] = color1;
                }
                else {
                    resultP[i] = color2;
                }
            }
            else {
                if ((i % interval >= 0) and (i % interval < interval / 2)) {
                    resultP[i] = color2;
                }
                else {
                    resultP[i] = color1;
                }
            }
        }
    }

    PasteImageAlpha(result, source, cv::Point(0, 0));

    return result;
}

void ImageToBGRplusAlpha(const cv::Mat &source, cv::Mat &dest, cv::Mat &alpha) // split an image with n channels to BGR + alpha
{
    alpha = cv::Mat(source.rows, source.cols, CV_8UC1);
    if (source.channels() == 1) { // if gray image, no alpha mask -> transform it to 3 channels BGR
        cv::cvtColor(source, dest, cv::COLOR_GRAY2BGR); // convert gray image to BGR
        alpha = 255; // alpha is set to maximum opacity
    }
    else if (source.channels() == 2) { // source is gray + alpha
        std::vector<cv::Mat1b> channels; // split source image
        cv::split(source, channels);

        std::vector<cv::Mat1b> BGR; // merge gray channel 3 times to obtain a BGR image
        BGR.push_back(channels[0]);
        BGR.push_back(channels[0]);
        BGR.push_back(channels[0]);
        cv::merge(BGR, dest);

        alpha = channels[1]; // alpha is from source image
    }
    else if (source.channels() == 3) { // if 3 channels, no alpha mask -> keep image as is
        source.copyTo(dest); // keep image
        alpha = 255; // alpha is set to maximum opacity
    }
    else if (source.channels() == 4) { // source is BGRA
        std::vector<cv::Mat1b> channels; // split source image
        cv::split(source, channels);

        std::vector<cv::Mat1b> BGR; // merge first 3 channels to obtain a BGR image
        BGR.push_back(channels[0]);
        BGR.push_back(channels[1]);
        BGR.push_back(channels[2]);
        cv::merge(BGR, dest);

        channels[3].copyTo(alpha); // alpha is from source image
    }
}

std::vector<cv::Mat> SplitImage(const cv::Mat source) // split image to channels
{
    std::vector<cv::Mat> matChannels;
    cv::split(source, matChannels); // split image in separate channels

    return matChannels;
}

///////////////////////////////////////////////////////////
//// Analysis
///////////////////////////////////////////////////////////

void SobelGradientAngle(const cv::Mat &source, const int &kernelSize, const bool blur, cv::Mat &gradient, cv::Mat &angle) // get Sobel gradients and angles from image
    // to use scharr 3x3 matrix use kernelSize = -1
{
    cv::Mat imageCopy;

    if (blur) // need to blur image before ?
        cv::edgePreservingFilter(source, imageCopy, cv::RECURS_FILTER, 200, 0.4); // use edge-resistant blur
    else
        source.copyTo(imageCopy); // no blur

    imageCopy.convertTo(imageCopy, CV_64F); // image to work upon

    // compute magnitude of edges
    cv::Mat sobel_x, sobel_y;
    cv::Sobel(imageCopy, sobel_x, CV_64F, 1, 0, kernelSize, 1, 0, cv::BORDER_DEFAULT); // sobel filter on "x" direction
    cv::Sobel(imageCopy, sobel_y, CV_64F, 0, 1, kernelSize, 1, 0, cv::BORDER_DEFAULT); // and on "y" direction

    cv::cartToPolar(sobel_x, sobel_y, gradient, angle, false);
    // magnitude = sqrt(dx² + dy²)
    // angle = atan2(dy,dx) (in radians)
    // note that angles are perpendicular to edges ! If you want to get a line angle add Pi/2
}

cv::Mat SobelizeImage(const cv::Mat &source, const int &kernelSize, const bool blur) // get Sobel image
{
    cv::Mat imageCopy, result;

    if (blur) // need to blur image before ?
        cv::edgePreservingFilter(source, imageCopy, cv::RECURS_FILTER, 200, 0.4); // use edge-resistant blur
    else
        source.copyTo(imageCopy);

    // compute magnitude of edges
    cv::Mat sobel_x, sobel_y, sobel;
    cv::Sobel(imageCopy, sobel_x, CV_32F, 1, 0, kernelSize, 1, 0, cv::BORDER_DEFAULT); // sobel filter on "x" direction
    cv::Sobel(imageCopy, sobel_y, CV_32F, 0, 1, kernelSize, 1, 0, cv::BORDER_DEFAULT); // and on "y" direction
    // nota : Scharr() could be used instead of sobel
    // nota : seems that spatialGradient() can do in one pass the x and y sobel operation ? is it faster ? should try it

    cv::magnitude(sobel_x, sobel_y, sobel); // magnitude = sqrt(dx² + dy²)
    cv::convertScaleAbs(sobel, result); // convert back sobel data to image form

    return result;
}

std::vector<double> HistogramImageGray(const cv::Mat &source) // compute histogram of gray image
{
    std::vector<double> histogram;
    histogram.reserve(256);
    for (int i = 0; i < 256; i++)
        histogram[i] = 0;
    int total = source.cols * source.rows;

    const uchar* sourceP = source.ptr<uchar>(0);
    for (int n = 0; n < total; n++)
        histogram[sourceP[n]]++;

    return histogram;
}

std::vector<double> HistogramImageGrayWithMask(const cv::Mat &source, const cv::Mat &mask) // compute histogram of gray image using a mask
{
    if (mask.empty()) // no mask = entire image
        return HistogramImageGray(source); // so return entire image histogram

    std::vector<double> histogram;
    histogram.reserve(256);
    for (int i = 0; i < 256; i++)
        histogram[i] = 0;
    int total = source.cols * source.rows;

    const uchar* sourceP = source.ptr<uchar>(0);
    const uchar* maskP = mask.ptr<uchar>(0);
    for (int n = 0; n < total; n++)
        if (maskP[n] != 0)
            histogram[sourceP[n]]++;

    return histogram;
}

cv::Vec3d MeanWeightedColor(const cv::Mat &source, const cv::Mat &mask) // use an histogram to get mean weighted color of an image area (BGR or BGRA)
{
    double total = cv::countNonZero(mask); // total number of pixels in mask / area

    // split image to gray channels
    std::vector<cv::Mat> grayChannels;
    cv::split(source, grayChannels);
    int nbChannels = std::min(3, source.channels()); // no more than 3 channels, this way we can compute gray, color and color+alpha images

    std::vector<std::vector<double>> histograms;
    for (int c = 0; c < nbChannels; c++)
        histograms.push_back(HistogramImageGrayWithMask(grayChannels[c], mask));

    cv::Vec3d result = cv::Vec3d(0, 0, 0);
    for (int c = 0; c < nbChannels; c++)
        for (int n = 0; n < 256; n++)
            result[c] += double(n * histograms[c][n]) / total;

    return result;
}

double MeanWeightedGray(const cv::Mat &source, const cv::Mat &mask) // use an histogram to get mean weighted gray of an image area
{
    double total = cv::countNonZero(mask); // total number of pixels in mask / area

    std::vector<double> histogram = HistogramImageGrayWithMask(source, mask);

    double result = 0;
    for (int n = 0; n < 256; n++)
        result += double(n * histogram[n]) / total;

    return result;
}

cv::Mat CreatePaletteImageFromImage(const cv::Mat3b &source) // parse BGR image and create a one-line RGB palette image from all colors - super-fast !
    // the resulting Mat can be used as palette values
    // it is in RGB format, not BGR !
{
    // get RGB values in array std::set (super-fast !)
    std::set<int> paletteInt;
    for (cv::Vec3b &p : cv::Mat_<cv::Vec3b>(source)) // iterate over pixels
        paletteInt.insert((p[2] << 16) | (p[1] << 8) | (p[0])); // RGB "hatch" representation of the pixel

    int nbColors = paletteInt.size(); // how many colors found ?
    cv::Mat3b palette = cv::Mat::zeros(sqrt(nbColors) + 1, sqrt(nbColors) + 1, CV_8UC3); // create palette image on one line, nbColors pixels wide

    cv::Vec3b* paletteP = palette.ptr<cv::Vec3b>(0);
    auto value = paletteInt.begin();
    for (int n = 0; n < nbColors; n++) { // parse palette
        paletteP[n] = cv::Vec3b(*value & 0x000000FF, (*value & 0x0000FF00) >> 8, (*value & 0x00FF0000) >> 16); // and set pixel color to palette image
        value++;
    }

    return palette;
}

void ImageLuminanceReflectance(const cv::Mat &source, cv::Mat &reflectance, cv::Mat &luminance) // get reflectance and luminance from RGB image using retinex theory
    // Adaptive image enhancement method for correcting low-luminance images (2019), Wang et al.
    // loosely adapted from original code : https://github.com/dengyueyun666/Image-Contrast-Enhancement
{
    // conversion to HSV (step 2)
    cv::Mat HSV;
    cv::cvtColor(source, HSV, cv::COLOR_BGR2HSV_FULL);
    std::vector<cv::Mat> HSV_channels;
    cv::split(HSV, HSV_channels);
    cv::Mat S = HSV_channels[1];
    cv::Mat V = HSV_channels[2];

    // estimation of the reflection component : gaussian convolution (multiscale frequency extraction) (step 3)
    int ksize = (std::max(source.rows, source.cols) / 1024 + 2) * 2 + 1; // obtain a kernel size from image max size, minimum size is 5
    cv::Mat gauker1 = cv::getGaussianKernel(ksize, 15);
    cv::Mat gauker2 = cv::getGaussianKernel(ksize, 80);
    cv::Mat gauker3 = cv::getGaussianKernel(ksize, 250);

    cv::Mat gauV1, gauV2, gauV3;
    cv::filter2D(V, gauV1, CV_64F, gauker1, cv::Point(-1, -1), 0, cv::BORDER_CONSTANT);
    cv::filter2D(V, gauV2, CV_64F, gauker2, cv::Point(-1, -1), 0, cv::BORDER_CONSTANT);
    cv::filter2D(V, gauV3, CV_64F, gauker3, cv::Point(-1, -1), 0, cv::BORDER_CONSTANT);

    cv::Mat V_g = (gauV1 + gauV2 + gauV3) / 3.0; // average of gaussian convolutions

    // Adaptive brightness enhancement (step 4)
    cv::Scalar avg_S = cv::mean(S);
    avg_S += cv::Scalar(255.0);
    double k = avg_S[0];

    // get reflectance
    cv::Mat V_double;
    V.convertTo(V_double, CV_64F);
    reflectance = ((255 + k) * V_double).mul(1.0 / (cv::max(V_double, V_g) + k));

    // get luminance : considering that for a retinex image I = R * L ==> L = I / R
    const cv::Vec3b* sourceP = source.ptr<cv::Vec3b>(0); // pointer to source image
    luminance = cv::Mat::zeros(source.rows, source.cols, CV_8UC3); // luminance image
    cv::Vec3b* luminanceP = luminance.ptr<cv::Vec3b>(0); // pointer to luminance image
    double* reflectanceP = reflectance.ptr<double>(0); // pointer to reflectance image
    int total = source.cols * source.rows; // total of pixels
    for (int n = 0; n < total; n++) { // parse all pixels
        luminanceP[n][0] = 255 * sourceP[n][0] / reflectanceP[n]; // L = I / R, for each channel
        luminanceP[n][1] = 255 * sourceP[n][1] / reflectanceP[n];
        luminanceP[n][2] = 255 * sourceP[n][2] / reflectanceP[n];
    }

    // convert reflectance to 3 channels "gray" image
    reflectance.convertTo(reflectance, CV_8UC1);
    std::vector<cv::Mat> reflectance_channels;
    reflectance_channels.push_back(reflectance);
    reflectance_channels.push_back(reflectance);
    reflectance_channels.push_back(reflectance);
    cv::merge(reflectance_channels, reflectance);
}

///////////////////////////////////////////////////////////
//// Image files
///////////////////////////////////////////////////////////

void SavePNG(const std::string &filename, const cv::Mat &source, const bool &transparency) // save PNG with or without transparency
{
    if (!transparency) {
        imwrite(filename, source); // save to PNG image without alpha
    }
    else {
        cv::Mat alpha;
        source.copyTo(alpha);
        alpha = AddAlphaToImage(alpha, cv::Mat());
        imwrite(filename, alpha); // save to PNG image with alpha
    }
}

///////////////////////////////////////////////////////////
//// MATLAB equivalents
///////////////////////////////////////////////////////////

cv::Mat conv2d(const cv::Mat &img, const cv::Mat &kernel, const Conv2DShape &shape) // equivalent to MATLAB conv2d
    // shape :
    // full - return the full convolution, including border
    // same - return only the part that corresponds to the original image
    // valid - return only the submatrix containing elements that were not influenced by the border
{
    cv::Mat dest;
    cv::Mat source = img;

    if(shape == conv2d_full) {
        source = cv::Mat();
        const int additionalRows = kernel.rows - 1;
        const int additionalCols = kernel.cols - 1;
        cv::copyMakeBorder(img, source, (additionalRows + 1) / 2, additionalRows / 2, (additionalCols + 1) / 2, additionalCols / 2, cv::BORDER_CONSTANT, cv::Scalar(0));
    }

    cv::Point anchor(kernel.cols - kernel.cols / 2 - 1, kernel.rows - kernel.rows / 2 - 1);
    cv::Mat kernelFlipped;
    cv::flip(kernel, kernelFlipped, -1); // this a convolution so flip the kernel !
    kernelFlipped /= cv::sum(kernelFlipped); // normalize it
    cv::filter2D(source, dest, img.depth(), kernelFlipped, anchor, 0, cv::BORDER_CONSTANT);

    if(shape == conv2d_valid) {
        dest = dest.colRange((kernel.cols - 1) / 2, dest.cols - kernel.cols / 2).rowRange((kernel.rows - 1) / 2, dest.rows - kernel.rows / 2);
    }

  return dest;
}

cv::Mat filter2(const cv::Mat &img, const cv::Mat &kernel) // equivalent to MATLAB filter2
{
    cv::Mat result;

    cv::filter2D(img, result, -1, kernel, cv::Point(-1, -1), 0, cv::BORDER_CONSTANT); // the trick is using BORDER_CONSTANT (0-padding)

    return result;
}
