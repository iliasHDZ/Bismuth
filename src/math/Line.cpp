#include "Line.hpp"
#include "glm/fwd.hpp"

static bool onSegment(glm::vec2 p, glm::vec2 q, glm::vec2 r) {
    if (q.x <= glm::max(p.x, r.x) && q.x >= glm::min(p.x, r.x)
        && q.y <= glm::max(p.y, r.y) && q.y >= glm::min(p.y, r.y))
        return true;
    return false;
}

static i32 orientation(glm::vec2 p, glm::vec2 q, glm::vec2 r) {
    double val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (val == 0)
        return 0;
  
    return (val > 0) ? 1 : 2;
}

static bool doIntersect(glm::vec2 p1, glm::vec2 q1, glm::vec2 p2, glm::vec2 q2, glm::vec2& res) {
    double a1 = q1.y - p1.y;
    double b1 = p1.x - q1.x;
    double c1 = a1 * p1.x + b1 * p1.y;

    double a2 = q2.y - p2.y;
    double b2 = p2.x - q2.x;
    double c2 = a2 * p2.x + b2 * p2.y;

    double determinant = a1 * b2 - a2 * b1;

    if (determinant != 0) {
        res.x = (c1 * b2 - c2 * b1) / determinant;
        res.y = (a1 * c2 - a2 * c1) / determinant;
        return true;
    }

    return false;
}

std::optional<glm::vec2> Line::intersectionWith(const Line& line) const {
    glm::vec2 point;

    if (doIntersect(firstPoint(), secondPoint(), line.firstPoint(), line.secondPoint(), point))
        return point;

    return std::nullopt;
}