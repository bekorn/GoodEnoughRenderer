layout (binding = 2) uniform _Camera
{
    mat4 TransformV;
    mat4 TransformP;
    mat4 TransformVP;
    vec3 CameraWorldPosition;
};
