in vec2 uv;

uniform sampler3D volume;
uniform float z;

out vec4 out_color;

void main()
{
    out_color = textureLod(volume, vec3(uv, z), 0);
}