/*#-------------------------------------------------
#
#     Dominant colors library with openCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.4 - 2022/12/12
#
#   - all in OKLAB color space
#   - sectored means (my own) algorithm
#   - eigen vectors algorithm
#   - K-means algorithm
#
#-------------------------------------------------*/


#include "dominant-colors.h"


///////////////////////////////////////////////
////         Sectored-Means algorithm
///////////////////////////////////////////////
// Original algorithm by AbsurdePhoton

int WhichColorSector(const double &H) // get the color sector of a given Hue in HSL (H in degrees)
{
    if (H == -360.0) // is a gray value ?
        return 0;

    //double h = H % 360.0; // Hue in [0..359]
    double h = H;
    if (h < 8) // reds come across the color circle "origin" (0) from 360- to 0+
        h += 360.0; // get a continuous range for red category

    int hPrime; // value to return
    for (hPrime = 1; hPrime < nb_color_sectors; hPrime++) // parse Hue ranges
        if ((h >= color_sectorsOKLAB[hPrime].begin) and (h <= color_sectorsOKLAB[hPrime].end)) // is this the right range ?
            break; // yes !

    if (hPrime == nb_color_sectors) // didn't find a range ?
        hPrime = -1; // dummy value (should never happen)

    return hPrime; // return color sector
}

int WhichLightnessCategory(const double &L) // get the Lightness category (L from OKLAB)
{
    //double max = color_sectors[colorSector].maxLightness; // get highest Chroma value for current color category
    //double Lrectified = L / max * 100.0; // ligthness in [0..100]
    double Lrectified = L; // ligthness in [0..1]
    if (Lrectified > 1.0) // lightness value is clipped to 100 if needed
        Lrectified = 1.0;

    int l;
    for (l = 0; l < nb_lightness_categories; l++) // parse Lightness ranges
        if ((Lrectified >= lightness_categories[l].begin) and (Lrectified < lightness_categories[l].end)) // is this the right range ?
            break; // yes !

    if (l == nb_lightness_categories) // didn't find a range ?
        l = -1; // dummy value (should never happen)

    return l; // return Lightness catgory
}

int WhichChromaCategory(const double &C, const int &colorSector) // get the Chroma category (C from CIE LChab)
{
    if (colorSector == 0)
        return 0;

    double max = color_sectorsOKLAB[colorSector].maxChroma; // get highest Chroma value for current color category
    //double Crectified = C / max * 100.0; // chroma in [0..100]
    double Crectified = C / max; // chroma in [0..1]
    if (Crectified > 1.0) // chroma value is clipped to 1.0 if needed
        Crectified = 1.0;

    int c;
    for (c = 0; c < nb_chroma_categories; c++) // parse Chroma ranges
        if ((Crectified >= chroma_categories[c].begin) and (Crectified < chroma_categories[c].end)) // is this the right range ?
            break; // yes !

    if (c == nb_chroma_categories) // didn't find a range ?
        c = -1; // dummy value (should never happen)

    return c; // return Chroma category
}

int WhichSaturationCategory(const double &S, const int &colorSector) // get the Saturation category (S from HSL)
{
    if (colorSector == 0)
        return 0;

    double max = color_sectorsOKLAB[colorSector].maxSaturation; // get highest Saturation value for current color category
    //double Crectified = C / max * 100.0; // chroma in [0..100]
    double Srectified = S / max; // saturation in [0..1]
    if (Srectified > 1.0) // chroma value is clipped to 1.0 if needed
        Srectified = 1.0;

    int s;
    for (s = 0; s < nb_chroma_categories; s++) // parse Saturation ranges
        if ((Srectified >= chroma_categories[s].begin) and (Srectified < chroma_categories[s].end)) // is this the right range ?
            break; // yes !

    if (s == nb_chroma_categories) // didn't find a range ?
        s = -1; // dummy value (should never happen)

    return s; // return Chroma category
}

int Hash3Bytes(const uchar &B1, const uchar &B2, const uchar &B3) // concatenate 3 bytes in one integer value
{
    return (B1 << 16) | (B2 << 8) | (B3); // shifting is ultra-fast
}

void DeHash3Bytes(const int &hash, uchar &B1, uchar &B2, uchar &B3) // de-concatenate an integer's 3 lower bytes to individual values
{
    B1 = hash >> 16 & 255; // right-shift and mask to get individual bytes
    B2 = hash >> 8 & 255;
    B3 = hash & 255;
}

