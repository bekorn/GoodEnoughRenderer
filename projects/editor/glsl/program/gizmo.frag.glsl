struct V2F
{
    vec3 color;
};

in V2F v2f;

out vec4 out_color;

void main()
{
    // TODO(bekorn): make this a post proccessing effect
    out_color = vec4(pow(v2f.color,  vec3(1 / 2.2)), 1);
}