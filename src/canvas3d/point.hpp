#pragma once

namespace horizon {
class Point3D {
public:
    Point3D(double ax, double ay, double az) : x(ax), y(ay), z(az)
    {
    }
    double x;
    double y;
    double z;
};
} // namespace horizon
