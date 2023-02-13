in vec3 pos;

uniform sampler3D volume;

out vec4 out_color;

void main()
{
    vec4 color = textureLod(volume, pos, 0);

//    if (color.a == 0)
//        discard;

    out_color = color;
}