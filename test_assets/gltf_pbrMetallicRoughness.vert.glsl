layout(location = ATTRIBUTE_LOCATION_POSITION)      in vec3 a_position;
layout(location = ATTRIBUTE_LOCATION_NORMAL)        in vec3 a_normal;
layout(location = ATTRIBUTE_LOCATION_COLOR_0)       in vec3 a_color;
layout(location = ATTRIBUTE_LOCATION_TEXCOORD_0)    in vec2 a_texcoord;

layout(location = ATTRIBUTE_LOCATION_POSITION)      out vec3 vertex_position;
layout(location = ATTRIBUTE_LOCATION_NORMAL)        out vec3 vertex_normal;
layout(location = ATTRIBUTE_LOCATION_COLOR_0)       out vec3 vertex_color;
layout(location = ATTRIBUTE_LOCATION_TEXCOORD_0)    out vec2 vertex_texcoord;

void main()
{
    vertex_position = a_position;
    vertex_normal = a_normal;
    vertex_color = a_color;
    vertex_texcoord = a_texcoord;

    gl_Position = vec4(vertex_position, 1);
}
