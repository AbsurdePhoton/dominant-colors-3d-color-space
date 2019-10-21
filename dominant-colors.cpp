/*#-------------------------------------------------
#
#     Dominant colors library with openCV
#               in 3D color spaces
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1 - 2019/10/21
#
#   - eigen vectors algorithm
#   - K-means algorithm
#   - conversions between color spaces
#
#-------------------------------------------------*/

#include <opencv2/opencv.hpp>

#include "dominant-colors.h"

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
        ret.push_back(cv::Vec3b(mean.at<double>(0) * 255.0f,
                                mean.at<double>(1) * 255.0f,
                                mean.at<double>(2) * 255.0f));
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
    const uchar class_id = node->class_id;

    cv::Mat mean = cv::Mat(3, 1, CV_64FC1, cv::Scalar(0));
    cv::Mat cov = cv::Mat(3, 3, CV_64FC1, cv::Scalar(0));

    // start out with the average color
    double pix_count = 0;
    for (int y = 0; y < height; y++) {
        cv::Vec3b* ptr = img.ptr<cv::Vec3b>(y);
        uchar* ptrClass = classes.ptr<uchar>(y);
        for (int x=0; x < width; x++) {
            if (ptrClass[x] != class_id)
                continue;

            cv::Vec3b color = ptr[x];
            cv::Mat scaled = cv::Mat(3, 1, CV_64FC1, cv::Scalar(0));
            scaled.at<double>(0) = color[0] / 255.0f;
            scaled.at<double>(1) = color[1] / 255.0f;
            scaled.at<double>(2) = color[2] / 255.0f;

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

void PartitionClass(cv::Mat img, cv::Mat classes, uchar nextid, color_node *node) {
    const int width = img.cols;
    const int height = img.rows;
    const int class_id = node->class_id;

    const uchar new_id_left = nextid;
    const uchar new_id_right = nextid + 1;

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
        uchar* ptr_class = classes.ptr<uchar>(y);
        for (int x = 0; x < width; x++) {
            if (ptr_class[x] != class_id)
                continue;

            cv::Vec3b color = ptr[x];
            cv::Mat scaled = cv::Mat(3, 1,
                                  CV_64FC1,
                                  cv::Scalar(0));

            scaled.at<double>(0) = color[0] / 255.0f;
            scaled.at<double>(1) = color[1] / 255.0f;
            scaled.at<double>(2) = color[2] / 255.0f;

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
        uchar *ptr_class = classes.ptr<uchar>(y);
        cv::Vec3b *ptr = ret.ptr<cv::Vec3b>(y);
        for (int x = 0; x < width; x++) {
            uchar pixel_class = ptr_class[x];
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
        uchar *ptr_class = classes.ptr<uchar>(y);
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

    cv::Mat classes = cv::Mat(height, width, CV_8UC1, cv::Scalar(1));
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

/////////////////// Color spaces conversions //////////////////////
//// All values are in range [0..1]
/// Apart LAB : L [0..100], a [-127..127], b [-127..127]

void SpectralColorToRGB(const float &L, float &R, float &G, float &B) // RGB <0,1> <- lambda l <400,700> [nm]
{
    double t;
    R = 0.0;
    G = 0.0; B = 0.0;

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

void RGBtoHSV(const float &R, const float &G, const float &B, float& H, float& S, float &V, float &C) // convert RGB value to HSV+C
{
    float cmax = max(max(R, G), B);    // maximum of RGB
    float cmin = min(min(R, G), B);    // minimum of RGB
    float diff = cmax-cmin;       // diff of cmax and cmin.

    if (diff > 0) { // not particular case of diff = 0 -> find if R G or B is dominant
        if (cmax == R) { // R is dominant
            H = 60 * (fmod(((G - B) / diff), 6)); // compute H
        }
        else if (cmax == G) { // G is dominant
            H = 60 * (((B - R) / diff) + 2); // compute H
        }
        else if (cmax == B) { // B is dominant
            H = 60 * (((R - G) / diff) + 4); // compute H
        }

        if (cmax > 0) { // compute S
            S = diff / cmax;
        }
        else {
            S = 0;
        }

        V = cmax; // compute V
    }
    else { // particular case -> H = red (convention)
        H = 0;
        S = 0;
        V = cmax;
    }

    if (H < 0) { // H must be in [0..360] range
        H += 360;
    }
    if (H >= 360) {
        H -= 360;
    }

    // Final results are in range [0..1]
    H = H / 360.0f; // was in degrees
    C = diff; // chroma
}

void HSVtoRGB(const float &H, const float &S, const float &V, float &R, float &G, float &B) { // convert HSV value to RGB
  float C = V * S; // chroma
  float HPrime = fmod(H / 60.0f, 6); // dominant 6th part of H - H must be in [0..360]
  float X = C * (1 - fabs(fmod(HPrime, 2) - 1));
  float M = V - C;

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

void RGBtoHSL(const float &R, const float &G, const float &B, float &H, float &S, float &L, float &C) // convert RGB value to HSL
{
    float cmax = max(max(R, G), B);    // maximum of RGB
    float cmin = min(min(R, G), B);    // minimum of RGB
    float diff = cmax - cmin;       // diff of cmax and cmin.

    L = (cmax + cmin) / 2.0f;

    if(cmax == cmin) // particular case : color is a gray
    {
        S = 0;
        H = 0;
    }
    else
    {
        if (L < .50) // result depends on Lightness
        {
            S = diff / (cmax + cmin); // compute S
        }
        else
        {
            S = diff / (2 - cmax - cmin); // compute S
        }

        // which is the dominant in R, G, B
        if (cmax == R) // red
        {
            H = (G - B) / diff; // compute H
        }
        if (cmax == G) // green
        {
            H = 2 + (B - R) / diff; // compute H
        }
        if (cmax == B) // blue
        {
            H = 4 + (R - G) / diff; // compute H
        }
    }

    H = H * 60; // H in degrees

    if (H < 0) // H in [0..360]
        H += 360;
    if (H >= 360)
        H -= 360;

    // Final results in range [0..1]
    H = H / 360.0f; // was in degrees
    C = diff; // Chroma
}

float HueToRGB(const float &v1, const float &v2, const float &H) // Convert H to R, G or B value for HSLtoRGB function
{
    float vH = H;

    if (vH < 0) vH += 1; // H must be in range [0..1]
    if (vH > 1) vH -= 1;

    if ((6 * vH) < 1) // which component (R, G, B) to compute ?
        return v1 + (v2 - v1) * 6.0f * vH;
    if ((2 * vH) < 1 )
        return v2;
    if ((3 * vH) < 2 )
        return (v1 + (v2 - v1) * ((2.0f / 3.0f) - vH) * 6.0f);
    return (v1);
}

void HSLtoRGB(const float &H, const float &S, const float &L, float &R, float &G, float &B) // convert HSL to RGB value - H is in degrees
{
    if ( S == 0 ) // color is a gray
    {
        R = L;
        G = L;
        B = L;
    }
    else {
        float var_1, var_2;
        if (L < 0.5) // Result depends on Luminance
            var_2 = L * (1.0f + S);
        else
            var_2 = (L + S) - (S * L);

        var_1 = 2.0f * L - var_2; // first component based on Luminance

        R = HueToRGB(var_1, var_2, H / 360.0f + (1.0f / 3.0f)); // compute R, G, B
        G = HueToRGB(var_1, var_2, H / 360.0f);
        B = HueToRGB(var_1, var_2, H / 360.0f - (1.0f / 3.0f));
    }
}

//// HWB
//// see https://en.wikipedia.org/wiki/HWB_color_model

void HSVtoHWB(const float &h, const float &s, const float &v, float &H, float &W, float &B) // convert HSV to HWB
{
    // calculus is simple ! There is a direct relation
    H = h;
    W = (1.0f - s) * v;
    B = 1.0f - v;
}

void RGBToHWB(const float &r, const float &g, const float &b, float &H, float &W, float &B) // convert RGB to HWB
{
    float h, s, v, c;
    RGBtoHSV(r, g, b, h, s, v, c); // first find HSV
    HSVtoHWB(h, s, v, H, W, B); // then convert HSV to HWB
}

void HWBtoHSV(const float &h, const float &w, const float &b, float &H, float &S, float &V) // convert HWB to HSV
{
    // calculus is simple ! There is a direct relation
    H = h;
    S = 1 - (w / (1.0f - b));
    V = 1 - b;
}

void HWBtoRGB(const float &h, const float &w, const float &b, float &R, float &G, float &B) // convert HWB to RGB
{
    float H, S, V;
    HWBtoHSV(h, w, b, H, S, V); // first convert to HSV
    HSVtoRGB(H, S, V, R, G, B); // then to RGB
}

//// XYZ and LAB
//// See https://en.wikipedia.org/wiki/CIE_1931_color_space
//// see https://fr.wikipedia.org/wiki/CIE_XYZ for XYZ
//// see https://en.wikipedia.org/wiki/CIELAB_color_space for LAB

void RGBtoXYZ(const float &R, const float &G, const float &B, float &X, float &Y, float &Z) // convert RGB value to CIE XYZ
{
    float r, g, b;

    // Gamma correction - conversion to linear space
    if (R > 0.04045)
        r = powf((R + 0.055f) / 1.055f, 2.4f) * 100.0f;
    else
        r = R / 12.92f * 100.0f;
    if (G > 0.04045)
        g = powf((G + 0.055f) / 1.055f, 2.4f) * 100.0f;
    else
        g = G / 12.92f * 100.0f;
    if (B > 0.04045)
        b = powf((B + 0.055f) / 1.055f, 2.4f) * 100.0f;
    else
        b = B / 12.92f * 100.0f;

    // Gammut conversion : Observer= 2° and Illuminant= D65
    X = r * 0.4124f + g * 0.3576f + b * 0.1805f;
    Y = r * 0.2126f + g * 0.7152f + b * 0.0722f;
    Z = r * 0.0193f + g * 0.1192f + b * 0.9505f;

    //  Illuminant D65 with normalization Y = 100
    X = X / 95.047f;         // ref_X =  95.047
    Y = Y / 100.0f;          // ref_Y = 100.000
    Z = Z / 108.883f;        // ref_Z = 108.883
}

void XYZtoLAB(const float &X, const float &Y, const float &Z, float &L, float &A, float &B) // convert CIE XYZ value to CIE LAB
{
    float x = X;
    float y = Y;
    float z = Z;

    if (x > 0.008856) // two-part equation
        x = powf(x, 1.0f / 3.0f);
    else
        x = (7.787f * x) + (16.0f / 116.0f);
    if (y > 0.008856)
        y = powf(y, 1.0f / 3.0f);
    else
        y = (7.787f * y) + (16.0f / 116.0f);
    if (z > 0.008856)
        z = powf(z, 1.0f / 3.0f);
    else
        z = (7.787f * z) + (16.0f / 116.0f);

    L = 116.0f * y - 16; // final result
    A = 500.0f * (x - y);
    B = 200.0f * (y - z);
}

void LABtoXYZ(const float &L, const float &a, const float &b, float &X, float &Y, float &Z) // convert CIE LAB to CIE XYZ
{
    // it is exactly the reverse of XYZ to LAB
    Y = (L + 16.0f) / 116.0f;
    X = a / 500.0f + Y;
    Z = Y - b / 200.0f;

    if (powf(X,3) > 0.008856)
      X = 0.95047f * powf(X,3);
    else
      X = 0.95047f * (X - 16.0f / 116.0f) / 7.787f;

    if (powf(Y,3) > 0.008856)
      Y = 1.0f * powf(Y,3);
    else
      Y = 1.0f * (Y - 16.0f / 116.0f) / 7.787f;

    if (powf(Z,3) > 0.008856)
      Z = 1.08883f * powf(Z,3);
    else
      Z = 1.08883f * (Z - 16.0f / 116.0f) / 7.787f;
}

void XYZtoRGB(const float &X, const float &Y, const float &Z, float &R, float &G, float &B) // convert from XYZ to RGB
{
    // Observer. = 2°, Illuminant = D65
    R = X *  3.2406f + Y * -1.5372f + Z * -0.4986f;
    G = X * -0.9689f + Y *  1.8758f + Z *  0.0415f;
    B = X *  0.0557f + Y * -0.2040f + Z *  1.0570f;

    // Gamma profile
    if (R > 0.0031308)
        R = 1.055f * powf(R, 1.0f/2.4f) - 0.055f;
    else
        R = R * 12.92f;

    if (G > 0.0031308)
        G = 1.055f * powf(G, 1.0f/2.4f) - 0.055f;
    else
        G = G * 12.92f;

    if (B > 0.0031308)
        B = 1.055f * powf(B, 1.0f/2.4f) - 0.055f;
    else
        B = B * 12.92f;
}
