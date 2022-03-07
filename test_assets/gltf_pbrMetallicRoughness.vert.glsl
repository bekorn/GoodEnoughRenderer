// TODO(bekorn): find a proper location
layout(location = 10) uniform mat4 transform;

in vec3 position;
in vec3 normal;
in vec3 color;
in vec2 texcoord;

layout (binding = 48) uniform Test
{
    float value;
    vec3  vector;
    mat4  matrix;
    float values[3];
    bool  boolean;
    int   integer;
};

out Vertex
{
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 texcoord;
} vertex;

out float z;

void main()
{
    gl_Position = transform * vec4(position, 1);

    vertex.position = vec3(gl_Position);
    vertex.normal = vec3(transform * vec4(normal, 0));
    vertex.color = color;
    vertex.texcoord = texcoord;
}
