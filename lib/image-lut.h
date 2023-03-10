/*#-------------------------------------------------
#
#      Image Cube LUT library with OpenCV
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.0 - 2022/03/09
#
#   - Load 3D Cube LUT files
#   - Apply Cube LUT with opacity factor :
#       * 1D
#       * Nearest
#       * Tri-linear (best)
#
#-------------------------------------------------*/

#ifndef IMAGE_LUT_H
#define IMAGE_LUT_H

#include "opencv2/opencv.hpp"

#include <fstream>


class CubeLUT
{
public:
        using tableRow = std::vector<double>;
        using table1D = std::vector<tableRow>;
        using table2D = std::vector<table1D>;
        using table3D = std::vector<table2D>;

        enum LUTState {
            OK = 0,
            NotInitialized = 1,
            ReadError = 10,
            WriteError,
            PrematureEndOfFile,
            LineError,
            UnknownOrRepeatedKeyword = 20,
            TitleMissingQuote,
            DomainBoundsReversed,
            LUTSizeOutOfRange,
            CouldNotParseTableData
        };

        enum LUTMode {
            Trilinear = 0,
            Nearest = 1
        };

        LUTState status;
        std::string title;
        tableRow domainMin;
        tableRow domainMax;
        table1D LUT1D;
        table3D LUT3D;

        CubeLUT()
        {
                status = NotInitialized;
        }

        LUTState LoadCubeFile(std::ifstream& infile);

        cv::Mat_<cv::Vec3b> ApplyLUT(const cv::Mat image, double opacity, LUTMode mode);

private:
        std::string ReadLine(std::ifstream& infile, char lineSeparator);
        tableRow ParseTableRow(const std::string& lineOfText);

        //// for 1D
        double getAvgVal(int nValues, unsigned char value, unsigned char channel);
        unsigned char getColor(double value);
        cv::Mat_<cv::Vec3b> applyBasic1D(const cv::Mat& img, double opacity);
        //// for tri-linear
        std::vector<double> mul(const std::vector<double>& vec, const double val);
        std::vector<double> sum(const std::vector<double>& a, const std::vector<double>& b);
        cv::Mat applyTrilinear(cv::Mat img, double opacity);
        //// for nearest
        cv::Mat applyNearest(cv::Mat img, double opacity);
};

#endif
