/*#-------------------------------------------------
#
#     Dominant colors library with openCV
#               in 3D color spaces
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.2 - 2019/10/28
#
#   - eigen vectors algorithm
#   - K-means algorithm
#
#-------------------------------------------------*/

#include <opencv2/opencv.hpp>

#include "dominant-colors.h"
#include "mat-image-tools.h"

using namespace std;

/////////////////// Eigen vectors algorithm/////////////////

//// code adapted from Utkarsh Sinha
//// http://aishack.in/tutorials/dominant-color/

std::vector<color_node*> GetLeaves(color_node *root)
{
    std::vector<color_node*> ret;
    std::queue<color_node*> queue;
    queue.push(root);

    while (queue.size() > 0) {
        color_node *current = queue.front();
        queue.pop();

        if (current->left && current->right) {
            queue.push(current->left);
            queue.push(current->right);
            continue;
        }

        ret.push_back(current);
    }

    return ret;
}

std::vector<cv::Vec3b> GetDominantColors(color_node *root)
{
    std::vector<color_node*> leaves = GetLeaves(root);
    std::vector<cv::Vec3b> ret;

    for (unsigned int i=0; i < leaves.size(); i++) {
        cv::Mat mean = leaves[i]->mean;
        ret.push_back(cv::Vec3b(mean.at<double>(0) * 255.0,
                                mean.at<double>(1) * 255.0,
                                mean.at<double>(2) * 255.0));
    }

    return ret;
}

int GetNextClassId(color_node *root) {
    int maxid = 0;
    std::queue<color_node*> queue;
    queue.push(root);

    while (queue.size() > 0) {
        color_node* current = queue.front();
        queue.pop();

        if (current->class_id > maxid)
            maxid = current->class_id;

        if (current->left != NULL)
            queue.push(current->left);

        if (current->right)
            queue.push(current->right);
    }

    return maxid + 1;
}

void GetClassMeanCov(cv::Mat img, cv::Mat classes, color_node *node)
{
    const int width = img.cols;
    const int height = img.rows;
    const int class_id = node->class_id;

    cv::Mat mean = cv::Mat(3, 1, CV_64FC1, cv::Scalar(0));
    cv::Mat cov = cv::Mat(3, 3, CV_64FC1, cv::Scalar(0));

    // start out with the average color
    double pix_count = 0;
    for (int y = 0; y < height; y++) {
        cv::Vec3b* ptr = img.ptr<cv::Vec3b>(y);
        char16_t* ptrClass = classes.ptr<char16_t>(y);
        for (int x=0; x < width; x++) {
            if (ptrClass[x] != class_id)
                continue;

            cv::Vec3b color = ptr[x];
            cv::Mat scaled = cv::Mat(3, 1, CV_64FC1, cv::Scalar(0));
            scaled.at<double>(0) = color[0] / 255.0;
            scaled.at<double>(1) = color[1] / 255.0;
            scaled.at<double>(2) = color[2] / 255.0;

            mean += scaled;
            cov = cov + (scaled * scaled.t());

            pix_count++;
        }
    }

    cov = cov - (mean * mean.t()) / pix_count;
    mean = mean / pix_count;

    // node mean and covariance
    node->mean = mean.clone();
    node->cov = cov.clone();

    return;
}

void PartitionClass(cv::Mat img, cv::Mat classes, char16_t nextid, color_node *node) {
    const int width = img.cols;
    const int height = img.rows;
    const int class_id = node->class_id;

    const int new_id_left = nextid;
    const int new_id_right = nextid + 1;

    cv::Mat mean = node->mean;
    cv::Mat cov = node->cov;
    cv::Mat eigen_values, eigen_vectors;
    cv::eigen(cov, eigen_values, eigen_vectors);

    cv::Mat eig = eigen_vectors.row(0);
    cv::Mat comparison_value = eig * mean;

    node->left = new color_node();
    node->right = new color_node();

    node->left->class_id = new_id_left;
    node->right->class_id = new_id_right;

    // start out with average color
    for (int y = 0; y < height; y++) {
        cv::Vec3b* ptr = img.ptr<cv::Vec3b>(y);
        char16_t* ptr_class = classes.ptr<char16_t>(y);
        for (int x = 0; x < width; x++) {
            if (ptr_class[x] != class_id)
                continue;

            cv::Vec3b color = ptr[x];
            cv::Mat scaled = cv::Mat(3, 1,
                                  CV_64FC1,
                                  cv::Scalar(0));

            scaled.at<double>(0) = color[0] / 255.0;
            scaled.at<double>(1) = color[1] / 255.0;
            scaled.at<double>(2) = color[2] / 255.0;

            cv::Mat this_value = eig * scaled;

            if (this_value.at<double>(0, 0) <= comparison_value.at<double>(0, 0)) {
                ptr_class[x] = new_id_left;
            } else {
                ptr_class[x] = new_id_right;
            }
        }
    }
    return;
}

