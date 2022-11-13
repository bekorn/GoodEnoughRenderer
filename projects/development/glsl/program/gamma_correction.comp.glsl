layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba8) uniform image2D img;

void main()
{
    ivec2 texel_idx = ivec2(gl_GlobalInvocationID.xy);

    vec4 color = imageLoad(img, texel_idx);
    color.rgb = pow(color.rgb, vec3(1. / 2.2));
    imageStore(img, texel_idx, color);
}