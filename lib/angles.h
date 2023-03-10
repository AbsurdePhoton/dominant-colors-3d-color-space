/*#-------------------------------------------------
#
#             Angles utilities library
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.1 - 2021/04/12
#
#-------------------------------------------------*/

#ifndef ANGLES_H
#define ANGLES_H

#include <limits>
#include <cmath>


static const double Pi = 3.14159265358979323846264338328;

// Epsilon values for types
static const double EPSd = std::numeric_limits<double>::epsilon();
static const float  EPSf = std::numeric_limits<float>::epsilon();
static const int    EPS255 = 1;


double DegInRange(const double &deg); // keep degrees angle in [0..360]
double RadInRange(const double &rad); // keep radians angle in [0..2Pi]

inline double RadToDeg(const double &rad) // convert radians value to degrees
{
    return 180.0 / Pi * rad; // 2Pi = 360°
}

inline double DegToRad(const double &deg) // convert degrees value to radians
{
    return deg * Pi / 180.0; // 360° = 2Pi
}

inline double NormalizedToRad(const double &normalized) // convert normalized radians to 2Pi rad
{
    return normalized * 2.0 * Pi; // 2Pi
}

inline double NormalizedToDeg(const double &normalized) // convert normalized degrees to 360°
{
    return normalized * 360.0; // 360°
}

inline double RadToNormalized(const double &rad) // convert radians to [0..1]
{
    return rad / 2.0 / Pi; // 2Pi
}

inline double DegToNormalized(const double &deg) // convert degrees to [0..1]
{
    return deg / 360.0; // 360°
}

double DifferenceRad(const double &a1, const double &a2); // distance between 2 angles in radians
double DifferenceDeg(const double &a1, const double &a2); // distance between 2 angles in degrees

inline double VectorToHorizontalRad(const double &x1, const double &y1, const double &x2, const double &y2) // degrees angle between vector and horizontal line
{
    return atan2(y1 - y2, x2 - x1); // clockwise coordinate system
}

inline double VectorToHorizontalDeg(const double &x1, const double &y1, const double &x2, const double &y2) // radians angle between vector and horizontal line
{
    return RadToDeg(VectorToHorizontalRad(x1, y1, x2, y2));
}

#endif // ANGLES_H
