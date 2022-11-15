in vec2 uv;

uniform mat4x3 view_directions;
uniform samplerCube environment_map;

out vec4 out_color;

void main()
{
    // mat3 invVP = inverse(mat3(TransformVP));
    // view_directions[0] = invVP * vec3(-1, -1, 1);
    // view_directions[1] = invVP * vec3(+1, -1, 1);
    // view_directions[2] = invVP * vec3(-1, +1, 1);
    // view_directions[3] = invVP * vec3(+1, +1, 1);
    vec3 view_dir = mix(
        mix(view_directions[0], view_directions[1], uv.x),
        mix(view_directions[2], view_directions[3], uv.x),
        uv.y
    );

    out_color = texture(environment_map, view_dir);
}