// projects each triangle into the axis that will generate the most fragments
// by picking the axis with the least change as the depth (z) axis
// see the diagrams at https://developer.nvidia.com/content/basics-gpu-voxelization

#ifdef ONLY_VERT ///////////////////////////////////////////////////////////////////////////////////////////////////////
uniform mat4 TransformM;

in vec3 pos;

void main()
{
    gl_Position = TransformM * vec4(pos, 1);
}
#endif

#ifdef ONLY_GEOM ///////////////////////////////////////////////////////////////////////////////////////////////////////
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

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
    vec3 verts[3] = {
        gl_in[0].gl_Position.xyz,
        gl_in[1].gl_Position.xyz,
        gl_in[2].gl_Position.xyz,
    };

    vec3 aabb_min = min(min(verts[0], verts[1]), verts[2]);
    vec3 aabb_max = max(max(verts[0], verts[1]), verts[2]);
    vec3 aabb_size = aabb_max - aabb_min;
    int min_comp_idx = minCompIdx(aabb_size);

    int proj_axes[2] = {
        (min_comp_idx + 1) % 3,
        (min_comp_idx + 2) % 3,
        //min_comp_idx
    };

    // TODO(bekorn): not sure if moving the triangle/fragments to the center will still be usefull
    // after proper transformation and bounding box implementation
    vec3 center = (verts[0] + verts[1] + verts[2]) / 3;

    for (int i = 0; i < 3; ++i)
    {
        vec3 v = verts[i] - center;

        gl_Position.x = v[proj_axes[0]];
        gl_Position.y = v[proj_axes[1]];
        gl_Position.z = 0.5;

        GS_pos = verts[i] * 0.5 + 0.5;
        EmitVertex();
    }
    EndPrimitive();
}
#endif

#ifdef ONLY_FRAG ///////////////////////////////////////////////////////////////////////////////////////////////////////
uniform layout(rgba8) writeonly image3D voxels;
uniform ivec3 voxels_res;
uniform int fragment_multiplier = 1;

in vec3 GS_pos;

void main()
{
//  for some reason imageSize.z is always 1 :/
//    ivec3 voxels_res = imageSize(voxels);

    ivec3 texel = ivec3(GS_pos * voxels_res);
    imageStore(voxels, texel, vec4(GS_pos, 1));

// to visualize reprojected triangles
//    imageStore(voxels, ivec3(gl_FragCoord.xy / fragment_multiplier, 0), vec4(GS_pos, 1));
}
#endif