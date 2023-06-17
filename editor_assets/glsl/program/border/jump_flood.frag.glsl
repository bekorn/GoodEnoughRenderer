uniform usampler2D positions;
uniform int step;

out uvec2 out_position;

//#define SAMPLE_ORTHOGONAL   // has rare defacts, usable if needed
//#define SAMPLE_DIAGONAL     // has easily noticable defacts
#define SAMPLE_BOTH         // best approximation

#if defined SAMPLE_ORTHOGONAL
const ivec2 offsets[5] = ivec2[5](
                        ivec2(0,+step),
    ivec2(-step,    0), ivec2(0,    0), ivec2(+step,    0),
                        ivec2(0,-step)
);
#elif defined SAMPLE_DIAGONAL
const ivec2 offsets[5] = ivec2[5](
    ivec2(-step,+step),                 ivec2(+step,+step),
                        ivec2(0,    0),
    ivec2(-step,-step),                 ivec2(+step,-step)
);
#elif defined SAMPLE_BOTH
const ivec2 offsets[9] = ivec2[9](
    ivec2(-step,+step), ivec2(0,+step), ivec2(+step,+step),
    ivec2(-step,    0), ivec2(0,    0), ivec2(+step,    0),
    ivec2(-step,-step), ivec2(0,-step), ivec2(+step,-step)
);
#endif

const ivec2 max_texel_idx = textureSize(positions, 0);
const ivec2 min_texel_idx = ivec2(0);

const ivec2 texel_idx = ivec2(gl_FragCoord.xy);
const uvec2 pos = uvec2(gl_FragCoord.xy);

bool is_valid(ivec2 texel_idx)
{ return all(greaterThanEqual(texel_idx, min_texel_idx)) && all(lessThan(texel_idx, max_texel_idx)); }

void main()
{
    uvec2 closest = uvec2(0);
    float shortest_dist = 1. / 0.;

    for (int i = 0; i < offsets.length(); ++i)
    {
        ivec2 sample_texel_idx = texel_idx + offsets[i];
        if (!is_valid(sample_texel_idx))
            continue;

        uvec2 sample_pos = texelFetch(positions, sample_texel_idx, 0).rg;

        uvec2 diff = abs(ivec2(pos - sample_pos));
        float dist = dot(diff, diff);

        if (dist < shortest_dist)
        {
            shortest_dist = dist;
            closest = sample_pos;
        }
    }

    if (closest != uvec2(0))
        out_position = closest;
}