// Usage: glDrawArrays(GL_POINTS, 0, 1)
//  Assumes: sdf volume at (0, 0, 0) with dimensions (2, 2, 2)
// GEOM: Generates a cube on the boundary of the sdf
// FRAG: Sphere traces the sdf until the isosurface value is reached

#ifdef ONLY_VERT ///////////////////////////////////////////////////////////////////////////////////////////////////////
void main() {}
#endif

#ifdef ONLY_GEOM ///////////////////////////////////////////////////////////////////////////////////////////////////////
layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

out vec3 GS_pos_object;

vec3 corners_object[8];
vec4 corners_clip[8];

void emit_quad(int idx0, int idx1, int idx2, int idx3)
{
    gl_Position = corners_clip[idx0], GS_pos_object = corners_object[idx0], EmitVertex();
    gl_Position = corners_clip[idx1], GS_pos_object = corners_object[idx1], EmitVertex();
    gl_Position = corners_clip[idx2], GS_pos_object = corners_object[idx2], EmitVertex();
    gl_Position = corners_clip[idx3], GS_pos_object = corners_object[idx3], EmitVertex();
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
        corners_clip[i] = TransformVP * vec4(corners_object[i], 1);

    emit_quad(0, 4, 2, 6); // -x
    emit_quad(1, 3, 5, 7); // +x

    emit_quad(0, 1, 4, 5); // -y
    emit_quad(2, 6, 3, 7); // +y

    emit_quad(0, 2, 1, 3); // -z
    emit_quad(4, 5, 6, 7); // +z
}
#endif

#ifdef ONLY_FRAG ///////////////////////////////////////////////////////////////////////////////////////////////////////
layout (depth_greater) out float gl_FragDepth;
uniform sampler3D sdf;
uniform float isosurface_value;

in vec3 GS_pos_object;
vec3 GS_pos_world = GS_pos_object;

out vec4 out_color;

void main()
{
    vec3 dir = normalize(GS_pos_world - CameraWorldPosition);
    vec3 origin = GS_pos_object;
    float t = 0;
    float dist;

    int iter, max_iter = 256;
    for (iter = 0; iter < max_iter; ++iter)
    {
        vec3 pos = (origin + dir * t);
        if (any(greaterThan(abs(pos), vec3(1)))) discard;

        dist = texture(sdf, pos * 0.5 + 0.5).a;
        t += dist * 0.5; // move half dist to increase accuracy

        if (dist < isosurface_value) break;
    }

    vec3 pos = origin + dir * t;

    out_color = vec4(pos, 1);
//    out_color = vec4(vec3(float(iter) / max_iter), 1);

    vec4 pos_clip = TransformVP * vec4(pos, 1);
    gl_FragDepth = pos_clip.z / pos_clip.w;
}
#endif