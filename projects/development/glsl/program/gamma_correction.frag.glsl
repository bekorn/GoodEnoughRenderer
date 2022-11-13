in vec2 uv;

uniform uint64_t color_attachment_handle;

out vec4 out_color;

void main()
{
    sampler2D color_attachment = sampler2D(color_attachment_handle);
    vec4 color = texture(color_attachment, uv);
    color.rgb = pow(color.rgb, vec3(1 / 2.2));

    out_color = color;
}