/*#-------------------------------------------------
#
#     Dominant colors library with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.2 - 2019/10/28
#
#   - eigen vectors algorithm
#   - K-means algorithm
#
#-------------------------------------------------*/

#ifndef DOMINANT_H
#define DOMINANT_H

#include "opencv2/opencv.hpp"

//// Dominant colors

typedef struct color_node { // for eigen algorithm
    cv::Mat     mean;
    cv::Mat     cov;
    int       class_id;

    color_node *left;
    color_node *right;
} color_node;

std::vector<cv::Vec3b> DominantColorsEigen(const cv::Mat &img, const int &nb_colors, cv::Mat *quantized);

cv::Mat DominantColorsKMeans(const cv::Mat &image, const int &cluster_number, cv::Mat1f *dominant_colors);

#endif // DOMINANT_H
