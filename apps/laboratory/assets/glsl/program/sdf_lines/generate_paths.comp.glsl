layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

sampler3D sdf;

layout(binding = 0) buffer VAO
{
    float[] positions;
};

vec3 rand_1_3(float p)
{
    vec3 p3 = fract(vec3(p) * vec3(.131, .1030, .973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}

void query_sdf(vec3 pos, out vec3 closest, out float dist)
{
    vec4 sdf = texture(sdf, pos * 0.5 + 0.5);
    closest = sdf.xyz * 2 - 1;
    dist = sdf.w;
}

void set_position(uint idx, vec3 pos)
{
    idx = line_base + 3 * idx;
    positions[idx + 0] = pos[0]; positions[idx + 1] = pos[1]; positions[idx + 2] = pos[2];
}
vec3 get_position(uint idx)
{
    idx = line_base + 3 * idx;
    return vec3(positions[idx + 0], positions[idx + 1], positions[idx + 2]);
}


void main()
{
    float r = 0.32 / 32;

    vec3 head = get_position(0);
    vec3 dir = normalize(head - get_position(1));

    vec3 closest;
    float dist;
    query_sdf(head, closest, dist);

    if (any(greaterThan(abs(head), vec3(1.2))))
    {
        dir = normalize(-head);
    }
    else if (dist > 0.01)
    {
        const float bot = 0.01;
        const float mid = 0.03;
        const float top = 0.4;

        vec3 to_surface = normalize(head - closest);

        if (dist < mid)
        {
            float norm = (dist - bot) / (mid - bot);
            float f = pow(norm, 1.8);
            dir = mix(to_surface, dir, clamp(f, 0.0, 1.0));
            dir = normalize(dir);
        }
        else
        {
            float norm = (dist - mid) / (top - mid);
            float f = pow(norm, 1.5);
            dir = mix(dir, -to_surface, clamp(f, 0.0, 1.0));
            dir = normalize(dir);
        }
    }
    else
    {
        dir = normalize(get_position(1) - head);
    }

    vec3 next_pos = head + r * dir + 0.00*(rand_1_3(dist * dir.x + dir.y - dir.z) * 2 - 1);

    for (int i = 0; i < line_size; ++i)
    {
        vec3 old_pos = get_position(i);
        set_position(i, next_pos);
        next_pos = old_pos;
    }
}