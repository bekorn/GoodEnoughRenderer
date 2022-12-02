uniform mat4 TransformM;
uniform mat4 TransformMVP;

in vec3 position;
in vec3 normal;
in vec3 color;
in vec2 texcoord;

out Vertex
{
    vec3 object_position;
    vec3 world_position;
    vec3 normal;
    vec3 color;
    vec2 texcoord;
} vertex;

void main()
{
    vertex.object_position = position;
    vertex.world_position = vec3(TransformM * vec4(position, 1));

    vertex.normal = mat3(TransformM) * normal;
    vertex.color = color;
    vertex.texcoord = texcoord;

    gl_Position = TransformMVP * vec4(position, 1);
}
