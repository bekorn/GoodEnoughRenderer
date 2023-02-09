layout(points) in;
layout(triangle_strip, max_vertices = 12) out;

in ivec3 voxel_idx[1];
in int face_mask[1];

uniform ivec3 volume_res;

out flat int GS_axis;

void emit_vertex(vec4 v)
{
    gl_Position = v;
    EmitVertex();
}

void main()
{
    ivec3 voxel_idx = voxel_idx[0];
    int face_mask = face_mask[0];

    if (face_mask == 0)
        return;

    vec3 voxel_size = 2 / vec3(volume_res);
    vec3 voxel_pos = vec3(voxel_idx) * voxel_size - 1;
    vec4 corners[] = {
        //TransformVP * vec4(voxel_size * vec3(0, 0, 0) + voxel_pos, 1), omitted, not needed
        TransformVP * vec4(voxel_size * vec3(1, 0, 0) + voxel_pos, 1),
        TransformVP * vec4(voxel_size * vec3(0, 1, 0) + voxel_pos, 1),
        TransformVP * vec4(voxel_size * vec3(1, 1, 0) + voxel_pos, 1),

        TransformVP * vec4(voxel_size * vec3(0, 0, 1) + voxel_pos, 1),
        TransformVP * vec4(voxel_size * vec3(1, 0, 1) + voxel_pos, 1),
        TransformVP * vec4(voxel_size * vec3(0, 1, 1) + voxel_pos, 1),
        TransformVP * vec4(voxel_size * vec3(1, 1, 1) + voxel_pos, 1),
    };

    if (bool(face_mask & (1 << 0)))
    {
        GS_axis = 0;
        emit_vertex(corners[0]); emit_vertex(corners[2]);
        emit_vertex(corners[4]); emit_vertex(corners[6]);
        EndPrimitive();
    }
    if (bool(face_mask & (1 << 1)))
    {
        GS_axis = 1;
        emit_vertex(corners[2]); emit_vertex(corners[1]);
        emit_vertex(corners[6]); emit_vertex(corners[5]);
        EndPrimitive();
    }
    if (bool(face_mask & (1 << 2)))
    {
        GS_axis = 2;
        emit_vertex(corners[3]); emit_vertex(corners[4]);
        emit_vertex(corners[5]); emit_vertex(corners[6]);
        EndPrimitive();
    }
}