in vec3 position;

uniform mat4 TransformMVP;

void main()
{
    gl_Position = TransformMVP * vec4(position, 1);
}
