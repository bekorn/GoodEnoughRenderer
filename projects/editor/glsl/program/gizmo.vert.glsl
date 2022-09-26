in vec3 position;
in vec3 color;

layout(location = 0) uniform mat3 transform;

struct V2F
{
    vec3 color;
};

out V2F v2f;

void main()
{
    v2f.color = color;

    vec3 pos = transform * position;
    pos.z = 1 - pos.z;
    pos /= 6.;
    pos.xy += 0.8;
    gl_Position = vec4(pos , 1);
}