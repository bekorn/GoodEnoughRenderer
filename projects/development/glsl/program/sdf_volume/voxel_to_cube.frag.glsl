in flat vec3 GS_pos;

out vec4 out_color;

void main()
{
    out_color = vec4(GS_pos, 1);
}