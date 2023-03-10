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

#ifndef DOMINANTCOLORS_H
#define DOMINANTCOLORS_H

#include "opencv2/opencv.hpp"

#include <fstream>

#include "color-spaces.h"

///////////////////////////////////////////////
////         Sectored-Means algorithm
///////////////////////////////////////////////
// Original algorithm by AbsurdePhoton

struct struct_color_sectors { // struct to store color range values
    std::string name; // color name
    int hue; // main color hue (H from HSL)
    int begin, end; // range of hue
    int R, G, B; // equivalent in RGB with max saturation
    double maxChroma; // max Chroma value for this hue
    double maxLightness; // max Lightness value for this hue
    double maxSaturation; // max Saturation value for this hue
};

static const int nb_color_sectors = 25;
static const struct_color_sectors color_sectorsCIELAB[nb_color_sectors] = { // color range values
    // name                   Hue   Begin  End       R    G    B        Cmax        Lmax        Smax
    {"gray",                  -1,     -1,  -1,      119, 119, 119,      0,          1.0,    1.0     },
    {"red",                    0,    353, 368,      255,   0,   0,      0.823242,   1.0,    0.966133},
    {"red-orange",            15,      8,  23,      255,  64,   0,      0.804527,   1.0,    0.939794},
    {"orange",                30,     23,  38,      255, 127,   0,      0.715872,   1.0,    0.863747},
    {"orange-yellow",         45,     38,  53,      255, 191,   0,      0.702699,   1.0,    0.776041},
    {"yellow",                60,     53,  68,      255, 255,   0,      0.772765,   1.0,    0.797105},
    {"yellow-chartreuse",     75,     68,  83,      191, 255,   0,      0.828566,   1.0,    0.831885},
    {"chartreuse",            90,     83,  98,      127, 255,   0,      0.896678,   1.0,    0.860653},
    {"green-chartreuse",     105,     98, 113,       64, 255,   0,      0.937215,   1.0,    0.882719},
    {"green",                120,    113, 128,        0, 255,   0,      0.943117,   1.0,    0.892363},
    {"green-spring",         135,    128, 143,        0, 255,  64,      0.918330,   1.0,    0.878063},
    {"spring",               150,    143, 158,        0, 255, 127,      0.790749,   1.0,    0.846108},
    {"cyan-spring",          165,    158, 173,        0, 255, 191,      0.609155,   1.0,    0.795174},
    {"cyan",                 180,    173, 188,        0, 255, 255,      0.443026,   1.0,    0.728756},
    {"cyan-azure",           195,    188, 203,        0, 191, 255,      0.455485,   1.0,    0.851052},
    {"azure",                210,    203, 218,        0, 127, 255,      0.744598,   1.0,    0.959106},
    {"blue-azure",           225,    218, 233,        0,  64, 255,      1.004860,   1.0,    0.992290},
    {"blue",                 240,    233, 248,        0,   0, 255,      1.053600,   1.0,    0.998063},
    {"blue-indigo",          255,    248, 263,       64,   0, 255,      1.046700,   1.0,    0.996781},
    {"indigo",               270,    263, 278,      127,   0, 255,      1.009120,   1.0,    0.993834},
    {"magenta-indigo",       285,    278, 293,      191,   0, 255,      0.956457,   1.0,    0.989317},
    {"magenta",              300,    293, 308,      255,   0, 255,      0.917661,   1.0,    0.983269},
    {"pink-magenta",         315,    308, 323,      255,   0, 191,      0.817888,   1.0,    0.977968},
    {"pink",                 330,    323, 338,      255,   0, 127,      0.690802,   1.0,    0.972634},
    {"red-pink",             345,    338, 353,      255,   0,  64,      0.781101,   1.0,    0.996981}
}; // max C L and S were obtained from function FindSectorsMaxValuesCIELab