std::vector<std::vector<int>> SectoredMeansSegmentation(const cv::Mat &image, cv::Mat &quantized) // BGR image segmentation by color sector mean (H from HSL)
    // returns a palette that contains 7 values : R/G/B + pixels count + S/L/C
{
    cv::Mat sectors = cv::Mat::zeros(image.rows, image.cols, CV_32SC1); // used to store a hash of s/l/c values (i.e. Hue, Lightness and Chroma)
    cv::Mat imageLAB = cv::Mat::zeros(image.rows, image.cols, CV_64FC3); // to store converted image to OKLAB

    // compute matrix of sectors for each pixel
    double Hhsl, S, L, C, Hlab, R, G, B, a, b; // values for color spaces conversions
    for (int y = 0; y < image.rows; y++) { // parse image rows
        const cv::Vec3b* imageP = image.ptr<cv::Vec3b>(y); // pointers to images
        cv::Vec3d* imageLABp = imageLAB.ptr<cv::Vec3d>(y);
        int* sectorsP = sectors.ptr<int>(y);
        for (int x = 0; x < image.cols; x++) { // parse image columns
            //RGBtoHSL(imageP[x][2] / 255.0, imageP[x][1] / 255.0, imageP[x][0] / 255.0, Hhsl, S, L, C);
            //RGBtoOKLCH(imageP[x][2], imageP[x][1], imageP[x][0], L, C, Hlab);
            //OKLABtoOKLCH(a, b, C, Hlab);
            OKLABHSLChfromRGB(imageP[x][2], imageP[x][1], imageP[x][0], Hhsl, S, L, C, Hlab, a, b); // get OKLAB + values from RGB pixel - Hhsl = -1 if color is a gray

            imageLABp[x] = cv::Vec3d(L, a, b); // store OKLAB pixel

            int s = WhichColorSector(Hhsl * 360.0); // get sectors from Hhsl (HSL Hue), C (OKLAB Chroma) and L (OKLAB Lightness) categories for current pixel
            int l = WhichLightnessCategory(L);
            //int c = WhichChromaCategory(C, s);
            int c = WhichSaturationCategory(S, s);

            int hash = Hash3Bytes(s, l, c); // the trick to store the 3 categories in one value is to hash them
            sectorsP[x] = hash; // store pixel "sector" in sector mask
        }
    }

    std::vector<std::vector<int>> palette; // used to return function value : it contains 7 values : R/G/B + pixels count + S/L/C
    palette.reserve(nb_color_sectors * nb_lightness_categories * nb_chroma_categories * 7); // reserve space for pointer values
    quantized = cv::Mat::zeros(image.rows, image.cols, CV_8UC3); // init quantized image = black - this is the image output

    for (int s = 0; s < nb_color_sectors; s++) { // for each category
        for (int l = 0; l < nb_lightness_categories; l++) {
            for (int c = 0; c < nb_chroma_categories; c++) {
                int hash = Hash3Bytes(s, l, c); // hash for this sector
                cv::Mat mask = (sectors == hash); // get mask from sector using the hash value -> we get all pixels for this sector

                int count = cv::countNonZero(mask); // count sector's pixels
                if (count > 0) { // does sector mask contain values ?
                    cv::Scalar mean = cv::mean(imageLAB, mask); // compute mean color of entire sector from OKLAB image using sector mask
                    OKLABtoRGB(mean[0], mean[1], mean[2], R, G, B); // convert mean OKLAB color to RGB

                    std::vector<int> paletteTemp; // "palette" for this sector
                    paletteTemp.push_back(round(R * 255.0)); // store RGB values
                    paletteTemp.push_back(round(G * 255.0));
                    paletteTemp.push_back(round(B * 255.0));
                    paletteTemp.push_back(count); // store pixel count (useful for an histogram)
                    paletteTemp.push_back(s); // store sector categories
                    paletteTemp.push_back(l);
                    paletteTemp.push_back(c);
                    palette.push_back(paletteTemp); // store this sector info in global palette

                    quantized.setTo(cv::Vec3b(paletteTemp[2], paletteTemp[1], paletteTemp[0]), mask); // plot RGB mean color to quantized image
                }
            }
        }
    }

    return palette;
}

