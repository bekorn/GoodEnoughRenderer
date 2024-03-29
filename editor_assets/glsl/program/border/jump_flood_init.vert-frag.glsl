#ifdef ONLY_VERT ///////////////////////////////////////////////////////////////////////////////////////////////////////
uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(position, 1);
}
#endif

#ifdef ONLY_FRAG ///////////////////////////////////////////////////////////////////////////////////////////////////////
out uvec2 position;

void main()
{
    position = uvec2(gl_FragCoord.xy);
}
#endif