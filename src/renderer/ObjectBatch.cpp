#include "ObjectBatch.hpp"
#include "Renderer.hpp"

#include <string>

using namespace cocos2d;

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
    if (object->m_hasColorSprite && object->m_colorSprite)
        reservedQuadCount += countSelfAndDesendantsIfDraw(object->m_colorSprite);
}

void ObjectBatch::allocateReservations() {
    quads.resize(reservedQuadCount);
    currentQuadIndex = 0;
}

ObjectQuad& ObjectBatch::writeQuad(
    CCAffineTransform absoluteNodeTransform,
    CCPoint innerVertexOffset,
    CCSize contentSize,
    cocos2d::CCRect textureCrop,
    bool textureCropRotated,
    bool flipX, bool flipY
) {
    auto& quad = quads[currentQuadIndex];
    
    float x1 = innerVertexOffset.x;
    float y1 = innerVertexOffset.y;

    float x2 = x1 + contentSize.width;
    float y2 = y1 + contentSize.height;
    float x = absoluteNodeTransform.tx;
    float y = absoluteNodeTransform.ty;

    float cr  =  absoluteNodeTransform.a;
    float sr  =  absoluteNodeTransform.b;
    float cr2 =  absoluteNodeTransform.d;
    float sr2 = -absoluteNodeTransform.c;
    float ax = x1 * cr - y1 * sr2 + x;
    float ay = x1 * sr + y1 * cr2 + y;

    float bx = x2 * cr - y1 * sr2 + x;
    float by = x2 * sr + y1 * cr2 + y;

    float cx = x2 * cr - y2 * sr2 + x;
    float cy = x2 * sr + y2 * cr2 + y;

    float dx = x1 * cr - y2 * sr2 + x;
    float dy = x1 * sr + y2 * cr2 + y;

    quad.verticies[QUAD_BL].position = glm::vec2(ax, ay);
    quad.verticies[QUAD_BR].position = glm::vec2(bx, by);
    quad.verticies[QUAD_TL].position = glm::vec2(dx, dy);
    quad.verticies[QUAD_TR].position = glm::vec2(cx, cy);

    float left   = textureCrop.origin.x;
    float right  = textureCrop.origin.x + textureCrop.size.width;
    float top    = textureCrop.origin.y;
    float bottom = textureCrop.origin.y + textureCrop.size.height;

    if (textureCropRotated) {
        if (flipX) std::swap(top, bottom);
        if (flipY) std::swap(left, right);

        quad.verticies[QUAD_BL].texCoord = glm::vec2(left,  top);
        quad.verticies[QUAD_BR].texCoord = glm::vec2(left,  bottom);
        quad.verticies[QUAD_TL].texCoord = glm::vec2(right, top);
        quad.verticies[QUAD_TR].texCoord = glm::vec2(right, bottom);
    } else {
        if (flipX) std::swap(left, right);
        if (flipY) std::swap(top, bottom);

        quad.verticies[QUAD_BL].texCoord = glm::vec2(left,  bottom);
        quad.verticies[QUAD_BR].texCoord = glm::vec2(right, bottom);
        quad.verticies[QUAD_TL].texCoord = glm::vec2(left,  top);
        quad.verticies[QUAD_TR].texCoord = glm::vec2(right, top);
    }

    currentQuadIndex++;
    return quad;
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

    SpriteSheet sheet = (SpriteSheet)object->getParentMode();

    /*
    log::info("OBJECT {}", (void*)object);
    log::info("- isObjectBlack: {}", object->m_isObjectBlack);
    log::info("- isColorSpriteBlack: {}", object->m_isColorSpriteBlack);
    log::info("- hasColorSprite: {}", object->m_hasColorSprite);
    log::info("- hasCustomChild: {}", object->m_hasCustomChild);

    log::info("- mainSprite:");
    */
    writeSprite(object, transform, sheet, object->m_activeMainColorID, object->m_isObjectBlack, object->m_isColorSpriteBlack, object->m_colorSprite);


    if (object->m_glowSprite) {
        // log::info("- glowSprite:");
        writeSprite(object->m_glowSprite, transform, SpriteSheet::GLOW, object->m_activeMainColorID, object->m_isObjectBlack, false);
        
        if (object->m_glowSprite->getParent() == object)
            log::warn("Glow sprite of object is child of main sprite after setup");
    }

    if (object->m_hasColorSprite && object->m_colorSprite) {
        // log::info("- colorSprite:");
        writeSprite(object->m_colorSprite, transform, sheet, object->m_activeDetailColorID, object->m_isColorSpriteBlack, object->m_isColorSpriteBlack);
        
        if (object->m_colorSprite->getParent() == object)
            log::warn("Color sprite of object is child of main sprite after setup");
    }
}

void ObjectBatch::writeSprite(
    cocos2d::CCSprite* sprite,
    cocos2d::CCAffineTransform transform,
    SpriteSheet sheet,
    u32 colorChannel,
    bool isBlack,
    bool isChildrenBlack,
    cocos2d::CCSprite* childToIgnore
) {
    CCTexture2D* texture = renderer->getSpriteSheetTexture(sheet);
    
    transform = CCAffineTransformConcat(sprite->nodeToParentTransform(), transform);

    CCObject* child;
    CCARRAY_FOREACH(sprite->getChildren(), child) {
        auto sprite = (CCSprite*)child;
        if (sprite->getZOrder() < 0 && sprite != childToIgnore)
            writeSprite(sprite, transform, sheet, colorChannel, isChildrenBlack, isChildrenBlack, childToIgnore);
    }
    
    if (texture && !sprite->getDontDraw()) {
        CCPoint relativeOffset = sprite->getUnflippedOffsetPosition();

        if (sprite->isFlipX())
            relativeOffset.x = -relativeOffset.x;
        if (sprite->isFlipY())
            relativeOffset.y = -relativeOffset.y;

        u32 texWidth  = texture->getPixelsWide();
        u32 texHeight = texture->getPixelsHigh();

        CCRect crop = sprite->getTextureRect();

        CCSize size = crop.size;

        if (sprite->isTextureRectRotated()) {
            float temp = crop.size.height;
            crop.size.height = crop.size.width;
            crop.size.width  = temp;
        }

        float contentScale = CCDirector::get()->getContentScaleFactor();

        crop.origin.x    *= contentScale / texWidth;
        crop.origin.y    *= contentScale / texHeight;
        crop.size.width  *= contentScale / texWidth;
        crop.size.height *= contentScale / texHeight;

        // log::info("  - {}SPRITE", isBlack ? "BLACK " : "");

        auto& quad = writeQuad(
            transform,
            sprite->getOffsetPosition(),
            size,
            crop,
            sprite->isTextureRectRotated(),
            sprite->isFlipX(),
            sprite->isFlipY()
        );

        for (i32 i = 0; i < 4; i++) {
            auto& vertex = quad.verticies[i];
            vertex.spriteSheet  = (u32)sheet;
            vertex.colorChannel = isBlack ? COLOR_CHANNEL_BLACK : colorChannel;
        }
    }

    CCARRAY_FOREACH(sprite->getChildren(), child) {
        auto sprite = (CCSprite*)child;
        if (sprite->getZOrder() >= 0 && sprite != childToIgnore)
            writeSprite(sprite, transform, sheet, colorChannel, isChildrenBlack, isChildrenBlack, childToIgnore);
    }
}

void ObjectBatch::finishWriting() {
    quadCount = currentQuadIndex;
    
    std::vector<u32> indicies;
    indicies.resize(quadCount * 6);

    log::info("offset: {}", offsetof(ObjectVertex, spriteSheet));

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