in Vertex
{
    vec3 object_position;
    vec3 world_position;
    vec3 normal;
    vec3 color;
    vec2 texcoord;
} vertex;

uniform samplerCube envmap_diffuse;
uniform samplerCube envmap_specular;
uniform sampler2D envmap_brdf_lut;

layout(binding = 3) readonly buffer Material
{
    uint64_t base_color_texture_handle;
    vec4     base_color_factor;

    uint64_t metallic_roughness_texture_handle;
    vec2     metallic_roughness_factor;

    uint64_t emissive_texture_handle;
    vec3     emissive_factor;

    uint64_t occlusion_texture_handle;
    uint64_t normal_texture_handle;
} material;

out vec4 out_color;


bool is_handle_set(uint64_t handle)
{ return handle != 0; }

bool is_factor_bound(vec4 factor)
{
    // TODO(bekorn): this introduces a convention: col.a == 0 means no color set
    return factor.a != 0;
}


vec3 get_base_color()
{
    vec3 base_color = vec3(1);

    if (is_handle_set(material.base_color_texture_handle))
    {
        base_color *= texture(sampler2D(material.base_color_texture_handle), vertex.texcoord).rgb;
    }

    // TODO(bekorn): gltf specifies that baseColor.a is used for actual alpha values, so this check does not work on baseColor
    if (is_factor_bound(material.base_color_factor))
    {
        base_color *= material.base_color_factor.rgb;
    }

    return base_color;
}

void get_metallic_roughness(out float metallic, out float roughness)
{
    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_material_pbrmetallicroughness_metallicroughnesstexture
    vec2 metalic_rouhgness;

    if (is_handle_set(material.metallic_roughness_texture_handle))
    {
        metalic_rouhgness = texture(sampler2D(material.metallic_roughness_texture_handle), vertex.texcoord).bg;
    }
    else
    {
        metalic_rouhgness = material.metallic_roughness_factor;
    }

    metallic = clamp(metalic_rouhgness[0], 0, 1);
    roughness = clamp(metalic_rouhgness[1], 0, 1);
}

vec3 get_emission()
{
    if (is_handle_set(material.emissive_texture_handle))
    {
        return texture(sampler2D(material.emissive_texture_handle), vertex.texcoord).rgb;
    }
    else
    {
        return material.emissive_factor;
    }
}

float get_occlusion()
{
    if (is_handle_set(material.occlusion_texture_handle))
    {
        return texture(sampler2D(material.occlusion_texture_handle), vertex.texcoord).r;
    }
    else
    {
        return 1;
    }
}

vec3 get_normal()
{
    vec3 normal = normalize(vertex.normal);

    if (is_handle_set(material.normal_texture_handle))
    {
        //        vec3 normal_tex = normalize(texture(normal_sampler, vertex.texcoord).rgb * 2 - 1);
        // TODO(bekorn): implement normal mapping (prerequisite: bi/tangent space attributes)
    }

    return normal;
}

const float PI = 3.14159265359;
const float invPI = 1 / PI;

float dot_01(vec3 v0, vec3 v1)
{ return clamp(dot(v0, v1), 0, 1); }

vec3 diffuse_factor__Lambert(vec3 color)
{
    return color * invPI;
}

float distribution__Trowbridge_Reitz_GGX(float dot_HN, float a)
{
    float f = a / (dot_HN*dot_HN * (a*a - 1) + 1);
    return f*f * invPI;
}

float geometry__Schlick_GGX(float dot_NV, float a)
{
    return dot_NV / (dot_NV * (1-a) + a);
}

float geometry__Smith(float dot_NV, float dot_NL, float a)
{
    return geometry__Schlick_GGX(dot_NV, a) * geometry__Schlick_GGX(dot_NL, a);
}

vec3 fresnel__Schlick(float dot_HV, vec3 f0)
{
    return f0 + (1 - f0) * pow(1 - dot_HV, 5);
}

vec3 fresnel__Schlick_Roughness(float dot_NV, vec3 f0, float roughness)
{
    return f0 + (max(vec3(1 - roughness), f0) - f0) * pow(clamp(1 - dot_NV, 0, 1), 5);
}

void main()
{
    vec3 normal = get_normal();

    vec3 base_color = get_base_color();
    float occlusion = get_occlusion();

    float metallic, roughness;
    get_metallic_roughness(metallic, roughness);

    // BRDF Cook-Torrance
    vec3 L_out = vec3(0);

    float a = roughness * roughness;
//    float ior = 1.5;
//    vec3 f0 = vec3(pow((1 - ior) / (1 + ior), 2)); // = 0.04
    vec3 f0 = mix(vec3(0.04), base_color, metallic);

    vec3 N = normal;
    vec3 V = normalize(CameraWorldPosition - vertex.world_position);
    float dot_NV = dot(N, V);

    for (int i = 0; i < Lights.length(); ++i)
    {
        Light light = Lights[i];
        if (! light.is_active)
            continue;

        vec3 L = normalize(light.position - vertex.world_position);
        vec3 H = normalize(V + L);
        float dot_NL = dot(N, L);
        float dot_HL = dot(H, L);
        float dot_HV = dot(H, V);
        float dot_HN = dot(H, N);

        if (dot_NL > 0.0)
        {
            float attenuation = light.range / length(light.position - vertex.world_position);
            vec3 L_in = light.color * attenuation;

            vec3 specular_intensity = fresnel__Schlick(dot_HV, f0);

            float fD = distribution__Trowbridge_Reitz_GGX(dot_HN, a);
            float fG = geometry__Smith(dot_NV, dot_NL, (a+1) * (a+1) / 8);// for direct illumination
//            float fG = geometry__Smith(dot_NV, dot_NL,  a    *  a    / 2); // for image-based illumination

            vec3 specular_color = vec3(fD * fG / (4 * dot_NV * dot_NL));

            vec3 diffuse_intensity = mix(1 - specular_intensity, vec3(0), metallic);

            vec3 diffuse_color = diffuse_factor__Lambert(base_color);

            vec3 f_r = diffuse_intensity * diffuse_color + specular_intensity * specular_color;

            L_out += f_r * L_in * dot_NL;
        }
    }

    // environmental light
    {
        vec3 specular_intensity = fresnel__Schlick_Roughness(dot_NV, f0, roughness);
        vec3 L = reflect(-V, N);
        float level = roughness * textureQueryLevels(envmap_specular);
        vec3 specular_color = textureLod(envmap_specular, L, level).rgb;

        vec3 diffuse_intensity = mix(1 - specular_intensity, vec3(0), metallic);
        vec3 diffuse_color = base_color * texture(envmap_diffuse, N).rgb;

        vec2 env_brdf = texture(envmap_brdf_lut, vec2(dot_NV, roughness)).rg;
        L_out += diffuse_intensity * diffuse_color + specular_color * (specular_intensity * env_brdf.x + env_brdf.y);
    }

    vec3 emission_color = get_emission();

    out_color = vec4(emission_color + L_out, 1);
}
