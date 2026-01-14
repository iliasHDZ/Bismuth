#pragma once

#include "Line.hpp"
#include <common.hpp>
#include <vector>
#include <span>

using TriangleCallback = std::function<void(const glm::vec2&, const glm::vec2&, const glm::vec2&)>;

/*
    This convex polygon is made up of a number infinitely long lines.
    Any points that is behind all of the lines is inside the polygon.

    (look at Line.hpp to understand what 'behind' means here)
*/
class ConvexPolygon {
public:
    bool containsPoint(const glm::vec2& point) const;

    void addLine(const Line& line);

    /*
        This returns true when there is no part of
        the polygon that lies infront to the line.
    */
    bool isWholePolygonBehindLine(const Line& line, isize* index = nullptr) const;

    void removeExcessLines();

    void triangulate(TriangleCallback callback) const;

    inline isize nextIndex(isize index) const {
        index++;
        if (index >= lines.size()) return 0;
        return index;
    }

    inline isize prevIndex(isize index) const {
        index--;
        if (index < 0) return lines.size() - 1;
        return index;
    }

    inline std::span<const Line> getLines() const {
        return lines;
    }

private:
    // This array must always be ordered by angle (or psuedoangle)
    std::vector<Line> lines;
};