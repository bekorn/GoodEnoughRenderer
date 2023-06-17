out vec2 uv;

void main()
{
//  2
//  |`.
//  |  `.
//  |----`.
//  |    | `.
//  '----'----
//  0         1
//
//  gl_VertexID = 0 | gl_Position = (-1, -1, 1, 1) | uv = (0, 0)
//  gl_VertexID = 1 | gl_Position = (+3, -1, 1, 1) | uv = (2, 0)
//  gl_VertexID = 2 | gl_Position = (-1, +3, 1, 1) | uv = (0, 2)

    gl_Position = vec4(
        gl_VertexID == 1 ? 3 : -1,
        gl_VertexID == 2 ? 3 : -1,
        1,
        1
    );

    uv = vec2(
        gl_VertexID == 1 ? 2 : 0,
        gl_VertexID == 2 ? 2 : 0
    );
}