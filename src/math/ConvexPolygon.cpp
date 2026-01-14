#include "ConvexPolygon.hpp"
#include "Line.hpp"
#include <optional>

using namespace geode::prelude;

bool ConvexPolygon::containsPoint(const glm::vec2& point) const {
    for (const Line& line : lines) {
        if (line.isPointInFront(point))
            return false;
    }
    return true;
}

void ConvexPolygon::addLine(const Line& line) {
    isize index = 0;
    if (isWholePolygonBehindLine(line, &index))
        return;

    lines.insert(lines.begin() + index, line);
    removeExcessLines();
}

bool ConvexPolygon::isWholePolygonBehindLine(const Line& line, isize* indexOut) const {
    if (lines.size() == 0)
        return false;

    float angle = line.psuedoangle();

    isize index = -1;
    for (isize i = 0; i < lines.size(); i++) {
        float oangle = lines[i].psuedoangle();

        if (abs(angle - oangle) < EPSILON)
            return line.distance > lines[i].distance;

        if (oangle > angle && index == -1)
            index = i;
    }

    if (indexOut)
        *indexOut = (index == -1) ? lines.size() : index;

    if (index == -1)
        return false;

    isize prevIdx = prevIndex(index);

    if (prevIdx == index)
        return false;

    const Line& prevLine = lines[prevIdx];
    const Line& nextLine = lines[index];

    if (glm::dot(line.normal, prevLine.normal) < 0 || glm::dot(line.normal, nextLine.normal) < 0)
        return false;

    std::optional<glm::vec2> intersection = nextLine.intersectionWith(prevLine);

    if (!intersection.has_value())
        return false;

    return line.isPointBehind(intersection.value());
}

void ConvexPolygon::removeExcessLines() {
    for (isize i = 0; i < lines.size();) {
        isize nextI = nextIndex(i);

        if (i == nextI)
            break;

        const Line& line     = lines[i];
        const Line& nextLine = lines[nextI];

        if (line.sameNormalAs(nextLine)) {
            if (nextLine.distance < line.distance)
                lines[i] = nextLine;

            lines.erase(lines.begin() + nextI);
        } else
            i++;
    }

    for (isize i = 0; i < lines.size();) {
        isize prevI = prevIndex(i);
        isize nextI = nextIndex(i);

        if (prevI == i || nextI == i || prevI == nextI)
            break;

        const Line& prevLine = lines[prevI];
        const Line& nextLine = lines[nextI];
        const Line& line     = lines[i];

        std::optional<glm::vec2> intersection;
        
        if (glm::dot(line.normal, prevLine.normal) > 0 && glm::dot(line.normal, nextLine.normal) > 0)
            intersection = nextLine.intersectionWith(prevLine);
            
        if (intersection.has_value() && line.isPointBehind(intersection.value())) {
            lines.erase(lines.begin() + i);
        } else
            i++;
    }
}

void ConvexPolygon::triangulate(TriangleCallback callback) const {
    if (lines.size() <= 2)
        return;

    std::optional<glm::vec2> firstPointOpt = lines[0].intersectionWith(lines[1]);
    if (!firstPointOpt) return;

    std::optional<glm::vec2> secondPointOpt = lines[1].intersectionWith(lines[2]);
    if (!secondPointOpt) return;

    glm::vec2 firstPoint = firstPointOpt.value();
    glm::vec2 prevPoint  = secondPointOpt.value();

    for (isize i = 2; i < lines.size(); i++) {
        isize nextIndex = i + 1;
        if (nextIndex == lines.size())
            nextIndex = 0;

        std::optional<glm::vec2> pointOpt = lines[i].intersectionWith(lines[nextIndex]);
        if (!pointOpt) return;

        glm::vec2 point = pointOpt.value();

        callback(point, firstPoint, prevPoint);
        prevPoint = point;
    }
}