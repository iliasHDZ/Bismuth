#pragma once

#include "glm/geometric.hpp"
#include <common.hpp>
#include <optional>

#define EPSILON 0.00001

// This is useful for ordering vectors
inline float psuedoangle(const glm::vec2& v) {
    float r = v.y / (abs(v.x) + abs(v.y));
    return (v.x < 0) ? (2 - r) : (4 + r);
}

/*
    This class defines a infinite line.

    Importantly, this line has a distinct facing direction.
    In other words, it is always known whether a point is
    infront or behind the line.
*/
class Line {
public:
    inline Line(float distance, const glm::vec2& normal)
        : distance(distance), normal(glm::normalize(normal)) {}

    inline Line(const glm::vec2& point, const glm::vec2& normal)
        : normal(glm::normalize(normal))
    {
        distance = glm::dot(point, this->normal);
    }

    inline bool isPointInFront(const glm::vec2& p) const {
        return glm::dot(p, normal) > distance;
    }

    inline bool isPointBehind(const glm::vec2& p) const { return !isPointInFront(p); }

    inline float psuedoangle() const { return ::psuedoangle(normal); }

    inline glm::vec2 firstPoint() const { return normal * distance; }

    inline glm::vec2 secondPoint() const { return glm::vec2(normal.y, -normal.x) + firstPoint(); }

    inline bool sameNormalAs(const Line& line) const {
        return abs(psuedoangle() - line.psuedoangle()) < EPSILON;
    }

    std::optional<glm::vec2> intersectionWith(const Line& line) const;

public:
    /*
        This is a normalized vector perpendicular
        to the line that points toward the front
        of the line.
    */
    glm::vec2 normal;

    /*
        This is the line's distance from
        the origin.
    */
    float distance;
};