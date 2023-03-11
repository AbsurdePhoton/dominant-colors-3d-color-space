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


#include "image-lut.h"
#include <QDebug>


// .cube parser code: Adobe
// Cube LUT Specification 1.0 https://wwwimages2.adobe.com/content/dam/acom/en/products/speedgrade/cc/pdfs/cube-lut-specification-1.0.pdf

std::string CubeLUT::ReadLine(std::ifstream& infile, const char lineSeparator)
{
    // Skip empty lines and comments
    const char CommentMarker = '#';
    std::string textLine;
    while ((textLine.empty()) or (textLine[0] == CommentMarker))
    {
        if (infile.eof())
        {
            status = PrematureEndOfFile;
            break;
        }
        getline(infile, textLine, lineSeparator);
        if (infile.fail())
        {
            status = ReadError;
            break;
        }
    }
    return textLine;
}

std::vector<double> CubeLUT::ParseTableRow(const std::string& lineOfText)
{
    int N = 3;
    tableRow f(N);
    std::istringstream line(lineOfText);
    for (int i{ 0 }; i < N; ++i)
    {
        line >> f[i];
        if (line.fail())
        {
            status = CouldNotParseTableData;
            break;
        }
    }
    return f;
}

CubeLUT::LUTState CubeLUT::LoadCubeFile(std::ifstream& infile)
{
    // defaults
    status = OK;
    title.clear();
    domainMin = tableRow(3, 0.0);
    domainMax = tableRow(3, 1.0);

    LUT1D.clear();
    LUT3D.clear();

    const char NewlineCharacter = '\n';
    char lineSeparator = NewlineCharacter;

    const char CarriageReturnCharacter = '\r';
    for (int i{ 0 }; i < 255; ++i)
    {
        char inc = infile.get();
        if (inc == NewlineCharacter)
            break;
        if (inc == CarriageReturnCharacter)
        {
            if (infile.get() == NewlineCharacter)
                break;
            lineSeparator = CarriageReturnCharacter;
            //clog << "INFO: This file uses non-compliant line separator \\r (0x0D)" << endl;
        }
        if (i > 250)
        {
            status = LineError;
            break;
        }
    }
    infile.seekg(0);
    infile.clear();

    int N, CntTitle, CntSize, CntMin, CntMax;
    N = CntTitle = CntSize = CntMin = CntMax = 0;
    long linePos = 0;
    while (status == OK)
    {
        linePos = infile.tellg();
        std::string lineOfText = ReadLine(infile, lineSeparator);
        if (status != OK)
            break;

        std::istringstream line(lineOfText);
        std::string keyword;
        line >> keyword;

        if (("+" < keyword) and (keyword < ":")) //numbers
        {
            infile.seekg(linePos);
            break;
        }
        else if ((keyword == "TITLE") and (CntTitle++ == 0))
        {
            const char QUOTE = '"';
            char startOfTitle;
            line >> startOfTitle;
            if (startOfTitle != QUOTE)
            {
                status = TitleMissingQuote;
                break;
            }
            getline(line, title, QUOTE);
        }
        else if ((keyword == "DOMAIN_MIN") and (CntMin++ == 0))
        {
            line >> domainMin[0] >> domainMin[1] >> domainMin[2];
        }
        else if ((keyword == "DOMAIN_MAX") and (CntMax++ == 0))
        {
            line >> domainMax[0] >> domainMax[1] >> domainMax[2];
        }
        else if ((keyword == "LUT_1D_SIZE") and (CntSize++ == 0))
        {
            line >> N;
            if ((N < 2) or (N > 65536))
            {
                status = LUTSizeOutOfRange;
                qDebug() << "LUT size out of range : " << N;
                break;
            }
            LUT1D = table1D(N, tableRow(3));
        }
        else if ((keyword == "LUT_3D_SIZE") and (CntSize++ == 0))
        {
            line >> N;
            if ((N < 2) or (N > 256))
            {
                status = LUTSizeOutOfRange;
                qDebug() << "LUT size out of range : " << N;
                break;
            }
            LUT3D = table3D(N, table2D(N, table1D(N, tableRow(3))));
        }
        else
        {
            if (keyword != "") {
                status = UnknownOrRepeatedKeyword;
                qDebug() << "Unknown keyword : " << QString::fromStdString(keyword);
                break;
            }
        }

        if ((line.fail()) and (keyword != ""))
        {
            status = ReadError;
            break;
        }
    }

    if (status == OK) {
        if (CntSize == 0) {
            status = LUTSizeOutOfRange;
            qDebug() << "LUT size out of range : no data";
        }
        if ((domainMin[0] >= domainMax[0]) or (domainMin[1] >= domainMax[1]) or (domainMin[2] >= domainMax[2]))
            status = DomainBoundsReversed;
    }

    // parsing loaded data
    infile.seekg(linePos - 1);
    while (infile.get() != '\n')
    {
        infile.seekg(--linePos);
    }
    // checking if there is a beginning of the line

    if (LUT1D.size() > 0)
    {
        N = LUT1D.size();
        for (int i{ 0 }; (i < N) and (status == OK); ++i)
        {
            LUT1D[i] = ParseTableRow(ReadLine(infile, lineSeparator));
        }
    }
    else
    {
        N = LUT3D.size();

        for (int b{ 0 }; (b < N) and (status == OK); ++b)
        {
            for (int g{ 0 }; (g < N) and (status == OK); ++g)
            {
                for (int r{ 0 }; (r < N) and (status == OK); ++r)
                {
                    LUT3D[r][g][b] = ParseTableRow(ReadLine(infile, lineSeparator));
                }
            }
        }
    }

    return status;
}

