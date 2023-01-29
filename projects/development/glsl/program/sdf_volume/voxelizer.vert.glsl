in vec3 pos;

out vec3 VS_pos;

void main()
{
    // temporary fit for DamagedHelmet mesh, will be replaced by bounding box values
    vec3 rotated_pos = pos.xzy * vec3(-1, -1, -1);
    VS_pos = rotated_pos * 0.5 + 0.5;
    VS_pos.z -= 0.1;

    gl_Position = vec4(rotated_pos, 1);
}