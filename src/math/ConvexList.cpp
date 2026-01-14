#include "ConvexList.hpp"

void ConvexList::triangulate(TriangleCallback callback) const {
    for (const auto& polygon : polygons)
        polygon.triangulate(callback);
}