//// apply 1D
double CubeLUT::getAvgVal(int nValues, const unsigned char value, const unsigned char channel)
{
    double val = 0;

    for (unsigned int i = 0; i < (unsigned int)nValues; ++i) {
        val += LUT1D[value * nValues + i][channel];
    }

    return val / nValues;
}

unsigned char CubeLUT::getColor(const double value)
{
    if (value > 1.0) { // assuming that max domain is 1.0
        return 255;
    }

    return static_cast<unsigned char>(round(value * 255.0));
}

cv::Mat_<cv::Vec3b> CubeLUT::applyBasic1D(const cv::Mat& img, const double opacity)
{
    int total = img.cols * img.rows;
    cv::Mat result = img.clone();
    cv::Vec3b* resultP = result.ptr<cv::Vec3b>(0);

#pragma omp parallel for
    for (int n = 0; n < total; n++) {
        double b = resultP[n][0] / 255.0;
        double g = resultP[n][1] / 255.0;
        double r = resultP[n][2] / 255.0;

        double r_o = r * (LUT1D.size() - 1);
        double g_o = g * (LUT1D.size() - 1);
        double b_o = b * (LUT1D.size() - 1);

        double R1 = ceil(r_o);
        double R0 = floor(r_o);
        double G1 = ceil(g_o);
        double G0 = floor(g_o);
        double B1 = ceil(b_o);
        double B0 = floor(b_o);

        double delta_r { r_o - R0 == 0 || R1 - R0 == 0 ? 0.000000001 : (r_o - R0) / (R1 - R0) };
        double delta_g { g_o - G0 == 0 || G1 - G0 == 0 ? 0.000000001 : (g_o - G0) / (G1 - G0) };
        double delta_b { b_o - B0 == 0 || B1 - B0 == 0 ? 0.000000001 : (b_o - B0) / (B1 - B0) };

        double new_r = LUT1D[R0][0] + (LUT1D[R1][0] - LUT1D[R0][0]) * delta_r;
        double new_g = LUT1D[R0][1] + (LUT1D[R1][1] - LUT1D[R0][1]) * delta_g;
        double new_b = LUT1D[R0][2] + (LUT1D[R1][2] - LUT1D[R0][2]) * delta_b;

        resultP[n][0] = (b + (new_b - b) * opacity) * 255.0;
        resultP[n][1] = (g + (new_g - g) * opacity) * 255.0;
        resultP[n][2] = (r + (new_r - r) * opacity) * 255.0;
    }

    return result;
}

//// apply tri-linear
std::vector<double> CubeLUT::mul(const std::vector<double> &vec, const double val)
{
    std::vector<double> newVec(3, 0.0f);
    for (int i = 0; i < 3; ++i)
        newVec[i] = vec[i] * val;

    return newVec;
}

std::vector<double> CubeLUT::sum(const std::vector<double> &a, const std::vector<double> &b)
{
    std::vector<double> newVec(3, 0.0f);
    for (int i = 0; i < 3; ++i)
        newVec[i] = a[i] + b[i];

    return newVec;
}

