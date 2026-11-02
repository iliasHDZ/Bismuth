in vec2 t_radiusVector;
in vec2 t_texCoord;

out vec4 fragColor;

uniform sampler2D u_gradient;
uniform sampler2D u_gradientDark;

uniform int u_outline;

#define FUZZ 0.1

#define OUTLINE_OUT (14.0 / 20.0)
#define OUTLINE_IN  (10.0 / 20.0)

float map(float value, float min1, float max1, float min2, float max2) {
    return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float radius;

vec4 background() {
    float alpha = clamp(map(radius, 1.0, 1.0 + FUZZ, 1.0, 0.0), 0.0, 1.0);
    
    return vec4(texture(u_gradientDark, t_texCoord).rgb, alpha);
}

vec4 outline() {
    float outAlpha = clamp(map(radius, OUTLINE_OUT, OUTLINE_OUT + FUZZ, 1.0, 0.0), 0.0, 1.0);
    float inAlpha  = clamp(map(radius, OUTLINE_IN - FUZZ, OUTLINE_IN, 0.0, 1.0), 0.0, 1.0);

    return vec4(texture(u_gradient, t_texCoord).rgb, min(outAlpha, inAlpha));
}

void main() {
    radius = dot(t_radiusVector, t_radiusVector);

    if (u_outline != 0)
        fragColor = outline();
    else
        fragColor = background();
}