static const struct_color_sectors color_sectorsOKLAB[nb_color_sectors] = { // color range values
    // name                   Hue   Begin  End       R    G    B        Cmax        Lmax        Smax
    {"gray",                  -1,     -1,  -1,      119, 119, 119,      0,          1.0,    1.0     },
    {"red",                    0,    353, 368,      255,   0,   0,      0.257683,   1.0,    0.379633},
    {"red-orange",            15,      8,  23,      255,  64,   0,      0.249243,   1.0,    0.364390},
    {"orange",                30,     23,  38,      255, 127,   0,      0.207552,   1.0,    0.287850},
    {"orange-yellow",         45,     38,  53,      255, 191,   0,      0.188347,   1.0,    0.213176},
    {"yellow",                60,     53,  68,      255, 255,   0,      0.221560,   1.0,    0.228892},
    {"yellow-chartreuse",     75,     68,  83,      191, 255,   0,      0.250691,   1.0,    0.267299},
    {"chartreuse",            90,     83,  98,      127, 255,   0,      0.278111,   1.0,    0.301638},
    {"green-chartreuse",     105,     98, 113,       64, 255,   0,      0.292767,   1.0,    0.319625},
    {"green",                120,    113, 128,        0, 255,   0,      0.294827,   1.0,    0.322136},
    {"green-spring",         135,    128, 143,        0, 255,  64,      0.289409,   1.0,    0.316604},
    {"spring",               150,    143, 158,        0, 255, 127,      0.257759,   1.0,    0.283677},
    {"cyan-spring",          165,    158, 173,        0, 255, 191,      0.206239,   1.0,    0.227999},
    {"cyan",                 180,    173, 188,        0, 255, 255,      0.162331,   1.0,    0.178249},
    {"cyan-azure",           195,    188, 203,        0, 191, 255,      0.179477,   1.0,    0.255725},
    {"azure",                210,    203, 218,        0, 127, 255,      0.250378,   1.0,    0.415632},
    {"blue-azure",           225,    218, 233,        0,  64, 255,      0.304189,   1.0,    0.547199},
    {"blue",                 240,    233, 248,        0,   0, 255,      0.313214,   1.0,    0.569556},
    {"blue-indigo",          255,    248, 263,       64,   0, 255,      0.308095,   1.0,    0.557197},
    {"indigo",               270,    263, 278,      127,   0, 255,      0.297027,   1.0,    0.507050},
    {"magenta-indigo",       285,    278, 293,      191,   0, 255,      0.312915,   1.0,    0.461913},
    {"magenta",              300,    293, 308,      255,   0, 255,      0.322491,   1.0,    0.429616},
    {"pink-magenta",         315,    308, 323,      255,   0, 191,      0.302098,   1.0,    0.411702},
    {"pink",                 330,    323, 338,      255,   0, 127,      0.270093,   1.0,    0.398669},
    {"red-pink",             345,    338, 353,      255,   0,  64,      0.256315,   1.0,    0.383114}
}; // max C L and S were obtained from function FindSectorsMaxValuesOKLAB

struct struct_color_category { // struct to store a category
    double begin, end; // range
    int R, G, B; // RGB equivalent with max saturation
    std::string name; // name
};

static const int nb_lightness_categories = 11;
static const struct_color_category lightness_categories[nb_lightness_categories] = { // Lightness categories
    { 0,    0.03,       11,  11,  11,      "black"},
    { 0.03, 0.10,       27,  27,  27,      "dark shadow"},
    { 0.10, 0.20,       48,  48,  48,      "middle shadow"},
    { 0.20, 0.30,       71,  71,  71,      "light shadow"},
    { 0.30, 0.45,       94,  94,  94,      "dark halftone"},
    { 0.45, 0.60,      119, 119, 119,      "halftone"},
    { 0.60, 0.70,      145, 145, 145,      "lowly bright"},
    { 0.70, 0.80,      171, 171, 171,      "medium bright"},
    { 0.80, 0.90,      198, 198, 198,      "high bright"},
    { 0.90, 0.97,      226, 226, 226,      "very bright"},
    { 0.97, 1.10,      247, 247, 247,      "white"}
};

static const int nb_chroma_categories = 11;
static const struct_color_category chroma_categories[nb_chroma_categories] = { // Chroma categories (from CIE LCHab) - values for cyan-blue H=240°
    { 0,     0.05,      111, 121, 126,      "gray"},
    { 0.05,  0.15,      101, 122, 133,      "near-gray"},
    { 0.15,  0.25,       77, 125, 148,      "very dull"},
    { 0.25,  0.35,       31, 128, 162,      "dull"},
    { 0.35,  0.45,        0, 131, 177,      "weakly colored"},
    { 0.45,  0.55,        0, 134, 192,      "lowly colored"},
    { 0.55,  0.65,        0, 137, 207,      "colored"},
    { 0.65,  0.75,        0, 140, 222,      "strong"},
    { 0.75,  0.85,        0, 142, 238,      "intense"},
    { 0.85,  0.95,        0, 145, 253,      "vivid"},
    { 0.95,  1.10,        0, 150, 255,      "saturated"}
};