cv::Mat CubeLUT::applyTrilinear(cv::Mat img, const double opacity)
{
    int total = img.cols * img.rows;
    cv::Mat result = img.clone();
    cv::Vec3b* resultP = result.ptr<cv::Vec3b>(0);

#pragma omp parallel for
    for (int n = 0; n < total; n++) {
        double b = resultP[n][0] / 255.0;
        double g = resultP[n][1] / 255.0;
        double r = resultP[n][2] / 255.0;

        double r_o = r * (LUT3D[0].size() - 1);
        double g_o = g * (LUT3D[1].size() - 1);
        double b_o = b * (LUT3D[2].size() - 1);

        double R1 = ceil(r_o);
        double R0 = floor(r_o);
        double G1 = ceil(g_o);
        double G0 = floor(g_o);
        double B1 = ceil(b_o);
        double B0 = floor(b_o);

        double delta_r { r_o - R0 == 0 || R1 - R0 == 0 ? 0.000000001 : (r_o - R0) / (R1 - R0) };
        double delta_g { g_o - G0 == 0 || G1 - G0 == 0 ? 0.000000001 : (g_o - G0) / (G1 - G0) };
        double delta_b { b_o - B0 == 0 || B1 - B0 == 0 ? 0.000000001 : (b_o - B0) / (B1 - B0) };

        std::vector<double> vr_gz_bz = sum(mul(LUT3D[R0][G0][B0], 1.0 - delta_r), mul(LUT3D[R1][G0][B0], delta_r));
        std::vector<double> vr_gz_bo = sum(mul(LUT3D[R0][G0][B1], 1.0 - delta_r), mul(LUT3D[R1][G0][B1], delta_r));
        std::vector<double> vr_go_bz = sum(mul(LUT3D[R0][G1][B0], 1.0 - delta_r), mul(LUT3D[R1][G1][B0], delta_r));
        std::vector<double> vr_go_bo = sum(mul(LUT3D[R0][G1][B1], 1.0 - delta_r), mul(LUT3D[R1][G1][B1], delta_r));

        std::vector<double> vrg_b0 = sum(mul(vr_gz_bz, 1.0 - delta_g), mul(vr_go_bz, delta_g));
        std::vector<double> vrg_b1 = sum(mul(vr_gz_bo, 1.0 - delta_g), mul(vr_go_bo, delta_g));

        std::vector<double> vrgb = sum(mul(vrg_b0, 1.0 - delta_b), mul(vrg_b1, delta_b));

        resultP[n][0] = (b + (vrgb[2] - b) * opacity) * 255.0;
        resultP[n][1] = (g + (vrgb[1] - g) * opacity) * 255.0;
        resultP[n][2] = (r + (vrgb[0] - r) * opacity) * 255.0;
    }

    return result;
}

//// apply nearest value
cv::Mat CubeLUT::applyNearest(cv::Mat img, const double opacity)
{
    int total = img.cols * img.rows;
    cv::Mat result = img.clone();
    cv::Vec3b* resultP = result.ptr<cv::Vec3b>(0);

#pragma omp parallel for
    for (int n = 0; n < total; n++) {
            unsigned int b_ind = round(resultP[n][0] * (LUT3D.size() - 1) / 255.0f);
            unsigned int g_ind = round(resultP[n][1] * (LUT3D.size() - 1) / 255.0f);
            unsigned int r_ind = round(resultP[n][2] * (LUT3D.size() - 1) / 255.0f);

            int newB = (int)(LUT3D[r_ind][g_ind][b_ind][2] * 255);
            int newG = (int)(LUT3D[r_ind][g_ind][b_ind][1] * 255);
            int newR = (int)(LUT3D[r_ind][g_ind][b_ind][0] * 255);

            unsigned char finalB = resultP[n][0] + (newB - resultP[n][0]) * opacity;
            unsigned char finalG = resultP[n][1] + (newG - resultP[n][1]) * opacity;
            unsigned char finalR = resultP[n][2] + (newR - resultP[n][2]) * opacity;

            resultP[n][0] = finalB;
            resultP[n][1] = finalG;
            resultP[n][2] = finalR;
    }

    return result;
}

//// Apply
cv::Mat_<cv::Vec3b> CubeLUT::ApplyLUT(const cv::Mat image, double opacity, LUTMode mode)
{
    if (status == OK) {
        cv::Mat result;

        if (!LUT1D.empty()) {
            result = applyBasic1D(image, opacity);
        }
        else if (!LUT3D.empty()) {
            if (mode == Trilinear)
                result = applyTrilinear(image, opacity);
            else
                result = applyNearest(image, opacity);
        }

        return result;
    }
    else
        return cv::Mat();
}
