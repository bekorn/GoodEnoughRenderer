layout(points) in;
layout(triangle_strip, max_vertices = 12) out;

uniform mat4 transform;

in ivec3 voxel_idx[1];
in int face_mask[1];

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

    // basis vectors and base, see http://www.joshbarczak.com/blog/?p=667
    vec4 X = transform[0];
    vec4 Y = transform[1];
    vec4 Z = transform[2];
    vec4 B = transform * vec4(voxel_idx, 1);

    vec4 corners[] = {
        B,
        B + X,
        B + Y,
        B + X + Y,

        B + Z,
        B + Z + X,
        B + Z + Y,
        B + Z + X + Y,
    };

    if (bool(face_mask & (1 << 0)))
    {
        GS_axis = 0;
        emit_vertex(corners[1]); emit_vertex(corners[3]);
        emit_vertex(corners[5]); emit_vertex(corners[7]);
        EndPrimitive();
    }
    if (bool(face_mask & (1 << 1)))
    {
        GS_axis = 1;
        emit_vertex(corners[3]); emit_vertex(corners[2]);
        emit_vertex(corners[7]); emit_vertex(corners[6]);
        EndPrimitive();
    }
    if (bool(face_mask & (1 << 2)))
    {
        GS_axis = 2;
        emit_vertex(corners[4]); emit_vertex(corners[5]);
        emit_vertex(corners[6]); emit_vertex(corners[7]);
        EndPrimitive();
    }
}