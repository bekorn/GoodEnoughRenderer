uniform sampler2D color_attachment;

out vec4 out_color;

#define PI 3.14159265359
#define invPI (1.0 / PI)
#define RAD_TO_DEG (180.0 / PI)

float compMax(vec3 v) { return max(max(v.x, v.y), v.z); }
float compMin(vec3 v) { return min(min(v.x, v.y), v.z); }

// ACES implementation from
// https://github.com/google/filament/blob/main/filament/src/ToneMapper.cpp

const mat3 AP1_to_XYZ = {
{ 0.6624541811f, 0.2722287168f, -0.0055746495f },
{ 0.1340042065f, 0.6740817658f, 0.0040607335f },
{ 0.1561876870f, 0.0536895174f, 1.0103391003f }
};

const mat3 XYZ_to_AP1 = {
{1.6410233797f, -0.6636628587f,  0.0117218943f},
{-0.3248032942f,  1.6153315917f, -0.0082844420f},
{-0.2364246952f,  0.0167563477f,  0.9883948585f},
};

mat3 Rec2020_to_XYZ = {
{0.6369530f,  0.2626983f,  0.0000000f},
{0.1446169f,  0.6780088f,  0.0280731f},
{0.1688558f,  0.0592929f,  1.0608272f},
};

const mat3 XYZ_to_Rec2020 = {
{1.7166634f,  -0.6666738f,  0.0176425f},
{-0.3556733f,  1.6164557f, -0.0427770f},
{-0.2533681f,  0.0157683f,  0.9422433f},
};

const mat3 AP0_to_AP1 = {
{1.4514393161f, -0.0765537734f,  0.0083161484f},
{-0.2365107469f,  1.1762296998f, -0.0060324498f},
{-0.2149285693f, -0.0996759264f,  0.9977163014f},
};

const mat3 AP1_to_AP0 = {
{0.6954522414f,  0.0447945634f, -0.0055258826f},
{0.1406786965f,  0.8596711185f,  0.0040252103f},
{0.1638690622f,  0.0955343182f,  1.0015006723f},
};

const vec3 LUMINANCE_AP1 = {0.272229f, 0.674082f, 0.0536895f};

const mat3 AP1_to_Rec2020 = XYZ_to_Rec2020 * AP1_to_XYZ;
const mat3 Rec2020_to_AP0 = AP1_to_AP0 * XYZ_to_AP1 * Rec2020_to_XYZ;

vec3 xyY_to_XYZ(vec3 v) {
    const float a = v.z / max(v.y, 1e-5f);
    return vec3(v.x * a, v.z, (1.0f - v.x - v.y) * a);
}

vec3 XYZ_to_xyY(vec3 v) {
    return vec3(v.xy / max(v.x + v.y + v.z, 1e-5f), v.y);
}

float rgb_2_saturation(vec3 rgb) {
    // Input:  ACES
    // Output: OCES
    float TINY = 1e-5f;
    float mi = compMin(rgb);
    float ma = compMax(rgb);
    return (max(ma, TINY) - max(mi, TINY)) / max(ma, 1e-2f);
}

float rgb_2_yc(vec3 rgb) {
    float ycRadiusWeight = 1.75f;

    // Converts RGB to a luminance proxy, here called YC
    // YC is ~ Y + K * Chroma
    // Constant YC is a cone-shaped surface in RGB space, with the tip on the
    // neutral axis, towards white.
    // YC is normalized: RGB 1 1 1 maps to YC = 1
    //
    // ycRadiusWeight defaults to 1.75, although can be overridden in function
    // call to rgb_2_yc
    // ycRadiusWeight = 1 -> YC for pure cyan, magenta, yellow == YC for neutral
    // of same value
    // ycRadiusWeight = 2 -> YC for pure red, green, blue  == YC for  neutral of
    // same value.

    float r = rgb.r;
    float g = rgb.g;
    float b = rgb.b;

    float chroma = sqrt(b * (b - g) + g * (g - r) + r * (r - b));

    return (b + g + r + ycRadiusWeight * chroma) / 3.0f;
}

float sigmoid_shaper(float x) {
    // Sigmoid function in the range 0 to 1 spanning -2 to +2.
    float t = max(1.0f - abs(x / 2.0f), 0.0f);
    float y = 1.0f + sign(x) * (1.0f - t * t);
    return y / 2.0f;
}

float glow_fwd(float ycIn, float glowGainIn, float glowMid) {
    float glowGainOut;

    if (ycIn <= 2.0f / 3.0f * glowMid) {
        glowGainOut = glowGainIn;
    } else if (ycIn >= 2.0f * glowMid) {
        glowGainOut = 0.0f;
    } else {
        glowGainOut = glowGainIn * (glowMid / ycIn - 1.0f / 2.0f);
    }

    return glowGainOut;
}

float rgb_2_hue(vec3 rgb) {
    // Returns a geometric hue angle in degrees (0-360) based on RGB values.
    // For neutral colors, hue is undefined and the function will return a quiet NaN value.
    float hue = 0.0f;
    // RGB triplets where RGB are equal have an undefined hue
    if (!(rgb.x == rgb.y && rgb.y == rgb.z))
    {
        hue = RAD_TO_DEG * atan(
        sqrt(3.0f) * (rgb.y - rgb.z),
        2.0f * rgb.x - rgb.y - rgb.z
        );
    }
    return (hue < 0.0f) ? hue + 360.0f : hue;
}

