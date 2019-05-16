#version 150 compatibility

out Vertex {
    vec4 P0;// Coordonnées dans l'espace projeté écran
    vec4 vP;// Coordonnées dans l'espace caméra
    vec4 e3;
    vec4 position;// Position dans l'espace world
    vec4 normal;
    vec4 color;
} vertex;
uniform float scale;
uniform int fix_length;

void main()
{
    gl_Position = ftransform();
    vertex.position = gl_Vertex;
    vertex.P0    = gl_Position;
    vertex.vP    = gl_ModelViewMatrix * gl_Vertex;
    vertex.color = gl_Color;
    vec4 transpos = vec4(gl_Color.xyz,0.) - vec4(0.5,0.5,0.5,0.0);
    transpos = (1-fix_length)*transpos + 0.5*fix_length*normalize(transpos);
    vertex.normal = vec4(gl_Normal,0.);
    vertex.e3 = scale * vec4(transpos.xyz,0.);
}
