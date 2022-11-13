layout(local_size_x = 8, local_size_y = 8) in;

uniform uint64_t depth_attachment_handle;
layout(rgba8) uniform writeonly image2D color_attachment;
uniform vec2 framebuffer_size;

uniform mat4x3 view_directions;

uniform uint64_t environment_map_handle;

void main()
{    ivec2 texel_idx = ivec2(gl_GlobalInvocationID.xy);

//    vec2 uv = (vec2(texel_idx) + 0.5) / vec2(imageSize(color_attachment));
//    vec2 uv = (vec2(texel_idx) + 0.5) / vec2(720, 720);  //==> -100 us
    vec2 uv = (vec2(texel_idx) + 0.5) / framebuffer_size;  //==> - 90 us

    float depth = texelFetch(sampler2D(depth_attachment_handle), texel_idx, 0).r;
    if (depth != 1)
        return;

    // mat3 invVP = inverse(mat3(TransformVP));
    // view_directions[0] = invVP * vec3(-1, -1, 1);
    // view_directions[1] = invVP * vec3(+1, -1, 1);
    // view_directions[2] = invVP * vec3(-1, +1, 1);
    // view_directions[3] = invVP * vec3(+1, +1, 1);
    vec3 view_dir = mix(
        mix(view_directions[0], view_directions[1], uv.x),
        mix(view_directions[2], view_directions[3], uv.x),
        uv.y
    );

    vec4 environment = textureLod(samplerCube(environment_map_handle), view_dir, 0);
    imageStore(color_attachment, texel_idx, environment);
}