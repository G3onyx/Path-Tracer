#version 450 core

out vec4 fragColour;

in vec3 worldPos;

uniform vec3 camPos;
uniform sampler2D accumTexture;
uniform vec2 resolution;
uniform vec2 renderScale;
uniform bool isEndPoint;
uniform bool isHighlighted;

void main() {
    vec2 uv = (gl_FragCoord.xy / resolution) * renderScale;

    vec2 texSize = vec2(textureSize(accumTexture, 0));
    vec2 maxUV = renderScale - (0.5 / texSize);
    vec2 safeUV = min(uv, maxUV);

    float depth = texture(accumTexture, safeUV).a;

    vec3 offset = worldPos - camPos;
    float fragDepthSq = dot(offset, offset);

    if (fragDepthSq > depth * depth) discard;
    
    vec3 colour = vec3(1.0, 0.0, 0.0);
    if (isEndPoint) colour = vec3(0.0, 1.0, 0.0);
    if (isHighlighted) colour = vec3(1.0, 1.0, 0.0);

    fragColour = vec4(colour, 1.0);
}