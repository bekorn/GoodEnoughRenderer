struct Light
{
    vec3 position;
    float range;
    vec3 color;
    bool is_active;
};

layout (binding = 10) uniform _Lights
{
    Light Lights[4];
};
