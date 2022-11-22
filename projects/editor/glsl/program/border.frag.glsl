uniform usampler2D positions;
uniform sampler2D border_map;
uniform float thickness = 12;

out vec4 out_color;

void main()
{
    ivec2 texel_idx = ivec2(gl_FragCoord.xy);
    uvec2 position = uvec2(gl_FragCoord.xy);

    uvec2 closest = texelFetch(positions, texel_idx, 0).rg;
    vec2 diff = abs(ivec2(position - closest));
    float dist = length(diff);

    if (dist < 1 || dist > thickness)
        discard;

    out_color = textureLod(border_map, vec2(dist / thickness, 0.5), 0);
    out_color.a *= smoothstep(thickness, thickness - 1, dist);
}