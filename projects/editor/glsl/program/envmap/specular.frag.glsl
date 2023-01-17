in vec2 uv;

uniform mat4x3 view_dirs;
uniform samplerCube environment;
uniform float roughness;

out vec4 out_color;

const float a = roughness * roughness;

float distribution__Trowbridge_Reitz_GGX(float dot_HN)
{
    float f = a / (dot_HN*dot_HN * (a*a - 1) + 1);
    return f*f * invPI;
}

mat3x3 tangent_to_world;
float phi_offset;
void compute_importance_sampling_constants(vec3 N)
{
    tangent_to_world = get_tangent_to_world(N);
    phi_offset = RAND12(uv);
}
vec3 ImportanceSampleGGX(vec2 Xi)
{
    // sample in spherical coordinates
    float phi = 2 * PI * (Xi.x + phi_offset);
    float cosTheta = sqrt((1 - Xi.y) / (1 + (a*a - 1) * Xi.y));
    float sinTheta = sqrt(1 - cosTheta*cosTheta);

    // to tangent space
    vec3 H = vec3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );

    // to world-space
    return normalize(tangent_to_world * H);
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
    const vec3 V = N;

    compute_importance_sampling_constants(N);

    const ivec2 environment_size = textureSize(environment, 0);
    const float environment_area = 6 * environment_size.x * environment_size.y;
    const float texel_solidangle = 4*PI / environment_area;

    vec3 irradiance = vec3(0);
    float total_weight = 0;

    const uint sample_count = 1024;
    for (uint s = 0; s < sample_count; ++s)
    {
        vec2 Xi = Hammersley(s, sample_count);
        vec3 H = ImportanceSampleGGX(Xi);
        float dot_VH = dot(V, H);
        vec3 L = reflect(-V, H);
        float dot_NL = dot(N, L);

        if (dot_NL > 0)
        {
            // pick correct mipmap level
            float dot_NH = dot_VH; // N == V
            float D = distribution__Trowbridge_Reitz_GGX(dot_NH);
            float pdf = D / 4;
            float sample_solidangle = 1 / (float(sample_count) * pdf);
            float level = 0.5 * log2(sample_solidangle / texel_solidangle);

            irradiance += textureLod(environment, L, level).rgb * dot_NL;
            total_weight += dot_NL;
        }
    }

    irradiance /= total_weight;

    out_color = vec4(irradiance, 1);
}