// TODO(bekorn): find a proper location
layout(location = 10) uniform mat4 transform;

layout(location = ATTRIBUTE_LOCATION_POSITION)      in vec3 position;
layout(location = ATTRIBUTE_LOCATION_NORMAL)        in vec3 normal;
layout(location = ATTRIBUTE_LOCATION_COLOR_0)       in vec3 color;
layout(location = ATTRIBUTE_LOCATION_TEXCOORD_0)    in vec2 texcoord;

layout(location = ATTRIBUTE_LOCATION_POSITION)      out vec3 vertex_position;
layout(location = ATTRIBUTE_LOCATION_NORMAL)        out vec3 vertex_normal;
layout(location = ATTRIBUTE_LOCATION_COLOR_0)       out vec3 vertex_color;
layout(location = ATTRIBUTE_LOCATION_TEXCOORD_0)    out vec2 vertex_texcoord;

void main()
{
    gl_Position = transform * vec4(position, 1);

    vertex_position = vec3(gl_Position);
    vertex_normal = vec3(transform * vec4(normal, 0));
    vertex_color = color;
    vertex_texcoord = texcoord;
}