void DrawSectoredMeansPalettesCIELab() // save Sectored Means palettes to images : scales are computed, values come from pre-defined RGB colors - use as reference
{
    // draw lightness scale
    cv::Mat lightness = cv::Mat::zeros(500, nb_lightness_categories * 100, CV_8UC3);
    for (int i = 0; i < nb_lightness_categories; i++) {
        double R, G, B;
        CIELabToRGB((lightness_categories[i].end - lightness_categories[i].begin) / 2 + lightness_categories[i].begin, 0, 0, R, G, B);
        cv::Vec3b color(round(B * 255.0), round(G * 255.0), round(R * 255.0));
        cv::rectangle(lightness, cv::Rect(i * 100, 0, 100, 500), color, -1);
    }
    cv::rectangle(lightness, cv::Rect(0, 0, nb_lightness_categories * 100, 500), cv::Vec3b(64, 64, 64), 1);
    cv::imwrite("lightness-scale.png", lightness);

    cv::Mat chroma = cv::Mat::zeros(500, nb_chroma_categories * 100, CV_8UC3);
    for (int i = 0; i < nb_chroma_categories; i++) {
        double R, G, B;
        CIELCHabToRGB(0.75, (chroma_categories[i].end - chroma_categories[i].begin) / 2 + chroma_categories[i].begin, 240 / 360.0, R, G, B);
        cv::Vec3b color(round(B * 255.0), round(G * 255.0), round(R * 255.0));
        cv::rectangle(chroma, cv::Rect(i * 100, 0, 100, 500), color, -1);
    }
    cv::rectangle(chroma, cv::Rect(0, 0, nb_chroma_categories * 100, 500), cv::Vec3b(64, 64, 64), 1);
    cv::imwrite("chroma-scale.png", chroma);

    // draw color scale
    cv::Mat colors = cv::Mat::zeros(500, nb_color_sectors * 100, CV_8UC3);
    for (int i = 1; i < nb_color_sectors; i++) {
        double R, G, B;
        HSLtoRGB(((color_sectorsCIELAB[i].end - color_sectorsCIELAB[i].begin) / 2 + color_sectorsCIELAB[i].begin) / 360.0, 1.0, 0.5, R, G, B);
        cv::Vec3b color(round(B * 255.0), round(G * 255.0), round(R * 255.0));
        cv::rectangle(colors, cv::Rect(i * 100, 0, 100, 500), color, -1);
    }
    cv::rectangle(colors, cv::Rect(0, 0, nb_color_sectors * 100, 500), cv::Vec3b(64, 64, 64), 1);
    cv::imwrite("colors-scale.png", colors);

    // draw lightness values
    cv::Mat lightness_values = cv::Mat::zeros(500, nb_lightness_categories * 100, CV_8UC3);
    for (int i = 0; i < nb_lightness_categories; i++) {
        cv::Vec3b color(lightness_categories[i].B, lightness_categories[i].G, lightness_categories[i].R);
        cv::rectangle(lightness_values, cv::Rect(i * 100, 0, 100, 500), color, -1);
    }
    cv::rectangle(lightness_values, cv::Rect(0, 0, nb_lightness_categories * 100, 500), cv::Vec3b(64, 64, 64), 1);
    cv::imwrite("lightness-values.png", lightness_values);

    // draw chroma values
    cv::Mat chroma_values = cv::Mat::zeros(500, nb_chroma_categories * 100, CV_8UC3);
    for (int i = 0; i < nb_chroma_categories; i++) {
        cv::Vec3b color(chroma_categories[i].B, chroma_categories[i].G, chroma_categories[i].R);
        cv::rectangle(chroma_values, cv::Rect(i * 100, 0, 100, 500), color, -1);
    }
    cv::rectangle(chroma_values, cv::Rect(0, 0, nb_chroma_categories * 100, 500), cv::Vec3b(64, 64, 64), 1);
    cv::imwrite("chroma-values.png", chroma_values);

    // draw color values
    cv::Mat colors_values = cv::Mat::zeros(500, nb_color_sectors * 100, CV_8UC3);
    for (int i = 0; i < nb_color_sectors; i++) {
        cv::Vec3b color(color_sectorsCIELAB[i].B, color_sectorsCIELAB[i].G, color_sectorsCIELAB[i].R);
        cv::rectangle(colors_values, cv::Rect(i * 100, 0, 100, 500), color, -1);
    }
    cv::rectangle(colors_values, cv::Rect(0, 0, nb_color_sectors * 100, 500), cv::Vec3b(64, 64, 64), 1);
    cv::imwrite("colors-values.png", colors_values);
}

