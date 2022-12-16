in vec2 uv;

uniform mat4x3 view_dirs;
uniform samplerCube environment;

out vec4 out_color;

const float PI = 3.14159265359;

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

    vec3 forward = normalize(view_dir);
    vec3 up = vec3(0, 1, 0);
    vec3 right = normalize(cross(forward, up));
    up = cross(forward, -right);

    vec3 irradiance = vec3(0);

    float lat_sample = 64, lon_sample = 128;
    for (float lat = 0; lat < 0.5*PI; lat += 0.5*PI / lat_sample)
    for (float lon = 0; lon < 2.0*PI; lon += 2.0*PI / lon_sample)
    {
        // spherical to cartesian (in tangent space)
        vec3 tangentSample = vec3(sin(lat) * cos(lon), sin(lat) * sin(lon), cos(lat));

        vec3 sample_dir = right * tangentSample.x + up * tangentSample.y + forward * tangentSample.z;
        irradiance += cos(lat) * sin(lat) * textureLod(environment, sample_dir, 0).rgb;
    }

    irradiance *= PI / (lat_sample * lon_sample);

    out_color = vec4(irradiance, 1);
}