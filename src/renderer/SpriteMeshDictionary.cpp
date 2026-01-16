#include "SpriteMeshDictionary.hpp"
#include "Geode/cocos/sprite_nodes/CCSpriteFrame.h"
#include "Geode/cocos/sprite_nodes/CCSpriteFrameCache.h"
#include "common.hpp"
#include "glm/fwd.hpp"
#include <matjson.hpp>
#include <optional>
#include <unordered_map>

using namespace geode::prelude;

static std::unordered_map<CCSprite*, CCSpriteFrame*> spriteFramesOfSprites;

#include <Geode/modify/CCSprite.hpp>
class $modify(SpriteMeshDictionaryCCSprite, CCSprite) {
    bool initWithSpriteFrame(CCSpriteFrame* frame) {
        if (CCSprite::initWithSpriteFrame(frame)) {
            spriteFramesOfSprites[this] = frame;
            return true;
        }
        return false;
    }

    ~SpriteMeshDictionaryCCSprite() {
        spriteFramesOfSprites.erase(this);
    }
};

static std::unordered_map<CCSpriteFrame*, ConvexList> spriteMeshesPerFrame;

ConvexList* SpriteMeshDictionary::getSpriteMeshForSprite(cocos2d::CCSprite* sprite) {
    auto frameIt = spriteFramesOfSprites.find(sprite);
    if (frameIt == spriteFramesOfSprites.end())
        return nullptr;

    auto meshIt = spriteMeshesPerFrame.find(frameIt->second);
    if (meshIt == spriteMeshesPerFrame.end())
        return nullptr;

    return &meshIt->second;
}

static std::optional<ConvexList> parseConvexList(const matjson::Value& array);

static fs::path lastLoadedFilePath = "";

void SpriteMeshDictionary::loadFromFile(const fs::path& path) {
    if (lastLoadedFilePath == path)
        return;
    lastLoadedFilePath = path;

    spriteMeshesPerFrame.clear();

    auto source = readResourceFile(path);
    if (!source.has_value())
        return;

    auto result = matjson::parse(source.value());
    if (!result.isOk())
        return;

    auto value = result.unwrap();

    if (!value.isObject())
        return;

    auto frames = value.as<std::map<std::string, matjson::Value>>();
    if (!frames.ok())
        return;

    for (auto [key, value] : frames.unwrap()) {
        CCSpriteFrame* frame = CCSpriteFrameCache::get()->spriteFrameByName(key.c_str());
        if (!frame)
            continue;

        std::optional<ConvexList> convexList = parseConvexList(value);
        if (!convexList.has_value())
            continue;

        spriteMeshesPerFrame[frame] = convexList.value();
    }
}

static std::optional<ConvexPolygon> parseConvexPolygon(const matjson::Value& array);

static std::optional<ConvexList> parseConvexList(const matjson::Value& array) {
    if (!array.isArray()) return std::nullopt;

    ConvexList list;

    for (auto& value : array.as<std::vector<matjson::Value>>().unwrap()) {
        std::optional<ConvexPolygon> polygon = parseConvexPolygon(value);
        if (!polygon.has_value())
            return std::nullopt;

        list.addPolygon(polygon.value());
    }

    return list;
}

// It is assumed that the convex polygons are counter clockwise
static std::optional<ConvexPolygon> parseConvexPolygon(const matjson::Value& array) {
    if (!array.isArray())
        return std::nullopt;

    std::vector<glm::vec2> points;
    auto pointsJson = array.as<std::vector<matjson::Value>>().unwrap();

    for (auto& pointJson : pointsJson) {
        if (!pointJson.isArray())
            return std::nullopt;

        auto pointJsonArray = pointJson.as<std::vector<matjson::Value>>().unwrap();

        if (pointJsonArray.size() != 2)
            return std::nullopt;

        auto xValue = pointJsonArray[0];
        auto yValue = pointJsonArray[1];

        if (!xValue.isNumber() || !yValue.isNumber())
            return std::nullopt;

        points.push_back(glm::vec2 {
            xValue.as<float>().unwrap(),
            yValue.as<float>().unwrap()
        });
    }

    ConvexPolygon polygon;

    for (isize i = 0; i < points.size(); i++) {
        isize nextI = i + 1;
        if (nextI >= points.size()) nextI = 0;

        glm::vec2 p1 = points[i];
        glm::vec2 p2 = points[nextI];

        auto line = Line { p1, getClockwise(p2 - p1) };
        polygon.addLine(line);
    }

    return polygon;
}