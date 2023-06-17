/// 3 points will be expanded into quads facing each axis, and offsetted by slice_pos
/// should be used with glDrawArrays(GL_POINTS, 0, 3)

#ifdef ONLY_VERT ///////////////////////////////////////////////////////////////////////////////////////////////////////
void main(){}
#endif

#ifdef ONLY_GEOM ///////////////////////////////////////////////////////////////////////////////////////////////////////
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec3 slice_pos;
uniform mat4 TransformVP;

out vec3 pos;

void main()
{
    int axis_idx = gl_PrimitiveIDIn;

    vec3 axis_mask = vec3(0);
    axis_mask[axis_idx] = 1;

    vec3 offset = (slice_pos * 2 - 1) * axis_mask; // map [0, 1] to world_space [-1, 1]

    vec3 corners[4];
    if (axis_idx == 0)
    {
        corners = vec3[4](
            vec3(0, -1, -1),
            vec3(0, +1, -1),
            vec3(0, -1, +1),
            vec3(0, +1, +1)
        );
    }
    else if (axis_idx == 1)
    {
        corners = vec3[4](
            vec3(-1, 0, -1),
            vec3(+1, 0, -1),
            vec3(-1, 0, +1),
            vec3(+1, 0, +1)
        );
    }
    else
    {
        corners = vec3[4](
            vec3(-1, -1, 0),
            vec3(+1, -1, 0),
            vec3(-1, +1, 0),
            vec3(+1, +1, 0)
        );
    }

    for (int i = 0; i < 4; ++i)
    {
        vec3 v = corners[i] + offset;
        gl_Position = TransformVP * vec4(v, 1);
        pos = v * 0.5 + 0.5; // map [-1, 1] to volume's range [0, 1]
        EmitVertex();
    }
    EndPrimitive();
}
#endif

#ifdef ONLY_FRAG ///////////////////////////////////////////////////////////////////////////////////////////////////////
in vec3 pos;

uniform sampler3D volume;

out vec4 out_color;

void main()
{
    vec4 color = textureLod(volume, pos, 0);

//    if (color.a == 0)
//        discard;

    out_color = color;
}
#endif