in vec3 GS_pos;

uniform layout(rgba8) writeonly image3D voxels;
uniform ivec3 voxels_res;

uniform int fragment_multiplier = 1;

void main()
{
//  for some reason imageSize.z is always 1 :/
//    ivec3 voxels_res = imageSize(voxels);

    ivec3 texel = ivec3(GS_pos * voxels_res);
    imageStore(voxels, texel, vec4(GS_pos, 1));
    // to visualize reprojected triangles
    imageStore(voxels, ivec3(gl_FragCoord.xy / fragment_multiplier, 0), vec4(GS_pos, 1));
}