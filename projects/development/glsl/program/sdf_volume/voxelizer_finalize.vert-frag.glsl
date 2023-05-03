// Reorders voxel data into usable form: vec4(dist, pos) -> vec4(pos, dist)
// Usage: glDrawArrays(GL_POINTS, 0, compMul(voxel_res))

#ifdef ONLY_VERT ///////////////////////////////////////////////////////////////////////////////////////////////////////
out flat int idx;

void main()
{
    idx = gl_VertexID;
}
#endif

#ifdef ONLY_FRAG ///////////////////////////////////////////////////////////////////////////////////////////////////////
uniform layout(rgba8) image3D voxels;
uniform ivec3 voxels_res;

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
    ivec3 voxel_idx = map_1_3(idx, voxels_res);
    vec4 data = imageLoad(voxels, voxel_idx);
    vec3 pos = data.gba;
    bool is_empty = data == vec4(1);
    imageStore(voxels, voxel_idx, is_empty ? vec4(0) : vec4(pos, 1));
}
#endif