cv::Mat GetQuantizedImage(cv::Mat classes, color_node *root) {
    std::vector<color_node*> leaves = GetLeaves(root);

    const int height = classes.rows;
    const int width = classes.cols;
    cv::Mat ret(height, width, CV_8UC3, cv::Scalar(0));

    for (int y = 0; y < height; y++) {
        char16_t *ptr_class = classes.ptr<char16_t>(y);
        cv::Vec3b *ptr = ret.ptr<cv::Vec3b>(y);
        for (int x = 0; x < width; x++) {
            char16_t pixel_class = ptr_class[x];
            for (unsigned int i = 0; i < leaves.size(); i++) {
                if (leaves[i]->class_id == pixel_class) {
                    ptr[x] = cv::Vec3b(leaves[i]->mean.at<double>(0) * 255,
                                       leaves[i]->mean.at<double>(1) * 255,
                                       leaves[i]->mean.at<double>(2) * 255);
                }
            }
        }
    }

    return ret;
}

cv::Mat GetClassificationImage(cv::Mat classes) {
    const int height = classes.rows;
    const int width = classes.cols;

    const int max_color_count = 12;
    cv::Vec3b *palette = new cv::Vec3b[max_color_count];
    palette[0]  = cv::Vec3b(  0,   0,   0); // black
    palette[1]  = cv::Vec3b(255,   0,   0); // blue
    palette[2]  = cv::Vec3b(  0, 255,   0); // green
    palette[3]  = cv::Vec3b(  0,   0, 255); // red
    palette[4]  = cv::Vec3b(255, 255,   0); // cyan
    palette[5]  = cv::Vec3b(  0, 255, 255); // yellow
    palette[6]  = cv::Vec3b(255,   0, 255); // magenta
    palette[7]  = cv::Vec3b(128, 128, 128); // gray
    palette[8]  = cv::Vec3b(128, 255, 128); // darker green
    palette[9]  = cv::Vec3b( 64,  64,  64); // darker gray
    palette[10] = cv::Vec3b(255, 128, 128); // darker blue
    palette[11] = cv::Vec3b(128, 128, 255); // darker red

    cv::Mat ret = cv::Mat(height, width, CV_8UC3, cv::Scalar(0, 0, 0));
    for (int y = 0; y < height; y++) {
        cv::Vec3b *ptr = ret.ptr<cv::Vec3b>(y);
        char16_t *ptr_class = classes.ptr<char16_t>(y);
        for (int x = 0;x < width; x++) {
            int color = ptr_class[x];
            ptr[x] = palette[color];
        }
    }

    return ret;
}

color_node* GetMaxEigenValueNode(color_node *current) {
    double max_eigen = -1;
    cv::Mat eigen_values, eigen_vectors;

    std::queue<color_node*> queue;
    queue.push(current);

    color_node *ret = current;
    if (!current->left && !current->right)
        return current;

    while (queue.size() > 0) {
        color_node *node = queue.front();
        queue.pop();

        if (node->left && node->right) {
            queue.push(node->left);
            queue.push(node->right);
            continue;
        }

        cv::eigen(node->cov, eigen_values, eigen_vectors);
        double val = eigen_values.at<double>(0);
        if (val > max_eigen) {
            max_eigen = val;
            ret = node;
        }
    }

    return ret;
}

std::vector<cv::Vec3b> DominantColorsEigen(const cv::Mat &img, const int &nb_colors, cv::Mat *quantized)
{
    const int width = img.cols;
    const int height = img.rows;

    cv::Mat classes = cv::Mat(height, width, CV_16UC1, cv::Scalar(1));
    color_node *root = new color_node();

    root->class_id = 1;
    root->left = NULL;
    root->right = NULL;

    color_node *next = root;
    GetClassMeanCov(img, classes, root);

    for (int i = 0; i < nb_colors - 1; i++) {
        next = GetMaxEigenValueNode(root);
        PartitionClass(img, classes, GetNextClassId(root), next);
        GetClassMeanCov(img, classes, next->left);
        GetClassMeanCov(img, classes, next->right);
    }

    std::vector<cv::Vec3b> colors = GetDominantColors(root);

    *quantized = GetQuantizedImage(classes, root);
    //*classification = GetClassificationImage(classes);

    return colors;
}

/////////////////// K_means algorithm //////////////////////

cv::Mat DominantColorsKMeans(const cv::Mat &source, const int &nb_clusters, cv::Mat1f *dominant_colors)
{
    const unsigned int data_size = source.rows * source.cols; // size of source
    cv::Mat data = source.reshape(1, data_size); // reshape the source to a single line
    data.convertTo(data, CV_32F); // floats needed by K-means

    std::vector<int> indices; // color clusters
    cv::Mat1f colors; // colors output
    cv::kmeans(data, nb_clusters, indices, cv::TermCriteria(cv::TermCriteria::EPS+cv::TermCriteria::COUNT, 100, 1.0),
               100, cv::KMEANS_PP_CENTERS, colors); // ending criterias : 100 iterations and epsilon=1.0

    for (unsigned int i = 0 ; i < data_size ; i++ ) { // replace colors in image data
        data.at<float>(i, 0) = colors(indices[i], 0);
        data.at<float>(i, 1) = colors(indices[i], 1);
        data.at<float>(i, 2) = colors(indices[i], 2);
    }

    cv::Mat output_image = data.reshape(3, source.rows); // RGB channels needed for output
    output_image.convertTo(output_image, CV_8UC3); // BGR image

    *dominant_colors = colors; // save colors clusters

    return output_image; // return quantized image
}
