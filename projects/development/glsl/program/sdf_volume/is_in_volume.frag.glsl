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