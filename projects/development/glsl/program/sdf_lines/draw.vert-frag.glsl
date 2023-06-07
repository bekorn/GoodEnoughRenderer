#ifdef ONLY_VERT ///////////////////////////////////////////////////////////////////////////////////////////////////////
in vec3 position;

out flat vec3 color;

const int line_size = 32;

const int ring_res = 6;
const int ring_count = line_size - 2;
const int tube_size = 2 * 1/*head & tail*/ + ring_count * ring_res/*body*/;

vec3 rand_1_3(float p)
{
    vec3 p3 = fract(vec3(p) * vec3(.131, .130, .973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}

void main()
{
    float line_idx = gl_VertexID / tube_size;
    float r = rand_1_3(line_idx / 10 + 1).x;
    const vec3 col0 = vec3(234, 240, 62) / 255.;
    const vec3 col1 = vec3(255, 0, 32) / 255.;
    color = mix(col0, col1, r * 1.5);

//    color = rand_1_3(gl_VertexID);

    gl_Position = TransformVP * vec4(position, 1);
}
#endif

#ifdef ONLY_FRAG ///////////////////////////////////////////////////////////////////////////////////////////////////////
in flat vec3 color;

out vec4 out_color;

void main()
{
    out_color = gl_FrontFacing ? vec4(0, 1, 0, 1) : vec4(1, 0, 0, 1);

    out_color = vec4(color, 1);
}
#endif