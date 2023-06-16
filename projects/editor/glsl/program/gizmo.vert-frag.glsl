struct VS
{
    vec3 color;
};

#ifdef ONLY_VERT ///////////////////////////////////////////////////////////////////////////////////////////////////////
uniform mat4 transform;

out VS vs;

void main()
{
    vs.color = color.rgb;

    gl_Position = transform * vec4(position, 1);
}
#endif

#ifdef ONLY_FRAG ///////////////////////////////////////////////////////////////////////////////////////////////////////
in VS vs;

out vec4 out_color;

void main()
{
    out_color = vec4(vs.color, 1);
}
#endif