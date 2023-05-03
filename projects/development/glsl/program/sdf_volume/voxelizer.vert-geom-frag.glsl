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

out vec3 GS_voxel_pos;

int maxCompIdx(vec3 v)
{
    int idx = 0;
    if (v[1] > v[idx]) idx = 1;
    if (v[2] > v[idx]) idx = 2;
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
    // TODO: check against volume's bounding box
    if (any(lessThan(aabb_max, vec3(-1))) || any(greaterThan(aabb_min, vec3(1))))
        return;

    vec3 e0 = verts[1] - verts[0];
    vec3 e1 = verts[2] - verts[0];
    vec3 n = cross(e0, e1);
    int dominant_axis = maxCompIdx(abs(n));

    int proj_axes[2] = {
        (dominant_axis + 1) % 3,
        (dominant_axis + 2) % 3,
        //dominant_axis
    };

    for (int i = 0; i < 3; ++i)
    {
        vec3 v = verts[i];

        gl_Position.x = v[proj_axes[0]];
        gl_Position.y = v[proj_axes[1]];
        gl_Position.z = 0.5;

        // map world -> uvw
        GS_voxel_pos = v * 0.5 + 0.5;
        EmitVertex();
    }
    EndPrimitive();
}
#endif

#ifdef ONLY_FRAG ///////////////////////////////////////////////////////////////////////////////////////////////////////
uniform layout(r32ui) uimage3D voxels;
uniform ivec3 voxels_res;
uniform int fragment_multiplier = 1;

in vec3 GS_voxel_pos;

void main()
{
    vec3 pos = GS_voxel_pos * voxels_res;
    ivec3 texel = ivec3(pos);

    vec3 diff = pos - (texel + 0.5);
    float dist = dot(diff, diff);
    // put dist on the most significant digits so the AtomicMin will set the voxel to the val with the minimum dist
    uint val = packUnorm4x8(vec4(dist, GS_voxel_pos));
    imageAtomicMin(voxels, texel, val);

// to visualize reprojected triangles
//    imageAtomicMin(voxels, ivec3(gl_FragCoord.xy / fragment_multiplier, 0), val);
}
#endif