// TODO(bekorn): find a proper location
layout(location = 10) uniform mat4 transform;

in vec3 position;
in vec3 normal;
in vec3 color;
in vec2 texcoord;

out vec3 vertex_position;
out vec3 vertex_normal;
out vec3 vertex_color;
out vec2 vertex_texcoord;

void main()
{
    gl_Position = transform * vec4(position, 1);

    vertex_position = vec3(gl_Position);
    vertex_normal = vec3(transform * vec4(normal, 0));
    vertex_color = color;
    vertex_texcoord = texcoord;
}
