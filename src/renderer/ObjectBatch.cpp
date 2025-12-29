#include "ObjectBatch.hpp"
#include "Renderer.hpp"
#include "common.hpp"

#include <string>

using namespace geode::prelude;

#define QUAD_BL 0
#define QUAD_BR 1
#define QUAD_TL 2
#define QUAD_TR 3

ObjectBatch::~ObjectBatch() {
    if (vertexBuffer)
        Buffer::destroy(vertexBuffer);
    if (indexBuffer)
        Buffer::destroy(indexBuffer);
    if (vao)
        glDeleteVertexArrays(1, &vao);
}

static u32 countSelfAndDesendantsIfDraw(CCSprite* node) {
    u32 count = node->getDontDraw() ? 0 : 1;

    CCObject* child;
    CCARRAY_FOREACH(node->getChildren(), child) {
        count += countSelfAndDesendantsIfDraw((CCSprite*)child);
    }

    return count;
}

void ObjectBatch::reserveForGameObject(GameObject* object) {
    reservedQuadCount += countSelfAndDesendantsIfDraw(object);

    if (object->m_glowSprite)
        reservedQuadCount += countSelfAndDesendantsIfDraw(object->m_glowSprite);
    if (object->m_colorSprite && object->m_colorSprite->getParent() != object)
        reservedQuadCount += countSelfAndDesendantsIfDraw(object->m_colorSprite);
}

void ObjectBatch::allocateReservations() {
    quads.resize(reservedQuadCount);
    currentQuadIndex = 0;
}

ObjectQuad* ObjectBatch::writeQuad(
    cocos2d::CCSprite* sprite,
    const cocos2d::CCAffineTransform& transform,
    SpriteSheet spriteSheet,
    const glm::vec2& parentObjectPosition
) {     
    CCTexture2D* texture = renderer->getSpriteSheetTexture(spriteSheet);
    if (texture == nullptr)
        return nullptr;

    CCPoint relativeOffset = sprite->getUnflippedOffsetPosition();

    if (sprite->isFlipX())
        relativeOffset.x = -relativeOffset.x;
    if (sprite->isFlipY())
        relativeOffset.y = -relativeOffset.y;

    CCRect crop = sprite->getTextureRect();
    CCSize size = crop.size;

    if (sprite->isTextureRectRotated()) {
        float temp = crop.size.height;
        crop.size.height = crop.size.width;
        crop.size.width  = temp;
    }

    float contentScale = CCDirector::get()->getContentScaleFactor();

    u32 texWidth  = texture->getPixelsWide();
    u32 texHeight = texture->getPixelsHigh();

    crop.origin.x    *= contentScale / texWidth;
    crop.origin.y    *= contentScale / texHeight;
    crop.size.width  *= contentScale / texWidth;
    crop.size.height *= contentScale / texHeight;

    auto& quad = quads[currentQuadIndex];
    
    float x1 = sprite->getOffsetPosition().x;
    float y1 = sprite->getOffsetPosition().y;

    float x2 = x1 + size.width;
    float y2 = y1 + size.height;
    float x = transform.tx;
    float y = transform.ty;

    float cr  =  transform.a;
    float sr  =  transform.b;
    float cr2 =  transform.d;
    float sr2 = -transform.c;
    float ax = x1 * cr - y1 * sr2 + x;
    float ay = x1 * sr + y1 * cr2 + y;
    float bx = x2 * cr - y1 * sr2 + x;
    float by = x2 * sr + y1 * cr2 + y;
    float cx = x2 * cr - y2 * sr2 + x;
    float cy = x2 * sr + y2 * cr2 + y;
    float dx = x1 * cr - y2 * sr2 + x;
    float dy = x1 * sr + y2 * cr2 + y;

    quad.verticies[QUAD_BL].positionOffset = glm::vec2(ax, ay) - parentObjectPosition;
    quad.verticies[QUAD_BR].positionOffset = glm::vec2(bx, by) - parentObjectPosition;
    quad.verticies[QUAD_TL].positionOffset = glm::vec2(dx, dy) - parentObjectPosition;
    quad.verticies[QUAD_TR].positionOffset = glm::vec2(cx, cy) - parentObjectPosition;

    float left   = crop.origin.x;
    float right  = crop.origin.x + crop.size.width;
    float top    = crop.origin.y;
    float bottom = crop.origin.y + crop.size.height;

    if (sprite->isTextureRectRotated()) {
        if (sprite->isFlipX()) std::swap(top, bottom);
        if (sprite->isFlipY()) std::swap(left, right);

        quad.verticies[QUAD_BL].texCoord = glm::vec2(left,  top);
        quad.verticies[QUAD_BR].texCoord = glm::vec2(left,  bottom);
        quad.verticies[QUAD_TL].texCoord = glm::vec2(right, top);
        quad.verticies[QUAD_TR].texCoord = glm::vec2(right, bottom);
    } else {
        if (sprite->isFlipX()) std::swap(left, right);
        if (sprite->isFlipY()) std::swap(top, bottom);

        quad.verticies[QUAD_BL].texCoord = glm::vec2(left,  bottom);
        quad.verticies[QUAD_BR].texCoord = glm::vec2(right, bottom);
        quad.verticies[QUAD_TL].texCoord = glm::vec2(left,  top);
        quad.verticies[QUAD_TR].texCoord = glm::vec2(right, top);
    }

    currentQuadIndex++;
    return &quad;
}

