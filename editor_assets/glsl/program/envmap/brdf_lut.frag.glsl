in vec2 uv;

out vec4 out_color;

float geometry__Schlick_GGX_IBL(float dot_NV, float a)
{
    float k = (a * a) / 2;
    return dot_NV / (dot_NV * (1 - k) + k);
}
float geometry__Smith(float dot_NV, float dot_NL, float a)
{
    return geometry__Schlick_GGX_IBL(dot_NV, a) * geometry__Schlick_GGX_IBL(dot_NL, a);
}

mat3x3 tangent_to_world;
float phi_offset;
float a;
void compute_importance_sampling_constants(vec3 N, float roughness)
{
    tangent_to_world = get_tangent_to_world(N);
    phi_offset = RAND12(uv);
    a = roughness * roughness;
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
    // can be simplified to this because N is constant, but the compiler already does it I guess
    //return normalize(H.yxz);
}

void main()
{
    const float dot_NV = uv.x;
    const float roughness = uv.y;

    const vec3 N = vec3(0, 0, 1);
    const vec3 V = vec3(sqrt(1 - dot_NV * dot_NV), 0, dot_NV); // because dot(N, V) == dot_NV

    compute_importance_sampling_constants(N, roughness);

    float A = 0, B = 0;

    const uint sample_count = 1024;
    for (uint s = 0; s < sample_count; ++s)
    {
        vec2 Xi = Hammersley(s, sample_count);
        vec3 H = ImportanceSampleGGX(Xi);
        float dot_VH = dot(V, H);
        vec3 L = reflect(-V, H);
        float dot_NL = dot(N, L);
        float dot_NH = dot(N, H);

        if (dot_NL > 0)
        {
            float G = geometry__Smith(dot_NV, dot_NL, roughness);
            float G_Vis = (G * dot_VH) / (dot_NH * dot_NV);
            float Fc = pow(1 - dot_VH, 5);

            A += (1 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    out_color.rg = vec2(A, B) / sample_count;
}