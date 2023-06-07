#define LOCAL_SIZE 2
layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

layout(binding = 0) readonly buffer LineVAO
{
    float[] line_positions;
};
layout(binding = 1) writeonly buffer TubeVAO
{
    float[] tube_positions;
// buffer will be structured as:
// [ (oooo) (oooo) ... (oooo) (o) (o) ]
//    ring0  ring1      ringN  H   T
// H: head, T: tail, (ring0, ringN): body, N: line_length-2
};

const float TWO_PI = 2 * 3.14156;

const int line_size = 32;
const uint line_base = gl_LocalInvocationIndex * line_size*3;

const int ring_res = 6;
const int ring_count = line_size - 2;
const int tube_size = 2 * 1/*head & tail*/ + ring_count * ring_res/*body*/;
const uint tube_base = gl_LocalInvocationIndex * tube_size*3;

vec3 get_line_position(uint idx)
{
    idx = line_base + 3 * idx;
    return vec3(line_positions[idx + 0], line_positions[idx + 1], line_positions[idx + 2]);
}
void set_tube_position(uint idx, vec3 pos)
{
    idx = tube_base + 3 * idx;
    tube_positions[idx + 0] = pos[0]; tube_positions[idx + 1] = pos[1]; tube_positions[idx + 2] = pos[2];
}

float get_radius(float n /*normalized over line_size*/)
{
    float d = 0.5 - abs(n - 0.5);
    return mix(0.0, 0.04, pow(smoothstep(.0, .5, d), 0.4));
}

vec3 rotate_around(vec3 axis, float angle, vec3 p)
{
    // http://www.songho.ca/opengl/gl_rotate.html
    vec3 a = normalize(axis);
    float c = cos(angle);
    float s = sin(angle);
    return (1 - c) * dot(p, a) * a + c * p + s * cross(a, p);
}

void main()
{
    /// head & tail
    vec3 head = get_line_position(0);
    vec3 tail = get_line_position(line_size - 1);

    set_tube_position(tube_size - 2, head);
    set_tube_position(tube_size - 1, tail);

    /// body
    // using Parallel Transport, tutorial by Entagma: https://youtu.be/5LedteSEgOE
    vec3 previous_tangent, previous_normal;
    {
        vec3 P0 = get_line_position(0);
        vec3 P1 = get_line_position(1);
        previous_tangent = normalize(P1 - P0);

        vec3 normal = previous_tangent.y < 0.5 ? vec3(0, 1, 0) : vec3(1, 0, 0);
        vec3 bitangent = normalize(cross(previous_tangent, normal));
        previous_normal = normalize(cross(previous_tangent, bitangent));
    }

    for (int r = 0; r < ring_count; ++r)
    {
        vec3 Pprev = get_line_position(r + 0);
        vec3 Pcurr = get_line_position(r + 1);
        vec3 Pnext = get_line_position(r + 2);

        vec3 tangent = normalize(Pnext - Pprev);
        vec3 bitangent = normalize(cross(previous_tangent, tangent));
        vec3 normal;

        if (dot(bitangent, bitangent) == 0)
        {
            normal = previous_normal;
        }
        else
        {
            float angle = acos(dot(tangent, previous_tangent));
            normal = rotate_around(bitangent, angle, previous_normal);
        }
        bitangent = normalize(cross(normal, tangent));

        vec3 center = Pcurr;
        vec3 up = normal;
        vec3 right = bitangent;
        float radius = get_radius(float(r+1) / line_size);

        for (int i = 0; i < ring_res; ++i)
        {
            float rad = (float(i) / ring_res) * TWO_PI;
            float x = cos(rad);
            float y = sin(rad);
            vec3 pos = center + radius * (x * right + y * up);
            set_tube_position(r * ring_res + i, pos);
        }

        previous_tangent = tangent;
        previous_normal = normal;
    }
}