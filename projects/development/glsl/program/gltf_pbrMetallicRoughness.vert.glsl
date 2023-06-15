uniform mat4 TransformM;
uniform mat4 TransformMVP;

out Vertex
{
    vec3 camera_position;
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
    vertex.camera_position = vec3(TransformV * vec4(position, 1));

    vertex.normal = vec3(TransformM * vec4(normal, 0));
    vertex.color = color;
    vertex.texcoord = texcoord;

    gl_Position = TransformMVP * vec4(position, 1);
}
