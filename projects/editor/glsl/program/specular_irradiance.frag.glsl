in vec2 uv;

uniform mat4x3 view_dirs;
uniform samplerCube environment;
uniform float roughness;

out vec4 out_color;

const float PI = 3.14159265359;
const float invPI = 1.0 / PI;

const float a = roughness * roughness;

float dot_01(vec3 a, vec3 b)
{ return clamp(dot(a, b), 0, 1); }

vec2 Hammersley(uint i, uint N)
{
    // Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10;

    return vec2(float(i) /float(N), rdi);
}

float distribution__Trowbridge_Reitz_GGX(float dot_HN)
{
    float f = a / (dot_HN*dot_HN * (a*a - 1) + 1);
    return f*f * invPI;
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N)
{
    // sample in spherical coordinates
    float phi = 2 * PI * Xi.x;
    float cosTheta = sqrt((1 - Xi.y) / (1 + (a*a - 1) * Xi.y));
    float sinTheta = sqrt(1 - cosTheta*cosTheta);

    // to tangent space
    vec3 H = vec3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );

    // to world-space
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

    if (roughness == 0) // mirror-reflection
    {
        out_color = textureLod(environment, view_dir, 0);
        return;
    }

    const vec3 N = normalize(view_dir);
    const vec3 R = N;
    const vec3 V = N;

    const ivec2 environment_size = textureSize(environment, 0);
    const float environment_area = 6 * environment_size.x * environment_size.y;
    const float texel_solidangle = 4*PI / environment_area;

    vec3 irradiance = vec3(0);
    float total_weight = 0;

    uint sample_count = 1024;
    for (uint s = 0; s < sample_count; ++s)
    {
        vec2 Xi = Hammersley(s, sample_count);
        vec3 H = ImportanceSampleGGX(Xi, N);
        float dot_VH = dot(V, H);
        vec3 L = reflect(-V, H);
        float dot_NL = dot(N, L);

        if (dot_NL > 0)
        {
            float dot_NH = dot(N, H);

            // pick correct mipmap level
            float D = distribution__Trowbridge_Reitz_GGX(dot_NH);
            float pdf = D / 4;
            float sample_solidangle = 1 / (float(sample_count) * pdf + 0.001);
            float level = 0.5 * log2(sample_solidangle / texel_solidangle);

            irradiance += textureLod(environment, L, level).rgb * dot_NL;
            total_weight += dot_NL;
        }
    }

    irradiance /= total_weight;

    out_color = vec4(irradiance, 1);
}