in Vertex
{
    vec3 camera_position;
    vec3 object_position;
    vec3 world_position;
    vec3 normal;
    vec3 color;
    vec2 texcoord;
} vertex;

uniform uint64_t environment_map_handle;

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

    metallic = metalic_rouhgness[0];
    roughness = metalic_rouhgness[1];
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

void main()
{
    vec3 normal = get_normal();

    vec3 base_color = get_base_color();
    float occlusion = get_occlusion();

    float metallic, roughness;
    get_metallic_roughness(metallic, roughness);

    vec3 diffuse_color = vec3(0);
    for (int i = 0; i < Lights.length(); ++i)
    {
        Light light = Lights[i];
        if (! light.is_active)
            continue;

        vec3 to_light = light.position - vertex.world_position;
        float dist_sqr = dot(to_light, to_light);

        float intensity = dot(normal, normalize(to_light));
        intensity *= occlusion;
        intensity *= light.intensity / dist_sqr;

        diffuse_color += light.color * max(0, intensity);
    }
    diffuse_color *= base_color;

    samplerCube environment_map = samplerCube(environment_map_handle);
    float level = roughness * textureQueryLevels(environment_map);
    vec3 to_environment = reflect(vertex.world_position - CameraWorldPosition, normal);
    vec3 ambient_color = base_color * textureLod(environment_map, to_environment, level).rgb;

    vec3 emission_color = get_emission();

    out_color = vec4(ambient_color + diffuse_color + emission_color, 1);
}