void FindSectorsMaxValuesCIELab(const int &intervals, const std::string filename) // write max values (C, S, L) for each color sector (CIELab)
    // only useful once : to determine S, L, C values in sectors
{
    std::ofstream saveCSV; // file to save
    saveCSV.open(filename + ".csv"); // save data file
    if (saveCSV) { // if successfully open
        saveCSV << "ColorSector;Hmin;Hmax;Cmax;Lmax;Smax\n"; //header

        for (int colorSector = 1; colorSector < nb_color_sectors; colorSector++) {
            double Smax = 0;
            double Cmax = 0;
            double Lmax = 0;
            double h = color_sectorsCIELAB[colorSector].begin;
            while (h <= color_sectorsCIELAB[colorSector].end) { // for this hue
                for (int hL = 1; hL <= intervals; hL++) { // for each HSL L value
                    //for (int hS = 1; hS <= intervals; hS++) { // for each HSL S value*/
                        double r, g, b;
                        double hPrime = h;
                        if (hPrime > 360)
                            hPrime -= 360;
                        hPrime /= 360.0;
                        HSLtoRGB(hPrime, 1.0, hL / double(intervals), r, g, b);
                        double L, A, B;
                        RGBtoCIELab(r, g, b, L, A, B);
                        double C, H;
                        CIELabToCIELCHab(A, B, C, H);
                        double S;

                        /*if (L == 0.0) { // black is a particular value
                            S = -1.0; // no saturation
                            C = -1.0; // ... and no chroma
                        }
                        else if (C == 0)
                            S = 0;
                        else // if not black and has chroma > 0*/
                            S = C / sqrt(C * C + L * L); // saturation S from LCHab - it's a distance !
                        /*if (S > 1.0) // clip saturation ?
                            S = 1.0;*/

                        if (C > Cmax)
                            Cmax = C;
                        if (S > Smax)
                            Smax = S;
                        if (L > Lmax)
                            Lmax = L;
                //    }
                }

                h += 1.0 / intervals;
            }

            saveCSV << colorSector << ";" << color_sectorsCIELAB[colorSector].begin << ";" << color_sectorsCIELAB[colorSector].end << ";" << Cmax << ";" << Lmax << ";" << Smax << "\n"; // write result to file
        }

        saveCSV.close(); // close text file
    }
}

void FindSectorsMaxValuesOKLAB(const int &intervals, const std::string filename) // write max values (C, S, L) for each color sector (OKLAB)
    // only useful once : to determine S, L, C values in sectors
{
    std::ofstream saveCSV; // file to save
    saveCSV.open(filename + ".csv"); // save data file
    if (saveCSV) { // if successfully open
        saveCSV << "ColorSector;Hmin;Hmax;Cmax;Lmax;Smax\n"; //header

        for (int colorSector = 1; colorSector < nb_color_sectors; colorSector++) {
            double Smax = 0;
            double Cmax = 0;
            double Lmax = 0;
            double h = color_sectorsOKLAB[colorSector].begin;
            while (h <= color_sectorsOKLAB[colorSector].end) { // for this hue
                for (int hL = 1; hL <= intervals; hL++) { // for each HSL L value
                    //for (int hS = 1; hS <= intervals; hS++) { // for each HSL S value*/
                        double r, g, b;
                        double hPrime = h;
                        if (hPrime > 360)
                            hPrime -= 360;
                        hPrime /= 360.0;
                        HSLtoRGB(hPrime, 1.0, hL / double(intervals), r, g, b);
                        double L, A, B;
                        RGBtoOKLAB(r, g, b, L, A, B);
                        double C, H;
                        OKLABtoOKLCH(A, B, C, H);
                        double S;

                        /*if (L == 0.0) { // black is a particular value
                            S = -1.0; // no saturation
                            C = -1.0; // ... and no chroma
                        }
                        else if (C == 0)
                            S = 0;
                        else // if not black and has chroma > 0*/
                            S = C / sqrt(C * C + L * L); // saturation S from LCHab - it's a distance !
                        /*if (S > 1.0) // clip saturation ?
                            S = 1.0;*/

                        if (C > Cmax)
                            Cmax = C;
                        if (S > Smax)
                            Smax = S;
                        if (L > Lmax)
                            Lmax = L;
                //    }
                }

                h += 1.0 / intervals;
            }

            saveCSV << colorSector << ";" << color_sectorsOKLAB[colorSector].begin << ";" << color_sectorsOKLAB[colorSector].end << ";" << Cmax << ";" << Lmax << ";" << Smax << "\n"; // write result to file
        }

        saveCSV.close(); // close text file
    }
}

////////////////////////////////////////////////////////////
////                Eigen vectors algorithm
////////////////////////////////////////////////////////////

// code adapted from Utkarsh Sinha, no more 256 colors limit by using int for "class id"
// source : http://aishack.in/tutorials/dominant-color/
// works for any color space, because values are in range [0..1]
// only implemented here for CIELab & OKLAB

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