void ObjectBatch::writeGameObjects(Ref<CCArray> objects) {
    CCObject* child;
    CCARRAY_FOREACH(objects, child) {
        reserveForGameObject((GameObject*)child);
    }

    allocateReservations();

    CCARRAY_FOREACH(objects, child) {
        writeGameObject((GameObject*)child);
    }

    finishWriting();
}

void ObjectBatch::writeGameObject(GameObject* object) {
    CCAffineTransform transform = CCAffineTransformMakeIdentity();

    float originalScaleX = object->getScaleX();
    float originalScaleY = object->getScaleY();

    if (object->m_usesAudioScale) {
        object->setScaleX(object->m_scaleX);
        object->setScaleY(object->m_scaleY);
    }

    SpriteSheet sheet = (SpriteSheet)object->getParentMode();

    DEBUG_LOG("OBJECT {}", (void*)object);
    DEBUG_LOG("- isObjectBlack: {}", object->m_isObjectBlack);
    DEBUG_LOG("- isColorSpriteBlack: {}", object->m_isColorSpriteBlack);
    DEBUG_LOG("- hasColorSprite: {}", object->m_hasColorSprite);
    DEBUG_LOG("- colorSprite: {}", (void*)object->m_colorSprite);
    DEBUG_LOG("- activeMainColorID: {}", object->m_activeMainColorID);
    DEBUG_LOG("- activeDetailColorID: {}", object->m_activeDetailColorID);
    DEBUG_LOG("- opacityMod2: {}", object->m_opacityMod2);
    DEBUG_LOG("- srbIndex: {}", renderer->getObjectSRBIndex(object));
    DEBUG_LOG("- sprites:");

    writeSprite(object, object, transform);

    // transform = CCAffineTransformConcat(transform, object->nodeToParentTransform());

    if (object->m_glowSprite && object->m_glowSprite->getParent() != object)
        writeSprite(object, object->m_glowSprite, transform);

    if (object->m_colorSprite && object->m_colorSprite->getParent() != object)
        writeSprite(object, object->m_colorSprite, transform);

    object->setScaleX(originalScaleX);
    object->setScaleY(originalScaleY);
}

static std::string spriteTypeToString(ObjectBatch::SpriteType type) {
    switch (type) {
    case ObjectBatch::SpriteType::MAIN:  return "MAIN ";
    case ObjectBatch::SpriteType::COLOR: return "COLOR";
    case ObjectBatch::SpriteType::GLOW:  return "GLOW ";
    }
    return "<invalid>";
}

void ObjectBatch::writeSprite(
    GameObject* object,
    cocos2d::CCSprite* sprite,
    cocos2d::CCAffineTransform transform,
    SpriteType type
) {
    transform = CCAffineTransformConcat(sprite->nodeToParentTransform(), transform);

    if (sprite == object->m_colorSprite) type = SpriteType::COLOR;
    if (sprite == object->m_glowSprite)  type = SpriteType::GLOW;

    for (auto child : CCArrayExt<CCSprite*>(sprite->getChildren())) {
        if (child->getZOrder() < 0)
            writeSprite(object, child, transform, type);
    }
    
    if (!sprite->getDontDraw()) {
        SpriteSheet sheet = type != SpriteType::GLOW ? (SpriteSheet)object->getParentMode() : SpriteSheet::GLOW;

        u32 colorChannel = type == SpriteType::COLOR ? object->m_activeDetailColorID : object->m_activeMainColorID;

        bool isSpriteBlack = (sprite == object) ? object->m_isObjectBlack : object->m_isColorSpriteBlack;
        if (isSpriteBlack)
            colorChannel = COLOR_CHANNEL_BLACK;
        if (type == SpriteType::GLOW && object->m_glowColorIsLBG)
            colorChannel = COLOR_CHANNEL_LBG;

        if (type == SpriteType::COLOR)
            colorChannel |= A_COLOR_CHANNEL_IS_SPRITE_DETAIL;

        DEBUG_LOG("  - {} {}SPRITE {}", spriteTypeToString(type), isSpriteBlack ? "BLACK " : "", (void*)sprite);

        DEBUG_LOG("    {:.2f}, {:.2f}, {:.2f}", transform.a, transform.c, transform.tx);
        DEBUG_LOG("    {:.2f}, {:.2f}, {:.2f}", transform.b, transform.d, transform.ty);

        usize srbIndex = renderer->getObjectSRBIndex(object);

        auto quad = writeQuad(sprite, transform, sheet, ccPointToGLM(object->m_startPosition));
        if (quad) {
            for (i32 i = 0; i < 4; i++) {
                auto& vertex = quad->verticies[i];
                vertex.srbIndex     = srbIndex;
                vertex.spriteSheet  = (u32)sheet;
                vertex.colorChannel = colorChannel;
                vertex.opacity      = (object->m_opacityMod2 > 0.0) ? (u8)(object->m_opacityMod2 * 255.0) : 255;
            }
        }
    }

    for (auto child : CCArrayExt<CCSprite*>(sprite->getChildren())) {
        if (child->getZOrder() >= 0)
            writeSprite(object, child, transform, type);
    }
}

