in Vertex
{
    vec3 camera_position;
    vec3 object_position;
    vec3 world_position;
    vec3 normal;
    vec3 color;
    vec2 texcoord;
} vertex;


layout(binding = 0)  uniform sampler2D base_color_sampler;
layout(location = 0) uniform vec4      base_color_factor;

layout(binding = 1) uniform sampler2D metallic_roughness_sampler;
layout(binding = 2) uniform sampler2D emissive_sampler;
layout(binding = 3) uniform sampler2D occlusion_sampler;
layout(binding = 4) uniform sampler2D normal_sampler;

out vec4 out_color;


bool is_sampler_bound(sampler2D sampler)
{
    // I believe these are Dynamically Uniform Expressions, e.i. no branching among the warp
    return textureSize(sampler, 0).x > 1;
    //    return textureQueryLevels(sampler) != 0; // this does not work with my nvidia drivier :/
}

bool is_factor_bound(vec4 factor)
{
    // same as above
    // TODO(bekorn): this introduces a convention: col.a == 0 means no color set
    return factor.a != 0;
}


vec3 get_base_color()
{
    vec3 base_color = vec3(1);

    if (is_sampler_bound(base_color_sampler))
    {
        base_color *= texture(base_color_sampler, vertex.texcoord).rgb;
    }

    // TODO(bekorn): gltf specifies that baseColor.a is used for actual alpha values, so this check does not work on baseColor
    if (is_factor_bound(base_color_factor))
    {
        base_color *= base_color_factor.rgb;
    }

    return base_color;
}

vec3 get_emission()
{
    if (is_sampler_bound(emissive_sampler))
    {
        return texture(emissive_sampler, vertex.texcoord).rgb;
    }
    else
    {
        return vec3(0);
    }
}

float get_occlusion()
{
    if (is_sampler_bound(occlusion_sampler))
    {
        return texture(occlusion_sampler, vertex.texcoord).r;
    }
    else
    {
        return 1;
    }
}

vec3 get_normal()
{
    vec3 normal = normalize(vertex.normal);

    if (is_sampler_bound(normal_sampler))
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

    vec3 diffuse_color = vec3(0);
    for (int i = 0; i < 4; ++i)
    {
        Light light = Lights[i];
        if (! light.is_active)
            continue;

        vec3 to_light = light.position - vertex.world_position;
        float dist_sqr = dot(to_light, to_light);
        float dist = sqrt(dist_sqr);
        vec3 dir = normalize(to_light);

        float intensity = dot(normal, dir);
        intensity *= occlusion;
        intensity *= clamp(light.intensity / (0.3 * dist + 0.5 * dist_sqr), 0, 1);

        diffuse_color += base_color * light.color * max(0, intensity);
    }

    vec3 ambient_color = vec3(0.2);
    ambient_color *= max(vec3(0), 0.3 - diffuse_color);

    vec3 emission_color = get_emission();

    out_color = vec4(ambient_color + diffuse_color + emission_color, 1);
}
