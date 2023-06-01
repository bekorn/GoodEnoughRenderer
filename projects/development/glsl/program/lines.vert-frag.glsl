#ifdef ONLY_VERT ///////////////////////////////////////////////////////////////////////////////////////////////////////
in vec3 position;

out flat vec3 color;

const int line_size = 128;

vec3 rand_1_3(float p)
{
    vec3 p3 = fract(vec3(p) * vec3(.131, .130, .973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}

void main()
{
    float line_idx = gl_VertexID / line_size;
    color = pow(rand_1_3(line_idx / 10 + 1), vec3(2.5));
    color = max(vec3(0.05), 1.4 * color);

    gl_Position = TransformVP * vec4(position, 1);
}
#endif

#ifdef ONLY_FRAG ///////////////////////////////////////////////////////////////////////////////////////////////////////
in flat vec3 color;

out vec4 out_color;

void main()
{
    out_color = vec4(color, 1);
}
#endif