#version 450 core

out vec4 fColour;

in vec2 uv;

uniform sampler2D accumTexture;
uniform vec2 renderScale;
uniform bool isMoving;
uniform vec2 resolution;

const float EXPOSURE = 2.0;

// Crosshair dimensions (in pixels)
const float halfThickness = 1.0; // Half the thickness (in pixels)
const float lenPercent = 0.01; // Length of one line (as % of screen)
const float gapPercent = 0.0028; // Gap in the middle (as % of screen)


const vec3 IGN = vec3(0.06711056, 0.00583715, 52.9829189);
float interleavedGradientNoise(vec2 fragCoord) {
    return fract(IGN.z * fract(dot(fragCoord, IGN.xy)));
}

float isCrosshair() {
    float len = resolution.y * lenPercent;
    float gap = resolution.y * gapPercent;

    vec2 diff = abs(resolution * (uv - 0.5));
    vec2 hStep = step(diff.yx, vec2(halfThickness, len)) * step(gap, diff.x);
    vec2 vStep = step(diff.xy, vec2(halfThickness, len)) * step(gap, diff.y);
    return clamp(hStep.x * hStep.y + vStep.x * vStep.y, 0.0, 1.0);
}

void main() {
    vec2 maxUV = renderScale - (0.5 / resolution);
    vec2 safeUV = min(uv * renderScale, maxUV);

    vec3 colour = texture(accumTexture, safeUV).rgb * EXPOSURE;

    // Tonemapping (ACES Narkowicz Fit)
    vec3 a = fma(colour, vec3(2.51), vec3(0.03));
    vec3 b = fma(colour, vec3(2.43), vec3(0.59));
    colour = (colour * a) / fma(colour, b, vec3(0.14));
    colour = clamp(colour, 0.0, 1.0);

    // Gamma correction
    colour = sqrt(colour); // sqrt=1/2 close enough to 1/2.2

    // Dithering
    float dither = interleavedGradientNoise(gl_FragCoord.xy) - 0.5;
    colour += dither * 0.00392156863; // * (1/255)

    // Crosshair
    if (isMoving) {
        float crossMask = isCrosshair();
        colour = mix(colour, 1.0 - colour, crossMask);
    }

    fColour = vec4(colour, 1.0);
}