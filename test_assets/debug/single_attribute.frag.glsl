layout(location = 0) in vec4 vertex_visualize;

out vec4 out_color;

void main()
{
    out_color = vertex_visualize;
}
