#include "ObjectBatch.hpp"
#include "Renderer.hpp"

#include <string>

using namespace cocos2d;

#define QUAD_BL 0
#define QUAD_BR 1
#define QUAD_TL 2
#define QUAD_TR 3

ObjectBatch::~ObjectBatch() {
    if (vbo)
        glDeleteBuffers(1, &vbo);
    if (ibo)
        glDeleteBuffers(1, &ibo);
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

void ObjectBatch::writeQuad(
    CCAffineTransform absoluteNodeTransform,
    CCPoint innerVertexOffset,
    CCSize contentSize,
    cocos2d::CCRect textureCrop,
    bool textureCropRotated,
    SpriteSheet sheet,
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

    if (sheet != SpriteSheet::GAME_1)
        log::info("{} {}", currentQuadIndex, (u32)sheet);

    for (i32 i = 0; i < 4; i++) {
        quad.verticies[i].spriteSheet = (u32)sheet;
    }

    currentQuadIndex++;
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

    if (vbo == 0)
        glGenBuffers(1, &vbo);

    if (ibo == 0)
        glGenBuffers(1, &ibo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, quadCount * sizeof(ObjectQuad), quads.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicies.size() * sizeof(u32), indicies.data(), GL_STATIC_DRAW);

    prepareVAO();

    restoreGLStates();

    quads.clear();
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

    writeSprite(object, transform, sheet);

    if (object->m_glowSprite) {
        writeSprite(object->m_glowSprite, transform, SpriteSheet::GLOW);
        
        if (object->m_glowSprite->getParent() == object)
            log::warn("Glow sprite of object is child of main sprite after setup");
    }

    if (object->m_hasColorSprite && object->m_colorSprite) {
        writeSprite(object->m_colorSprite, transform, sheet);
        
        if (object->m_colorSprite->getParent() == object)
            log::warn("Color sprite of object is child of main sprite after setup");
    }
}

void ObjectBatch::writeSprite(CCSprite* sprite, CCAffineTransform transform, SpriteSheet sheet) {
    CCTexture2D* texture = renderer->getSpriteSheetTexture(sheet);
    
    transform = CCAffineTransformConcat(sprite->nodeToParentTransform(), transform);
    
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

        writeQuad(
            transform,
            sprite->getOffsetPosition(),
            size,
            crop,
            sprite->isTextureRectRotated(),
            sheet,
            sprite->isFlipX(),
            sprite->isFlipY()
        );
    }

    CCObject* child;
    CCARRAY_FOREACH(sprite->getChildren(), child) {
        writeSprite((CCSprite*)child, transform, sheet);
    }
}

struct AttribTypeInfo {
    i32 openGlType;
    i32 componentCount;
    u32 size;
};

static AttribTypeInfo getInfoOfAttributeTypeString(std::string type) {
    if (type == "float")     return { GL_FLOAT, 1, sizeof(float) * 1 };
    if (type == "glm::vec2") return { GL_FLOAT, 2, sizeof(float) * 2 };
    if (type == "glm::vec3") return { GL_FLOAT, 3, sizeof(float) * 3 };
    if (type == "glm::vec4") return { GL_FLOAT, 4, sizeof(float) * 4 };
    if (type == "int")       return { GL_INT,   1, sizeof(int) * 1 };
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
        vertexAttribPointer(ID, info, sizeof(ObjectVertex), offset); \
        glEnableVertexAttribArray(ID); \
        offset += info.size; \
    }

void ObjectBatch::prepareVAO() {
    if (vao == 0)
        glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    usize offset = 0;
    OBJECT_VERTEX_ATTRIBUTES(VERTEX_ATTRIBUTE_AS_ATTRIB_POINTER_CALL)
}