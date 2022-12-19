in vec2 uv;

uniform mat4x3 view_dirs;
uniform samplerCube environment;
uniform float roughness;

out vec4 out_color;

const float PI = 3.14159265359;
const float invPI = 1.0 / PI;

const float a = roughness * roughness;

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N)
{
    float phi = 2 * PI * Xi.x;
    float cosTheta = sqrt((1 - Xi.y) / ((a*a - 1) * Xi.y + 1));
    float sinTheta = sqrt(1 - cosTheta*cosTheta);

    // from spherical coordinates to cartesian coordinates
    vec3 H = vec3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );

    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 tangent   = normalize(cross(N, up));
    vec3 bitangent = cross(tangent, N);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

void main()
{
    // view_dirs[0] => uv 0,0
    // view_dirs[1] => uv 1,0
    // view_dirs[2] => uv 0,1
    // view_dirs[3] => uv 1,1
    vec3 view_dir = mix(
        mix(view_dirs[0], view_dirs[1], uv.x),
        mix(view_dirs[2], view_dirs[3], uv.x),
        uv.y
    );

    vec3 N = normalize(view_dir);
    vec3 R = N;
    vec3 V = N;

    vec3 irradiance = vec3(0);
    float total_weight = 0;

    uint sample_count = 128;
    for (uint s = 0; s < sample_count; ++s)
    {
        vec2 Xi = Hammersley(s, sample_count);
        vec3 H  = ImportanceSampleGGX(Xi, N);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);
        float dot_NL = max(0, dot(N, L));

        if (dot_NL > 0)
        {
            irradiance += textureLod(environment, L, 0).rgb;
            total_weight += dot_NL;
        }
    }

    irradiance /= total_weight;

    out_color = vec4(irradiance, 1);
}