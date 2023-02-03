uniform ivec3 volume_res;

out ivec3 voxel_idx;

void main()
{
    int idx = gl_VertexID;

    voxel_idx = ivec3(
        idx % volume_res.x,
        idx / volume_res.x % volume_res.y,
        idx / (volume_res.x * volume_res.y)
    );
}