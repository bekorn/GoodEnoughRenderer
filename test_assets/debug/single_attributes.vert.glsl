layout(location = ATTRIBUTE_LOCATION_POSITION) in vec3 a_position;
layout(location = ATTRIBUTE_LOCATION_VISUALIZE) in vec4 a_visualize;

layout(location = 0) out vec4 vertex_visualize;

void main()
{
    vertex_visualize = a_visualize;

    gl_Position = vec4(a_position, 1);
}
