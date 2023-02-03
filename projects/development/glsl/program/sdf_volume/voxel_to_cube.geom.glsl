// call: glDrawArrays(GL_POINTS, 0, compMul(volume_res))
// VS: pass the voxel_idx for each voxel in the volume
// GS: generate cube's edges for each occupied voxel (alpha != 0) in world space
// FS: output color

layout(points) in;
layout(line_strip, max_vertices = 16) out;

in ivec3 voxel_idx[1];

uniform sampler3D volume;
uniform ivec3 volume_res;

out flat vec3 GS_pos;

void emit_vertex(vec4 v)
{
    gl_Position = v;
    GS_pos = vec3(voxel_idx[0]) / volume_res;
    EmitVertex();
}

void emit_line(vec4 v0, vec4 v1)
{
    emit_vertex(v0);
    emit_vertex(v1);
    EndPrimitive();
}

void main()
{
    ivec3 voxel_idx = voxel_idx[0];

    if (texelFetch(volume, voxel_idx, 0).a == 0)
        return;

    vec3 voxel_size = 2 / vec3(volume_res);
    vec3 voxel_pos = vec3(voxel_idx) * voxel_size - 1;

    vec4 front_corners[4] = {
        TransformVP * vec4(voxel_size * vec3(0, 0, 0) + voxel_pos, 1),
        TransformVP * vec4(voxel_size * vec3(1, 0, 0) + voxel_pos, 1),
        TransformVP * vec4(voxel_size * vec3(0, 1, 0) + voxel_pos, 1),
        TransformVP * vec4(voxel_size * vec3(1, 1, 0) + voxel_pos, 1),
    };
    vec4 back_corners[4] = {
        TransformVP * vec4(voxel_size * vec3(0, 0, 1) + voxel_pos, 1),
        TransformVP * vec4(voxel_size * vec3(1, 0, 1) + voxel_pos, 1),
        TransformVP * vec4(voxel_size * vec3(0, 1, 1) + voxel_pos, 1),
        TransformVP * vec4(voxel_size * vec3(1, 1, 1) + voxel_pos, 1),
    };

    // the path below is not intuative but emits minimal vertex count (so far)
    emit_vertex(front_corners[0]);
    emit_vertex(front_corners[1]);
    emit_vertex(front_corners[3]);
    emit_vertex(front_corners[2]);
    emit_vertex(front_corners[0]);
    emit_vertex(back_corners[0]);
    emit_vertex(back_corners[1]);
    emit_vertex(front_corners[1]);
    EndPrimitive();

    emit_vertex(back_corners[0]);
    emit_vertex(back_corners[2]);
    emit_vertex(back_corners[3]);
    emit_vertex(back_corners[1]);
    EndPrimitive();

    emit_line(front_corners[2], back_corners[2]);
    emit_line(front_corners[3], back_corners[3]);
}