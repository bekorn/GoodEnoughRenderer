layout(location = ATTRIBUTE_LOCATION_POSITION)      in vec3 vertex_position;
layout(location = ATTRIBUTE_LOCATION_NORMAL)        in vec3 vertex_normal;
layout(location = ATTRIBUTE_LOCATION_COLOR_0)       in vec3 vertex_color;
layout(location = ATTRIBUTE_LOCATION_TEXCOORD_0)    in vec2 vertex_texcoord;

layout(binding = 0)  uniform sampler2D base_color_sampler;
layout(location = 0) uniform vec4      base_color_factor;

layout(binding = 1) uniform sampler2D metallic_roughness_sampler;
layout(binding = 2) uniform sampler2D emissive_sampler;
layout(binding = 3) uniform sampler2D occlusion_sampler;
layout(binding = 4) uniform sampler2D normal_sampler;

out vec4 out_color;


const vec3 light_pos = vec3(1, 1, -1);


vec3 get_base_color()
{
    vec3 base_color = vec3(1);

    // I believe this is a Dynamically Uniform Expression, e.i. no branching among the warp
    if (textureQueryLevels(base_color_sampler) != 0)
        base_color *= texture(base_color_sampler, vertex_texcoord).rgb;

    // same as above
    // TODO(bekorn): this introduces a convention: col.a == 0 means no color set
    if (base_color_factor.a != 0)
        base_color *= base_color_factor.rgb;

    return base_color;
}

vec3 get_emission()
{
    if (textureQueryLevels(emissive_sampler) != 0)
        return texture(emissive_sampler, vertex_texcoord).rgb;
    else
        return vec3(0);
}

float get_occlusion()
{
    if (textureQueryLevels(occlusion_sampler) != 0)
        return texture(occlusion_sampler, vertex_texcoord).r;
    else
        return 1;
}

vec3 get_normal()
{
    vec3 normal = vertex_normal;

    if (textureQueryLevels(normal_sampler) != 0)
    {
        vec3 normal_tex = normalize(texture(normal_sampler, vertex_texcoord).rgb * 2 - 1);
        // TODO(bekorn): implement normal mapping (prerequisite: bi/tangent space attributes)
    }

    return normal;
}

void main()
{
    vec3 normal = get_normal();

    vec3 base_color = get_base_color();

    vec3 light_dir = normalize(light_pos - vertex_position);

    float light_intensity = dot(normal, light_dir);
    light_intensity *= get_occlusion();

    vec3 diffuse_color = base_color * max(0, light_intensity);

    vec3 ambient_color = vec3(0.2);
    ambient_color *= max(vec3(0), 0.8 - diffuse_color);

    vec3 emission_color = get_emission();

    out_color = vec4(ambient_color + diffuse_color + emission_color, 1);
}
