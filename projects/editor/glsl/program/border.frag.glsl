uniform usampler2D positions;

out vec4 out_color;

const float thickness = pow(16, 1);

const ivec2 texel_idx = ivec2(gl_FragCoord.xy);
const uvec2 position = uvec2(gl_FragCoord.xy);

void main()
{
    uvec2 closest = texelFetch(positions, texel_idx, 0).rg;
    vec2 diff = abs(ivec2(position - closest));
    float dist = length(diff);

    if (dist < 1 || dist > thickness)
        discard;

//    float alpha_faloff = smoothstep(thickness, thickness - 1, dist);
    dist /= thickness;
#if 1
    out_color = vec4(
        dist > 0.5 ? vec3(1) : vec3(0),
        1
    );
#else
    out_color = vec4(
        dist > 0.7 ? vec3(1, 0.8, 0.05) : vec3(0),
        smoothstep(0.1, 1.0, dist)
    );
#endif
}