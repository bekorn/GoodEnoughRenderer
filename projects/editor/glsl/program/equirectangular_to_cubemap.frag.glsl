in vec2 uv;

uniform mat4x3 view_dirs;
uniform sampler2D equirect;
uniform vec3 up;
uniform vec3 face;

out vec4 out_color;

// https://learnopengl.com/Advanced-OpenGL/Cubemaps
vec2 map(vec3 v)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    // view_dirs[0] => uv 0,0
    // view_dirs[1] => uv 1,0
    // view_dirs[2] => uv 0,1
    // view_dirs[3] => uv 1,1
    vec3 view_dir = normalize(mix(
        mix(view_dirs[0], view_dirs[1], uv.x),
        mix(view_dirs[2], view_dirs[3], uv.x),
        uv.y
    ));

    out_color = textureLod(equirect, map(view_dir), 0);
}