in vec2 uv;

uniform mat4x3 view_dirs;
uniform samplerCube environment;

out vec4 out_color;

const float PI = 3.14159265359;

float RAND12(vec2 p)
{ return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }

void main()
{
    // view_dirs[0] => uv 0,0
    // view_dirs[1] => uv 1,0
    // view_dirs[2] => uv 0,1
    // view_dirs[3] => uv 1,1
    vec3 view_dir = mix(
        mix(view_dirs[0], view_dirs[1], uv.x),
        mix(view_dirs[2], view_dirs[3], uv.x),
        uv.y
    );

    mat3x3 tangent_to_world;
    {
        vec3 forward = normalize(view_dir);
        vec3 up = vec3(0, 1, 0);
        vec3 right = normalize(cross(forward, up));
        up = cross(forward, -right);
        tangent_to_world = mat3x3(right, up, forward);
    }

    float lon_offset = RAND12(uv);

    vec3 irradiance = vec3(0);

    const float lat_sample = 128, lon_sample = 256;
    const float normalier = PI / (lat_sample * lon_sample);
    for (float lat = 0;          lat < 0.5*PI;              lat += 0.5*PI / lat_sample)
    for (float lon = lon_offset; lon < 2.0*PI + lon_offset; lon += 2.0*PI / lon_sample)
    {
        vec3 sample_dir = tangent_to_world * vec3(
            sin(lat) * cos(lon),
            sin(lat) * sin(lon),
            cos(lat)
        );

        // TODO(bekorn): fix lod=4, it shouldn't be necessary
        irradiance += normalier * cos(lat) * sin(lat) * textureLod(environment, sample_dir, 4).rgb;
    }

    out_color = vec4(irradiance, 1);
}