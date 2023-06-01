layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

sampler3D sdf;

layout(binding = 0) writeonly buffer VAO
{
    float[] positions;
};

vec3 rand_1_3(float p)
{
    vec3 p3 = fract(vec3(p) * vec3(.131, .1030, .973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}

const int line_size = 128;

void main()
{
    uint idx = gl_LocalInvocationIndex;
    uint base = idx * line_size*3;

    vec2 start = mix(vec2(-0.5), vec2(+0.5), vec2(gl_LocalInvocationID) / (gl_WorkGroupSize.xy - 1));

    vec3 pos = vec3(start, 0.999);

    positions[base + 0] = pos[0];
    positions[base + 1] = pos[1];
    positions[base + 2] = pos[2];

    float r = 2.0 / line_size;

    for (int i = 1; i < line_size; ++i)
    {
        vec3 offset = (rand_1_3(float(base + i*3 + float(FrameIdx) / 600.0) * 3 / positions.length()) - 0.5);
//        float r = float(i) / (line_size - 1);
//        pos += r * offset + vec3(0, 0, -0.1);

        vec4 sdf = texture(sdf, pos * 0.5 + 0.5);
        vec3 closest = sdf.xyz * 2 - 1;
        float dist = sdf.w;

        vec3 dir = vec3(0, 0, -1) + (0.2) * offset;

        if (dist != 0 && dist < 0.05)
        {
            vec3 normal = normalize(pos - closest);
            dir = mix(normal, dir, clamp((1 / 0.05) * dist, 0, 1));
            dir = normalize(dir);
        }

        pos += r * dir;

        positions[base + i*3 + 0] = pos[0];
        positions[base + i*3 + 1] = pos[1];
        positions[base + i*3 + 2] = pos[2];
    }
}