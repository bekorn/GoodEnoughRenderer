
const float PI = 3.14159265359;
const float invPI = 1 / PI;

float dot_01(vec3 a, vec3 b)
{ return clamp(dot(a, b), 0, 1); }

float RAND12(vec2 p)
{ return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }

mat3x3 get_tangent_to_world(vec3 N)
{
    // forward = N
    vec3 up = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 right = normalize(cross(N, up));
    up = cross(N, -right);
    return mat3x3(right, up, N);
}

vec3 get_dir(float lat, float lon)
{
    return vec3(
        sin(lat) * cos(lon),
        sin(lat) * sin(lon),
        cos(lat)
    );
}

vec2 Hammersley(uint i, uint N)
{
    // Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10;

    return vec2(float(i) / float(N), rdi);
}
