layout (binding = 2) uniform _Camera
{
    mat4 TransformV;
    mat4 TransformP;
    mat4 TransformVP;
    mat4 TransformV_inv;
    mat4 TransformP_inv;
    mat4 TransformVP_inv;
    vec3 CameraWorldPosition;
};