float center_hue(float hue, float centerH) {
    float hueCentered = hue - centerH;
    if (hueCentered < -180.0f) {
        hueCentered = hueCentered + 360.0f;
    } else if (hueCentered > 180.0f) {
        hueCentered = hueCentered - 360.0f;
    }
    return hueCentered;
}

vec3 darkSurround_to_dimSurround(vec3 linearCV) {
    float DIM_SURROUND_GAMMA = 0.9811f;

    vec3 XYZ = AP1_to_XYZ * linearCV;
    vec3 xyY = XYZ_to_xyY(XYZ);

    xyY.z = clamp(xyY.z, 0.0f, 1.0/0.0);
    xyY.z = pow(xyY.z, DIM_SURROUND_GAMMA);

    XYZ = xyY_to_XYZ(xyY);
    return XYZ_to_AP1 * XYZ;
}

vec3 ACES(vec3 color, float brightness) {
    // Some bits were removed to adapt to our desired output

    // "Glow" module constants
    float RRT_GLOW_GAIN = 0.05f;
    float RRT_GLOW_MID = 0.08f;

    // Red modifier constants
    float RRT_RED_SCALE = 0.82f;
    float RRT_RED_PIVOT = 0.03f;
    float RRT_RED_HUE   = 0.0f;
    float RRT_RED_WIDTH = 135.0f;

    // Desaturation constants
    float RRT_SAT_FACTOR = 0.96f;
    float ODT_SAT_FACTOR = 0.93f;

    vec3 ap0 = Rec2020_to_AP0 * color;

    // Glow module
    float saturation = rgb_2_saturation(ap0);
    float ycIn = rgb_2_yc(ap0);
    float s = sigmoid_shaper((saturation - 0.4f) / 0.2f);
    float addedGlow = 1.0f + glow_fwd(ycIn, RRT_GLOW_GAIN * s, RRT_GLOW_MID);
    ap0 *= addedGlow;

    // Red modifier
    float hue = rgb_2_hue(ap0);
    float centeredHue = center_hue(hue, RRT_RED_HUE);
    float hueWeight = smoothstep(0.0f, 1.0f, 1.0f - abs(2.0f * centeredHue / RRT_RED_WIDTH));
    hueWeight *= hueWeight;

    ap0.r += hueWeight * saturation * (RRT_RED_PIVOT - ap0.r) * (1.0f - RRT_RED_SCALE);

    // ACES to RGB rendering space
    vec3 ap1 = clamp(AP0_to_AP1 * ap0, 0.0f, 1.0/0.0);

    // Global desaturation
    ap1 = mix(vec3(dot(ap1, LUMINANCE_AP1)), ap1, RRT_SAT_FACTOR);

    // NOTE: This is specific to Filament and added only to match ACES to our legacy tone mapper
    //       which was a fit of ACES in Rec.709 but with a brightness boost.
    ap1 *= brightness;

    // Fitting of RRT + ODT (RGB monitor 100 nits dim) from:
    // https://github.com/colour-science/colour-unity/blob/master/Assets/Colour/Notebooks/CIECAM02_Unity.ipynb
    float a = 2.785085f;
    float b = 0.107772f;
    float c = 2.936045f;
    float d = 0.887122f;
    float e = 0.806889f;
    vec3 rgbPost = (ap1 * (a * ap1 + b)) / (ap1 * (c * ap1 + d) + e);

    // Apply gamma adjustment to compensate for dim surround
    vec3 linearCV = darkSurround_to_dimSurround(rgbPost);

    // Apply desaturation to compensate for luminance difference
    linearCV = mix(vec3(dot(linearCV, LUMINANCE_AP1)), linearCV, ODT_SAT_FACTOR);

    return AP1_to_Rec2020 * linearCV;
}


// ACES aproximation from
// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
const mat3x3 ACESInputMat = {
{ 0.59719, 0.07600, 0.02840 },
{ 0.35458, 0.90834, 0.13383 },
{ 0.04823, 0.01566, 0.83777 }
};


// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3x3 ACESOutputMat = {
{ 1.60475, -0.10208, -0.00327 },
{ -0.53108, 1.10813, -0.07276 },
{ -0.07367, -0.00605, 1.07602 }
};

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

vec3 ACESFitted(vec3 color)
{
    color = ACESInputMat * color;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = ACESOutputMat * color;

    color = clamp(color, 0, 1);

    return color;
}

// Reinhard
vec3 Reinhard_simple(vec3 color)
{
    return color / (1 + color);
}

void main()
{
    ivec2 texel_idx = ivec2(gl_FragCoord.xy);
    vec4 hdr_color = texelFetch(color_attachment, texel_idx, 0);

//    hdr_color.rgb = Reinhard_simple(hdr_color.rgb);

//    hdr_color.rgb = ACESFitted(hdr_color.rgb);

    hdr_color.rgb = ACES(hdr_color.rgb, 1.0/0.6);
//    hdr_color.rgb = ACES(hdr_color.rgb, 1.0);

    out_color = hdr_color;
}