void ObjectBatch::finishWriting() {
    quadCount = currentQuadIndex;
    
    std::vector<u32> indicies;
    indicies.resize(quadCount * 6);

    usize vertexIndex = 0;
    for (usize i = 0; i < indicies.size(); i += 6) {
        indicies[i + 0] = vertexIndex + QUAD_BL;
        indicies[i + 1] = vertexIndex + QUAD_TL;
        indicies[i + 2] = vertexIndex + QUAD_TR;
        indicies[i + 3] = vertexIndex + QUAD_BL;
        indicies[i + 4] = vertexIndex + QUAD_TR;
        indicies[i + 5] = vertexIndex + QUAD_BR;
        vertexIndex += 4;
    }

    storeGLStates();

    if (vertexBuffer) {
        Buffer::destroy(vertexBuffer);
        vertexBuffer = nullptr;
    }

    if (indexBuffer) {
        Buffer::destroy(indexBuffer);
        indexBuffer = nullptr;
    }

    vertexBuffer = Buffer::createStaticDraw(quads.data(),    quadCount * sizeof(ObjectQuad));
    indexBuffer  = Buffer::createStaticDraw(indicies.data(), indicies.size() * sizeof(u32));

    prepareVAO();

    restoreGLStates();

    quads.clear();
}

void ObjectBatch::draw() {
    bind();
    glDrawElements(GL_TRIANGLES, indexCount(), GL_UNSIGNED_INT, nullptr);
}

struct AttribTypeInfo {
    i32 openGlType;
    i32 componentCount;
    u32 size;
};

static AttribTypeInfo getInfoOfAttributeTypeString(std::string type) {
    if (type == "float") return { GL_FLOAT, 1, sizeof(float) * 1 };
    if (type == "vec2")  return { GL_FLOAT, 2, sizeof(float) * 2 };
    if (type == "vec3")  return { GL_FLOAT, 3, sizeof(float) * 3 };
    if (type == "vec4")  return { GL_FLOAT, 4, sizeof(float) * 4 };

    if (type == "i8")                   return { GL_BYTE,  1, sizeof(i8)  };
    if (type == "i16")                  return { GL_SHORT, 1, sizeof(i16) };
    if (type == "int" || type == "i32") return { GL_INT,   1, sizeof(i32) };

    if (type == "u8")  return { GL_UNSIGNED_BYTE,  1, sizeof(u8)  };
    if (type == "u16") return { GL_UNSIGNED_SHORT, 1, sizeof(u16) };
    if (type == "u32") return { GL_UNSIGNED_INT,   1, sizeof(u32) };
    
    return { 0, 0 };
}

static void vertexAttribPointer(u32 id, const AttribTypeInfo& info, usize stride, usize offset) {
    if (info.openGlType == GL_FLOAT)
        glVertexAttribPointer(id, info.componentCount, info.openGlType, GL_FALSE, sizeof(ObjectVertex), (void*)offset);
    else if (info.openGlType == GL_DOUBLE)
        glVertexAttribLPointer(id, info.componentCount, info.openGlType, sizeof(ObjectVertex), (void*)offset);
    else
        glVertexAttribIPointer(id, info.componentCount, info.openGlType, sizeof(ObjectVertex), (void*)offset);
}

#define VERTEX_ATTRIBUTE_AS_ATTRIB_POINTER_CALL(ID, TYPE, NAME) \
    { \
        auto info = getInfoOfAttributeTypeString(#TYPE); \
        vertexAttribPointer(ID, info, sizeof(ObjectVertex), offsetof(ObjectVertex, NAME)); \
        glEnableVertexAttribArray(ID); \
    }

void ObjectBatch::prepareVAO() {
    if (vao == 0)
        glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);
    vertexBuffer->bindAs(GL_ARRAY_BUFFER);

    OBJECT_VERTEX_ATTRIBUTES(VERTEX_ATTRIBUTE_AS_ATTRIB_POINTER_CALL)
}