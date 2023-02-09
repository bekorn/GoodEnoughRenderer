// Usage: glDrawArrays(GL_POINTS, 0, compMul(ceil_to_multiple_of_4(volume_res + 1))
// VS: calculate the voxel_idx for each voxel, encode faces to generate for each axis
// GS: generate cube's faces in world space
// FS: output color wrt face's axis
// see: https://bekorn.notion.site/Optimizing-a-voxel-visualization-shader-using-a-space-filling-curve-11e729db28bd44bea40a2d79d409fc8c

uniform sampler3D volume;
uniform ivec3 compute_res;

out ivec3 voxel_idx;
out int face_mask;

// https://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
int Compact1By2(int x) // specialized for 4x4x4 volume
{
//  x &= 0x09249249;                  // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
//  x = (x ^ (x >>  2)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
//  return x;

    // or just this
    return ((x >> 2) & 2) | (x & 1);
}
ivec3 DecodeMorton3(int code)
{
    return ivec3(
        Compact1By2(code >> 0),
        Compact1By2(code >> 1),
        Compact1By2(code >> 2)
    );
}

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
    int idx = gl_VertexID;

    // map each 64 idx into 4x4x4 chunk to preserve locality (reduces branch divergence in GS)
    ivec3 chunk_res = compute_res / 4;
    ivec3 chunk_idx = map_1_3(idx / 64, chunk_res);
    ivec3 local_idx = DecodeMorton3(idx % 64); // faster than map_1_3(idx % 64, ivec3(4))
    voxel_idx = chunk_idx * 4 + local_idx;

    voxel_idx -= 1;

    float voxel = texelFetch(volume, voxel_idx, 0).a;
    face_mask = int(
        int(voxel != texelFetch(volume, voxel_idx + ivec3(1, 0, 0), 0).a) << 0 |
        int(voxel != texelFetch(volume, voxel_idx + ivec3(0, 1, 0), 0).a) << 1 |
        int(voxel != texelFetch(volume, voxel_idx + ivec3(0, 0, 1), 0).a) << 2
    );
}