std::vector<cv::Vec3d> GetDominantColors(color_node *root)
{
    std::vector<color_node*> leaves = GetLeaves(root);
    std::vector<cv::Vec3d> ret;

    for (unsigned int i = 0; i < leaves.size(); i++) {
        cv::Mat mean = leaves[i]->mean;
        ret.push_back(cv::Vec3d(mean.at<double>(0),
                                mean.at<double>(1),
                                mean.at<double>(2)));
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

    cv::Mat mean = cv::Mat::zeros(3, 1, CV_64FC1);
    cv::Mat cov = cv::Mat::zeros(3, 3, CV_64FC1);

    // start out with the average color
    double pix_count = 0;
    for (int y = 0; y < height; y++) {
        cv::Vec3d* ptr = img.ptr<cv::Vec3d>(y);
        char16_t* ptrClass = classes.ptr<char16_t>(y);
        for (int x=0; x < width; x++) {
            if (ptrClass[x] != class_id)
                continue;

            cv::Vec3d color = ptr[x];
            cv::Mat scaled = cv::Mat::zeros(3, 1, CV_64FC1);
            scaled.at<double>(0) = color[0];
            scaled.at<double>(1) = color[1];
            scaled.at<double>(2) = color[2];

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
        cv::Vec3d* ptr = img.ptr<cv::Vec3d>(y);
        char16_t* ptr_class = classes.ptr<char16_t>(y);
        for (int x = 0; x < width; x++) {
            if (ptr_class[x] != class_id)
                continue;

            cv::Vec3d color = ptr[x];
            cv::Mat scaled = cv::Mat(3, 1,
                                  CV_64FC1,
                                  cv::Scalar(0));

            scaled.at<double>(0) = color[0];
            scaled.at<double>(1) = color[1];
            scaled.at<double>(2) = color[2];

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
    cv::Mat ret(height, width, CV_64FC3, cv::Scalar(0));

    for (int y = 0; y < height; y++) {
        char16_t *ptr_class = classes.ptr<char16_t>(y);
        cv::Vec3d *ptr = ret.ptr<cv::Vec3d>(y);
        for (int x = 0; x < width; x++) {
            char16_t pixel_class = ptr_class[x];
            for (unsigned int i = 0; i < leaves.size(); i++) {
                if (leaves[i]->class_id == pixel_class) {
                    ptr[x] = cv::Vec3d(leaves[i]->mean.at<double>(0),
                                       leaves[i]->mean.at<double>(1),
                                       leaves[i]->mean.at<double>(2));
                }
            }
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

std::vector<cv::Vec3d> DominantColorsEigen(const cv::Mat &img, const int &nb_colors, cv::Mat &quantized) // Eigen algorithm with CIELab or OKLAB values in range [0..1]
    // input and ouput images in CIELab or OKLAB values of range [0..1]
    // returns a list of dominant colors in values of range [0..1]
{
    // particular cases are all white or all black image
    cv::Scalar mean = cv::mean(img);
    if (mean[0] > 0.99999999)
        mean[0] = 1.0;
    if (mean[1] < 0.000001)
        mean[1] = 0;
    if (mean[2] < 0.000001)
        mean[2] = 0;

    if ((mean == cv::Scalar(1.0, 0, 0)) or (mean == cv::Scalar(0.0, 0.0, 0.0))) {
        cv::Vec3d result;
        quantized = cv::Mat::zeros(img.rows, img.cols, CV_64FC3);
        if (mean == cv::Scalar(1.0, 0, 0)) {
            result = cv::Vec3d(1.0, 0, 0);
            quantized.setTo(result);
        }
        else {
            result = cv::Vec3d(0, 0, 0);
        }

        std::vector<cv::Vec3d> colors;
        for (int n = 0; n < nb_colors; n++)
            colors.push_back(result);

        return colors;
    }

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

    std::vector<cv::Vec3d> colors = GetDominantColors(root);
    quantized = GetQuantizedImage(classes, root); // the quantized image has values in range [0..1]

    delete(root);

    return colors;
}

////////////////////////////////////////////////////////////
////                K_means algorithm
////////////////////////////////////////////////////////////

cv::Mat DominantColorsKMeansRGB_U(const cv::Mat &source, const int &nb_clusters, cv::Mat1f &dominant_colors) // Dominant colors with K-means from RGB image using UMat
{
    const unsigned int data_size = source.rows * source.cols; // size of source
    cv:: UMat sourceU = source.getUMat(cv::ACCESS_READ);
    cv::UMat data = sourceU.reshape(1, data_size); // reshape the source to a single line
    data.convertTo(data, CV_32F); // floats needed by K-means

    std::vector<int> indices; // color clusters
    cv::Mat1f colors; // colors output
    cv::kmeans(data, nb_clusters, indices, cv::TermCriteria(cv::TermCriteria::EPS+cv::TermCriteria::COUNT, 100, 1.0),
               100, cv::KMEANS_PP_CENTERS, colors); // ending criterias : 100 iterations and epsilon=1.0

    cv::Mat data_res = data.getMat(cv::ACCESS_RW);
    for (unsigned int i = 0 ; i < data_size ; i++ ) { // replace colors in image data
        float* dataP = data_res.ptr<float>(i);
        dataP[0] = colors(indices[i], 0);
        dataP[1] = colors(indices[i], 1);
        dataP[2] = colors(indices[i], 2);
    }

    cv::Mat output_image = data_res.reshape(3, source.rows); // RGB channels needed for output
    output_image.convertTo(output_image, CV_8UC3); // BGR image

    dominant_colors = colors; // save colors clusters

    return output_image; // return quantized image
}

cv::Mat DominantColorsKMeansRGB(const cv::Mat &source, const int &nb_clusters, cv::Mat1f &dominant_colors) // Dominant colors with K-means from RGB image
{
    const unsigned int data_size = source.rows * source.cols; // size of source
    cv::Mat data = source.reshape(1, data_size); // reshape the source to a single line
    data.convertTo(data, CV_32F); // floats needed by K-means

    std::vector<int> indices; // color clusters
    cv::Mat1f colors; // colors output
    cv::kmeans(data, nb_clusters, indices, cv::TermCriteria(cv::TermCriteria::EPS+cv::TermCriteria::COUNT, 100, 1.0),
               100, cv::KMEANS_PP_CENTERS, colors); // ending criterias : 100 iterations and epsilon=1.0

    for (unsigned int i = 0 ; i < data_size ; i++ ) { // replace colors in image data
        float* dataP = data.ptr<float>(i);
        dataP[0] = colors(indices[i], 0);
        dataP[1] = colors(indices[i], 1);
        dataP[2] = colors(indices[i], 2);
    }

    cv::Mat output_image = data.reshape(3, source.rows); // RGB channels needed for output
    output_image.convertTo(output_image, CV_8UC3); // BGR image

    dominant_colors = colors; // save colors clusters

    return output_image; // return quantized image
}

cv::Mat DominantColorsKMeans(const cv::Mat &source, const int &nb_clusters, cv::Mat1f &dominant_colors) // Dominant colors with K-means in CIELAB or OKLAB space
    // source must be a CIELab or OKLAB image of type CV_64FC3
    // output is of same type : CV_64FC3
{
    cv::Mat temp;
    source.convertTo(temp, CV_32FC3); // source type is CV_64FC3 ! k-means function is not optimized anymore

    const unsigned int data_size = source.rows * source.cols; // size of source
    cv::Mat1f data = temp.reshape(1, data_size); // reshape data to a single line

    std::vector<int> indices; // color clusters
    cv::Mat1f colors; // colors output
    cv::kmeans(data, nb_clusters, indices, cv::TermCriteria(cv::TermCriteria::EPS+cv::TermCriteria::COUNT, 100, 1.0),
               100, cv::KMEANS_PP_CENTERS, colors); // k-means on data, ending criterias : 100 iterations and epsilon=1.0

    for (unsigned int i = 0 ; i < data_size ; i++ ) { // replace colors in data
        float* dataP = data.ptr<float>(i);
        dataP[0] = colors(indices[i], 0);
        dataP[1] = colors(indices[i], 1);
        dataP[2] = colors(indices[i], 2);
    }

    cv::Mat output_image = data.reshape(3, source.rows); // 3 channels needed for output
    output_image.convertTo(output_image, CV_64FC3); // same type as output

    dominant_colors = colors; // save colors clusters in CIELab or OKLAB color space (all values in range [0..1])

    return output_image; // return quantized image
}

////////////////////////////////////////////////////////////
////                  Mean-Shift algorithm
////////////////////////////////////////////////////////////

// adpated from Bingyang Liu to directly work in CIELab or OKLAB color space
// source : https://github.com/bbbbyang/Mean-Shift-Segmentation

// Definitions
#define MS_MAX_NUM_CONVERGENCE_STEPS	5										// up to 10 steps are for convergence
#define MS_MEAN_SHIFT_TOL_COLOR			0.3										// minimum mean color shift change
#define MS_MEAN_SHIFT_TOL_SPATIAL		0.3										// minimum mean spatial shift change
const int dxdy[][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};	// region growing

Point5D::Point5D() // Constructor
{
    x = -1;
    y = -1;
}

Point5D::~Point5D() // Destructor
{
}

void Point5D::MSPoint5DAccum(const Point5D &Pt) // Accumulate points
{
    x += Pt.x;
    y += Pt.y;
    l += Pt.l;
    a += Pt.a;
    b += Pt.b;
}

void Point5D::MSPoint5DCopy(const Point5D &Pt) // Copy a point
{
    x = Pt.x;
    y = Pt.y;
    l = Pt.l;
    a = Pt.a;
    b = Pt.b;
}

double Point5D::MSPoint5DColorDistance(const Point5D &Pt) // Color space distance between two points
{
    return sqrt((l - Pt.l) * 100.0 * (l - Pt.l) * 100.0 + (a - Pt.a) * 127.0 * (a - Pt.a) * 127.0 + (b - Pt.b) * 127.0 * (b - Pt.b) * 127.0); // CIE76 color difference - not very good but fast
    //return DistanceCIEDE2000LAB(Pt.l, Pt.a, Pt.b, l, a, b); // takes too much time
    //return DistanceCIEAdaptive(Pt.l, Pt.a, Pt.b, l, a, b, 2.0, 1.0, 1.0, 15); // even with adaptive version it is too slow
}

double Point5D::MSPoint5DSpatialDistance(const Point5D &Pt) // Spatial space distance between two points
{
    return sqrt((x - Pt.x) * (x - Pt.x) + (y - Pt.y) * (y - Pt.y)); // euclidian distance
}

void Point5D::MSPoint5DScale(const double scale) // Scale point
{
    x *= scale;
    y *= scale;
    l *= scale;
    a *= scale;
    b *= scale;
}

void Point5D::MSPOint5DSet(const double &px, const double &py, const double &pl, const double &pa, const double &pb) // Set point value
{
    x = px;
    y = py;
    l = pl;
    a = pa;
    b = pb;
}

MeanShift::MeanShift(const double &s, const double &r) // Constructor for spatial bandwidth and color bandwidth
{
    hs = s;
    hr = r;
}

void MeanShift::MeanShiftFiltering(cv::Mat &img) // Mean Shift Filtering
    // image must be CV_64FC3 (CIELab or OKLab)
{
    int ROWS = img.rows;			// Get row number
    int COLS = img.cols;			// Get column number
    split(img, IMGChannels);		// Split Lab color

    Point5D PtCur;					// Current point
    Point5D PtPrev;					// Previous point
    Point5D PtSum;					// Sum vector of the shift vector
    Point5D Pt;
    int Left;						// Left boundary
    int Right;						// Right boundary
    int Top;						// Top boundary
    int Bottom;						// Bottom boundary
    int NumPts;						// number of points in a hypersphere
    int step;

    for(int i = 0; i < ROWS; i++) {
        double* IMGChannels0P = IMGChannels[0].ptr<double>(i);
        double* IMGChannels1P = IMGChannels[1].ptr<double>(i);
        double* IMGChannels2P = IMGChannels[2].ptr<double>(i);
        cv::Vec3d* imgP = img.ptr<cv::Vec3d>(i);
        for(int j = 0; j < COLS; j++) {
            Left = (j - hs) > 0 ? (j - hs) : 0;						// Get Left boundary of the filter
            Right = (j + hs) < COLS ? (j + hs) : COLS;				// Get Right boundary of the filter
            Top = (i - hs) > 0 ? (i - hs) : 0;						// Get Top boundary of the filter
            Bottom = (i + hs) < ROWS ? (i + hs) : ROWS;				// Get Bottom boundary of the filter
            PtCur.MSPOint5DSet(i, j, IMGChannels0P[j], IMGChannels1P[j], IMGChannels2P[j]); // Set current point
            step = 0;				// count the times
            do {
                PtPrev.MSPoint5DCopy(PtCur);						// Set the original point and previous one
                PtSum.MSPOint5DSet(0, 0, 0, 0, 0);					// Initial Sum vector
                NumPts = 0;											// Count number of points that satisfy the bandwidths
                for(int hx = Top; hx < Bottom; hx++) {
                    double* IMGChannels0P = IMGChannels[0].ptr<double>(hx);
                    double* IMGChannels1P = IMGChannels[1].ptr<double>(hx);
                    double* IMGChannels2P = IMGChannels[2].ptr<double>(hx);
                    for(int hy = Left; hy < Right; hy++) {
                        Pt.MSPOint5DSet(hx, hy, IMGChannels0P[hy], IMGChannels1P[hy], IMGChannels2P[hy]); // Set point in the spatial bandwidth
                        if (Pt.MSPoint5DColorDistance(PtCur) < hr) { // Check it satisfied color bandwidth or not
                            PtSum.MSPoint5DAccum(Pt);				// Accumulate the point to Sum vector
                            NumPts++;								// Count
                        }
                    }
                }
                PtSum.MSPoint5DScale(1.0 / NumPts);					// Scale Sum vector to average vector
                PtCur.MSPoint5DCopy(PtSum);							// Get new origin point
                step++;												// One time end
            } while((PtCur.MSPoint5DColorDistance(PtPrev) > MS_MEAN_SHIFT_TOL_COLOR) && (PtCur.MSPoint5DSpatialDistance(PtPrev) > MS_MEAN_SHIFT_TOL_SPATIAL)
                        && (step < MS_MAX_NUM_CONVERGENCE_STEPS)); // filter iteration to end

            imgP[j] = cv::Vec3d(PtCur.l, PtCur.a, PtCur.b); // Copy result to image
        }
    }
}

void MeanShift::MeanShiftSegmentation(cv::Mat &img) // Mean Shift Segmentation
    // image must be CV_64FC3 (CIELab or OKLab)
{
    int ROWS = img.rows;			// Get row number
    int COLS = img.cols;			// Get column number

    Point5D PtCur;                  // Current point
    Point5D Pt;

    int label = -1;					// Label number
    double *Mode = new double [ROWS * COLS * 3];					// Store the Lab color of each region
    int *MemberModeCount = new int [ROWS * COLS];				// Store the number of each region
    memset(MemberModeCount, 0, ROWS * COLS * sizeof(int));		// Initialize the MemberModeCount
    split(img, IMGChannels); // split image

    // Label for each point
    int **Labels = new int *[ROWS];
    for(int i = 0; i < ROWS; i++)
        Labels[i] = new int [COLS];

    // Initialization
    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            Labels[i][j] = -1;
        }
    }

    for(int i = 0; i < ROWS; i++) {
        double* IMGChannels0P = IMGChannels[0].ptr<double>(i);
        double* IMGChannels1P = IMGChannels[1].ptr<double>(i);
        double* IMGChannels2P = IMGChannels[2].ptr<double>(i);
        for(int j = 0; j < COLS; j ++) {
            if (Labels[i][j] < 0) { // If the point is not being labeled
                Labels[i][j] = ++label;		// Give it a new label number
                PtCur.MSPOint5DSet(i, j, IMGChannels0P[j], IMGChannels1P[j], IMGChannels2P[j]); // Get the point

                // Store each value of Lab
                Mode[label * 3 + 0] = PtCur.l;
                Mode[label * 3 + 1] = PtCur.a;
                Mode[label * 3 + 2] = PtCur.b;

                // Region Growing 8 Neighbours
                std::vector<Point5D> NeighbourPoints;
                NeighbourPoints.push_back(PtCur);
                while(!NeighbourPoints.empty()) {
                    Pt = NeighbourPoints.back();
                    NeighbourPoints.pop_back();

                    // Get 8 neighbours
                    for(int k = 0; k < 8; k++) {
                        int hx = Pt.x + dxdy[k][0];
                        int hy = Pt.y + dxdy[k][1];
                        if ((hx >= 0) && (hy >= 0) && (hx < ROWS) && (hy < COLS) && (Labels[hx][hy] < 0)) {
                            Point5D P;
                            P.MSPOint5DSet(hx, hy, IMGChannels[0].at<double>(hx, hy), IMGChannels[1].at<double>(hx, hy), IMGChannels[2].at<double>(hx, hy));

                            // Check the color
                            if (PtCur.MSPoint5DColorDistance(P) < hr) { // Satisfied the color bandwidth
                                Labels[hx][hy] = label;				// Give the same label
                                NeighbourPoints.push_back(P);		// Push it into stack
                                MemberModeCount[label]++;			// This region number plus one
                                // Sum all color in same region
                                Mode[label * 3 + 0] += P.l;
                                Mode[label * 3 + 1] += P.a;
                                Mode[label * 3 + 2] += P.b;
                            }
                        }
                    }
                }
                MemberModeCount[label]++;							// Count the point itself
                Mode[label * 3 + 0] /= MemberModeCount[label];		// Get average color
                Mode[label * 3 + 1] /= MemberModeCount[label];
                Mode[label * 3 + 2] /= MemberModeCount[label];
            }
        }
    }

    // Get result image from Mode array
    for(int i = 0; i < ROWS; i++) {
        cv::Vec3d* imgP = img.ptr<cv::Vec3d>(i);
        for(int j = 0; j < COLS; j++) {
            label = Labels[i][j];
            double l = Mode[label * 3 + 0];
            double a = Mode[label * 3 + 1];
            double b = Mode[label * 3 + 2];
            Point5D Pixel;
            Pixel.MSPOint5DSet(i, j, l, a, b);
            imgP[j] = cv::Vec3d(Pixel.l, Pixel.a, Pixel.b);
        }
    }

// Clean Memory
    delete[] Mode;
    delete[] MemberModeCount;

    for(int i = 0; i < ROWS; i++)
        delete[] Labels[i];
    delete[] Labels;
}
