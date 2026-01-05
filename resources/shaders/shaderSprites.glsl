////////////////////////
//// SHADER SPRITES ////
////////////////////////

vec4 shaderSpriteSolidBlock(vec2 pos) {
    return vec4(1.0, 1.0, 1.0, 1.0);
}

vec4 shaderSpriteSoildSlope(vec2 pos) {
    float a = ( (1.0 - pos.x + pos.y) < 1.0 ) ? 1.0 : 0.0;
    return vec4(a, a, a, a);
}

////////////////////////////////////////
//// SHADER SPRITE SAMPLER FUNCTION ////
////////////////////////////////////////

vec4 sampleShaderSprite(uint id, vec2 pos) {
    switch (id) {
    case SHADER_SPRITE_SOLID_BLOCK: return shaderSpriteSolidBlock(pos);
    case SHADER_SPRITE_SOLID_SLOPE: return shaderSpriteSoildSlope(pos);
    }
    
    return vec4(0.0, 0.0, 0.0, 0.0);
}