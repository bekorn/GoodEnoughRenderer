in vec3 position;
in vec3 color;

uniform mat4 transform;

struct V2F
{
    vec3 color;
};

out V2F v2f;

void main()
{
    v2f.color = color;

    gl_Position = transform * vec4(position, 1);
}