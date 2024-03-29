uniform usampler2D positions;
uniform sampler2D border_map;
uniform float border_width = 12;

out vec4 out_color;

void main()
{
    ivec2 texel_idx = ivec2(gl_FragCoord.xy);
    uvec2 position = uvec2(gl_FragCoord.xy);

    uvec2 closest = texelFetch(positions, texel_idx, 0).rg;
    vec2 diff = abs(ivec2(position - closest));
    float dist = length(diff) - 1; // border starts where dist > 1

    const float fade_out = 1; // anti-aliasing

    if (dist < 0 || dist > border_width + fade_out)
        discard;

    out_color = textureLod(border_map, vec2(dist / border_width, 0), 0);
    out_color.a *= smoothstep(border_width + fade_out, border_width, dist);
}