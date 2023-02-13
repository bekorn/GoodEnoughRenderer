// see https://www.comp.nus.edu.sg/~tants/jfa/i3d06.pdf

#ifdef ONLY_VERT ///////////////////////////////////////////////////////////////////////////////////////////////////////
out flat int idx;

void main()
{
    idx = gl_VertexID;
}
#endif

#ifdef ONLY_FRAG ///////////////////////////////////////////////////////////////////////////////////////////////////////
uniform layout(binding = 0, rgba8) readonly  image3D read_volume;
uniform layout(binding = 1, rgba8) writeonly image3D write_volume;
uniform ivec3 volume_res;
uniform int step;

in flat int idx;

ivec3 map_1_3(int idx, ivec3 res)
{
    return ivec3(
        idx % res.x,
        (idx / res.x) % res.y,
        idx / (res.x * res.y)
    );
}

void main()
{
    ivec3 voxel_idx = map_1_3(idx, volume_res);
    vec3 pos = vec3(voxel_idx) / volume_res;

    vec3 closest = vec3(0);
    float shortest_dist = 1. / 0.;

    for (int x = -1; x <= 1; ++x)
    for (int y = -1; y <= 1; ++y)
    for (int z = -1; z <= 1; ++z)
    {
        ivec3 sample_voxel_idx = voxel_idx + step * ivec3(x, y, z);
        vec3 sample_pos = imageLoad(read_volume, sample_voxel_idx).xyz;

        vec3 diff = abs(pos - sample_pos);
        float dist = dot(diff, diff);

        if (dist < shortest_dist)
        {
            shortest_dist = dist;
            closest = sample_pos;
        }
    }

    if (closest != uvec3(0))
        imageStore(write_volume, voxel_idx, vec4(closest, 1));
}
#endif