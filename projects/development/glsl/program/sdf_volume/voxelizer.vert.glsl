in vec3 pos;

uniform mat4 TransformM;

void main()
{
    gl_Position = TransformM * vec4(pos, 1);
}