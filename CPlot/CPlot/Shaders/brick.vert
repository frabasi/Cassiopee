#version 400 compatibility
// Procedural bricks
/*varying vec2 MCposition;
varying vec3 Nv;
varying vec3 P;
varying vec4 initColor;
varying vec4 vertex;*/
out V2F_OUT
{
    vec4 position;
    vec4 mv_position;
    vec4 mvp_position;
    vec4 view_normal;
    vec4 nrm_view_normal;
    vec4 color;
    vec4 vdata1, vdata2, vdata3, vdata4;
} v2f_out;

out V2CT_OUT
{
    vec4 position;
    vec4 color;
    ivec4 data_comp; // Raconte si vdata1,2,3 ou 4 est utilisé
    vec4 vdata1, vdata2, vdata3, vdata4;
} v2ct_out;

void main(void)
{
    // P = vec3(gl_ModelViewMatrix * gl_Vertex);
    v2f_out.mv_position = gl_ModelViewMatrix * gl_Vertex;
    // Nv = gl_NormalMatrix * gl_Normal;
    v2f_out.view_normal = vec4(gl_NormalMatrix * gl_Normal,0.);
   // vertex = gl_Vertex;
    v2f_out.position = gl_Vertex;
    // MCposition = gl_Vertex.xy
    // initColor = gl_Color;
    v2f_out.color = gl_Color;

    v2ct_out.position = gl_Vertex;
    v2ct_out.color    = gl_Color ;
    v2ct_out.data_comp= ivec4(0,0,0,0);

    gl_Position = ftransform();
}
