struct V2F
{
    vec3 color;
};

in V2F v2f;

out vec4 out_color;

void main()
{
    out_color = vec4(v2f.color, 1);
}