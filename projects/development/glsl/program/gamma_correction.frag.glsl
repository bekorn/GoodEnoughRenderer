in vec2 uv;

uniform sampler2D color_attachment;

out vec4 out_color;

void main()
{
    vec4 color = texture(color_attachment, uv);
    color.rgb = pow(color.rgb, vec3(1 / 2.2));
    out_color = color;
}