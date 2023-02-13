#ifdef ONLY_VERT ///////////////////////////////////////////////////////////////////////////////////////////////////////
out flat int idx;

void main()
{
    idx = gl_VertexID;
}
#endif

#ifdef ONLY_FRAG ///////////////////////////////////////////////////////////////////////////////////////////////////////
uniform layout(binding = 0, rgba8) image3D volume;
uniform ivec3 volume_res;

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

    vec3 closest_pos = imageLoad(volume, voxel_idx).xyz;

    imageStore(volume, voxel_idx, vec4(closest_pos, distance(pos, closest_pos)));
}
#endif