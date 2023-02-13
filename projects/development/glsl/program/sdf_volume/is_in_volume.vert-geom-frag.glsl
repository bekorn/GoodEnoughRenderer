#ifdef ONLY_VERT ///////////////////////////////////////////////////////////////////////////////////////////////////////
void main() {}
#endif

#ifdef ONLY_GEOM ///////////////////////////////////////////////////////////////////////////////////////////////////////
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
#endif

#ifdef ONLY_FRAG ///////////////////////////////////////////////////////////////////////////////////////////////////////
uniform sampler2D depth_attachment;

in vec3 GS_pos_object;

out vec4 out_color;

void main()
{
    // wireframe
    {
        ivec3 test = ivec3(greaterThan(abs(GS_pos_object), vec3(0.98)));
        if (test.x + test.y + test.z >= 2)
        {
            out_color = vec4(0.9.xxx, 1);
            return;
        }
    }

    float depth = texelFetch(depth_attachment, ivec2(gl_FragCoord.xy), 0).r;

    // skip background
    if (depth == 1)
        discard;

    vec2 uv = gl_FragCoord.xy / textureSize(depth_attachment, 0);

    vec4 pos_world = TransformVP_inv * vec4(uv * 2 - 1, depth, 1);
    pos_world /= pos_world.w;

    if (any(greaterThan(abs(vec3(pos_world)), vec3(1))))
        discard;

    ivec3 pattern = ivec3((pos_world + 1) * 4) & 1;
    out_color = vec4(pattern * 0.8 , 1);
}
#endif