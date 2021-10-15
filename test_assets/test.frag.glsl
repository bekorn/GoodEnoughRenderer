layout(location = ATTRIBUTE_LOCATION_POSITION)      in vec3 vertex_position;
layout(location = ATTRIBUTE_LOCATION_NORMAL)        in vec3 vertex_normal;
layout(location = ATTRIBUTE_LOCATION_COLOR_0)       in vec3 vertex_color;
layout(location = ATTRIBUTE_LOCATION_TEXCOORD_0)    in vec2 vertex_texcoord;

out vec4 out_color;


const vec3 light_pos = vec3(1, 1, -1);

void main()
{
    vec3 albedo_color = vertex_color;

    vec3 light_dir = normalize(light_pos - vertex_position);
    float light_intensity = dot(vertex_normal, light_dir);

    vec3 diffuse_color = albedo_color * max(0, light_intensity);

    vec3 ambient_color = vec3(0.2);
    ambient_color *= max(vec3(0), 0.8 - diffuse_color);

    out_color = vec4(ambient_color + diffuse_color, 1);
}
