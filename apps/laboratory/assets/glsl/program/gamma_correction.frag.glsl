uniform sampler2D color_attachment;

out vec4 out_color;

void main()
{
    vec4 color = texelFetch(color_attachment, ivec2(gl_FragCoord.xy), 0);
    color.rgb = pow(color.rgb, vec3(1 / 2.2));
    out_color = color;
}