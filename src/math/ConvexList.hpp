#pragma once

#include "ConvexPolygon.hpp"

class ConvexList {
public:
    inline void addPolygon(const ConvexPolygon& polygon) {
        polygons.push_back(polygon);
    }

    void triangulate(TriangleCallback callback) const;

private:
    std::vector<ConvexPolygon> polygons;
};