layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

out vec3 GS_pos_object;

vec3 corners_object[8];
vec4 corners_world[8];

void emit_quad(int idx0, int idx1, int idx2, int idx3)
{
    gl_Position = corners_world[idx0], GS_pos_object = corners_object[idx0], EmitVertex();
    gl_Position = corners_world[idx1], GS_pos_object = corners_object[idx1], EmitVertex();
    gl_Position = corners_world[idx2], GS_pos_object = corners_object[idx2], EmitVertex();
    gl_Position = corners_world[idx3], GS_pos_object = corners_object[idx3], EmitVertex();
    EndPrimitive();
}

void main()
{
    corners_object = vec3[](
        vec3(-1, -1, -1),
        vec3(+1, -1, -1),
        vec3(-1, +1, -1),
        vec3(+1, +1, -1),
        vec3(-1, -1, +1),
        vec3(+1, -1, +1),
        vec3(-1, +1, +1),
        vec3(+1, +1, +1)
    );

    for (int i = 0; i < 8; ++i)
        corners_world[i] = TransformVP * vec4(corners_object[i], 1);

    emit_quad(0, 2, 4, 6); // -x
    emit_quad(1, 3, 5, 7); // +x

    emit_quad(0, 1, 4, 5); // -y
    emit_quad(2, 3, 6, 7); // +y

    emit_quad(0, 1, 2, 3); // -z
    emit_quad(4, 5, 6, 7); // +z
}