int WhichColorSector(const double &H); // get the color sector of a given Hue in HSL (H in degrees)
int WhichLightnessCategory(const double &L); // get the Lightness category (L from OKLAB)
int WhichChromaCategory(const double &C, const int &colorSector); // get the Chroma category (C from OKLCH)
int WhichSaturationCategory(const double &S, const int &colorSector); // get the Saturation category (S from HSL)
std::vector<std::vector<int>> SectoredMeansSegmentation(const cv::Mat &image, cv::Mat &quantized); // BGR image segmentation by color sector mean (H from HSL)
void DrawSectoredMeansPalettesCIELab(); // save Sectored Means palettes to images : scales are computed, values come from pre-defined RGB colors - use as reference
void FindSectorsMaxValuesCIELab(const int &intervals, const std::string filename); // write max values (C, S, L) for each color sector (CIELab)
void FindSectorsMaxValuesOKLAB(const int &intervals, const std::string filename); // write max values (C, S, L) for each color sector (OKLAB)

//// useful to hash RGB colors
int Hash3Bytes(const uchar &B1, const uchar &B2, const uchar &B3); // concatenate 3 bytes in one integer value
void DeHash3Bytes(const int &hash, uchar &B1, uchar &B2, uchar &B3); // de-concatenate an integer's 3 lower bytes to individual values

///////////////////////////////////////////////
////                 Eigen
///////////////////////////////////////////////

typedef struct color_node { // for eigen algorithm
    cv::Mat     mean;
    cv::Mat     cov;
    int       class_id;

    color_node *left;
    color_node *right;
} color_node;

std::vector<cv::Vec3d> DominantColorsEigen(const cv::Mat &img, const int &nb_colors, cv::Mat &quantized); // Eigen algorithm with CIELab or OKLAB values in range [0..1]

///////////////////////////////////////////////
////                K-means
///////////////////////////////////////////////

cv::Mat DominantColorsKMeansRGB_U(const cv::Mat &source, const int &nb_clusters, cv::Mat1f &dominant_colors); // Dominant colors with K-means from RGB image using UMat
cv::Mat DominantColorsKMeansRGB(const cv::Mat &image, const int &cluster_number, cv::Mat1f &dominant_colors); // Dominant colors with K-means from RGB image
cv::Mat DominantColorsKMeans(const cv::Mat &image, const int &cluster_number, cv::Mat1f &dominant_colors); // Dominant colors with K-means in CIELAB or OKLAB space from RGB image

///////////////////////////////////////////////
////              Mean-Shift
///////////////////////////////////////////////

class Point5D { // 5-Dimensional Point
    public:
        double x;			// Spatial value
        double y;			// Spatial value
        double l;			// Lab value
        double a;			// Lab value
        double b;			// Lab value
    public:
        Point5D();													// Constructor
        ~Point5D();													// Destructor
        void MSPoint5DAccum(const Point5D &);								// Accumulate points
        void MSPoint5DCopy(const Point5D &);								// Copy a point
        double MSPoint5DColorDistance(const Point5D &);						// Compute color space distance between two points
        double MSPoint5DSpatialDistance(const Point5D &);					// Compute spatial space distance between two points
        void MSPoint5DScale(const double);									// Scale point
        void MSPOint5DSet(const double &, const double &, const double &, const double &, const double &);		// Set point value
        //void Print();												// Print 5D point
};

class MeanShift {
    public:
        double hs;				// spatial radius
        double hr;				// color radius
        std::vector<cv::Mat> IMGChannels;
    public:
        MeanShift(const double &, const double &);									// Constructor for spatial bandwidth and color bandwidth
        void MeanShiftFiltering(cv::Mat &img);										// Mean Shift Filtering
        void MeanShiftSegmentation(cv::Mat &img);									// Mean Shift Segmentation
};

#endif // DOMINANTCOLORS_H
