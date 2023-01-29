layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 VS_pos[3];
out vec3 GS_pos;

int minCompIdx(vec3 v)
{
    int idx = 0;
    if (v[1] < v[idx]) idx = 1;
    if (v[2] < v[idx]) idx = 2;
    return idx;
}

void main()
{
    vec3 v0 = gl_in[0].gl_Position.xyz;
    vec3 v1 = gl_in[1].gl_Position.xyz;
    vec3 v2 = gl_in[2].gl_Position.xyz;
    vec3 center = (v0 + v1 + v2) / 3;

    vec3 aabb_min = min(min(v0, v1), v2);
    vec3 aabb_max = max(max(v0, v1), v2);
    vec3 aabb_size = aabb_max - aabb_min;
    int min_comp_idx = minCompIdx(aabb_size);

    int proj_axis[2] = {
        (min_comp_idx + 1) % 3,
        (min_comp_idx + 2) % 3,
        //min_comp_idx, but z will be 0
    };

    for (int i = 0; i < 3; ++i)
    {
        vec3 v = gl_in[i].gl_Position.xyz - center;
//        v *= 3;

        gl_Position.x = v[proj_axis[0]];
        gl_Position.y = v[proj_axis[1]];
        gl_Position.z = 0.5;

        GS_pos = VS_pos[i];
        EmitVertex();
    }
    EndPrimitive();
}