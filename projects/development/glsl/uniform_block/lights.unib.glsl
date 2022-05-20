// TODO(bekorn): find a proper place to sprecify extensions
#extension GL_ARB_bindless_texture : enable
#extension GL_ARB_gpu_shader_int64 : enable

struct Light
{
    vec3 position;
    float intensity;
    vec3 color;
    bool is_active;
};

layout (binding = 10) uniform _Lights
{
    Light Lights[4];
};
