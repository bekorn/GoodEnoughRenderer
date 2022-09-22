layout(location = ATTRIBUTE_LOCATION_POSITION)      in vec3 vertex_position;
layout(location = ATTRIBUTE_LOCATION_NORMAL)        in vec3 vertex_normal;
layout(location = ATTRIBUTE_LOCATION_COLOR_0)       in vec3 vertex_color;
layout(location = ATTRIBUTE_LOCATION_TEXCOORD_0)    in vec2 vertex_texcoord;

layout(binding = 0) uniform sampler2D base_color_sampler;
layout(location = 1) uniform vec4 base_color_factor;

out vec4 out_color;


const vec3 light_pos = vec3(1, 1, -1);


vec3 get_albedo()
{
    vec3 albedo_factor, albedo_texture;

    // I believe this is a Dynamically Uniform Expression, e.i. no branching among the warp
    if (textureQueryLevels(base_color_sampler) != 0)
        albedo_texture = texture(base_color_sampler, vertex_texcoord).rgb;
    else
        albedo_texture = vec3(1);

    // TODO(bekorn): this introduces a convention: col.a == 0 means no color set
    // same as above
    if (base_color_factor.a != 0)
        albedo_factor = base_color_factor.rgb;
    else
        albedo_factor = vec3(1);

    return albedo_factor * albedo_texture;
}

void main()
{
    vec3 albedo_color = get_albedo();

    vec3 light_dir = normalize(light_pos - vertex_position);
    float light_intensity = dot(vertex_normal, light_dir);

    vec3 diffuse_color = albedo_color * max(0, light_intensity);

    vec3 ambient_color = vec3(0.2);
    ambient_color *= max(vec3(0), 0.8 - diffuse_color);

    out_color = vec4(ambient_color + diffuse_color, 1);
}
