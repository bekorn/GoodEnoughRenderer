
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
