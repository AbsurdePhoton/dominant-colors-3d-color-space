/*#-------------------------------------------------
#
#             Angles utilities library
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.1 - 2021/04/12
#
#-------------------------------------------------*/

#include <cmath>

#include "angles.h"


///////////////////////////////////////////////////////////
//// Angle range
///////////////////////////////////////////////////////////

double DegInRange(const double &deg) // keep degrees angle in [0..360-]
{
    double angle = deg;

    while (angle < 0.0)
        angle += 360.0;
    while (angle >= 360.0)
        angle -= 360.0;

    return angle;
}

double RadInRange(const double &rad) // keep radians angle in [0..2Pi]
{
    double angle = rad;

    while (angle < 0.0)
        angle += 2.0 * Pi;
    while (angle >= 2.0 * Pi)
        angle -= 2.0 * Pi;

    return angle;
}

///////////////////////////////////////////////////////////
//// Angle difference
///////////////////////////////////////////////////////////

double DifferenceRad(const double &a1, const double &a2) // distance between 2 angles in radians
{
    double angle = abs(a1 - a2); // difference
    if (angle > Pi) // 2Pi-0 rad limit (circle)
        angle = Pi - angle;

    return angle;
}

double DifferenceDeg(const double &a1, const double &a2) // distance between 2 angles in degrees
{
    double angle = abs(a1 - a2); // difference
    if (angle > 180.0) // 360-0° degrees limit (circle)
        angle = 360.0 - angle;

    return angle;
}
