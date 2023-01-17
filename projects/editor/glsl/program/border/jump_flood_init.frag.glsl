out uvec2 position;

void main()
{
    position = uvec2(gl_FragCoord.xy);
}