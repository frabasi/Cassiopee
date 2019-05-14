#version 150 compatibility
layout(triangles) in;
layout(line_strip, max_vertices=50) out;

in Vertex {
    vec4 P0;
    vec4 translation;
    vec4 position;
    vec4 normal;
    vec4 color;
} vertex[];

out vec4 color;
out vec3 Nv;
out vec3 P;
out vec4 vert;
out float gAlpha;
uniform float density;

uniform sampler1D colormap;

float rand(vec2 co){                            //2147483647
	// uniform random
    //return (int(dot(co.xy ,co.xy) * 1664525 + 1013904223)%2147483647)/2147483647.f;
    
    // sinus random
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);

    // same with optimal sinus 
    /*
    float x = dot(co.xy ,vec2(12.9898,78.233));
    x *= 0.159155;
    x -= floor(x);
    float xx=x*x;
    float y=-6.87897;
    y=y*xx+33.7755;
    y=y*xx-72.5257;
    y=y*xx+80.5874;
    y=y*xx-41.2408;
    y=y*xx+6.28077;
    return fract(x*y*43758.5453); */
}

void main()
{
    int i;

    vec4 v0P0 = vertex[0].P0;
    vec4 v1P0 = vertex[1].P0;
    vec4 v2P0 = vertex[2].P0;
    vec4 e1 = v1P0 - v0P0;
    vec4 e2 = v2P0 - v0P0;
    vec3 unrm = cross(e1.xyz,e2.xyz);
    float nrm = length(unrm);
    //float nrm = sqrt(dot(unrm.xyz,unrm.xyz));
    //float nrm_e1 = sqrt(e1.x*e1.x+e1.y*e1.y+e1.z*e1.z);
    //float nrm_e2 = sqrt(e2.x*e2.x+e2.y*e2.y+e2.z*e2.z);
    //int ni = int(nrm_e1*density);
    //int nj = int(nrm_e2*density);
    float proba = nrm*density;
    int n = int(0.5f*(-1.f+sqrt(1.+8.f*proba)));
    n = ( n > 4 ? 4 : n );
    //int n = int(density);
    if (n>0) {
    for ( int i = 0; i <= n+1; ++i ) {
    	float psi = float(i)/(n+2.f);
    	for ( int j= 0; j <= n+1-i; ++j ) {
    		float ki = float(j)/(n+2.f);
    		float te = 1.f-ki-psi;
    		vec4 p = psi*v0P0 + ki*v1P0 + te*v2P0;
		    vec4 c = psi*vertex[0].color+ki*vertex[1].color+te*vertex[2].color;
            float f = length(c.rgb);
            f = clamp(f, 0.0f, 1.0f);
            vec3 val = vec3(texture1D(colormap, f));
            c = vec4(val.r, val.g, val.b, 1.);

    		vec4 nr= psi*vertex[0].normal+ki*vertex[1].normal+te*vertex[2].normal;
    		vec4 t = psi*vertex[0].translation+ki*vertex[1].translation+te*vertex[2].translation;
    		vec4 po= psi*vertex[0].position+ki*vertex[1].position+te*vertex[2].position;
    		gl_Position = p;
    		color = c;
    		Nv    = nr.xyz;
    		P     = p.xyz;
    		vert  = po;
    		gAlpha = 0.8f;
		    EmitVertex();

    		gl_Position = p+t;
    		color = c;
    		Nv    = nr.xyz;
    		P     = p.xyz;
    		vert  = po;
    		gAlpha = 0.f;
    		EmitVertex();
    		EndPrimitive();
    	}
    } }
    else {
    //if ( n == 0 ) {
    	float ust = (1./3.);
    	// On va tirer une proba pour voir si on emet un vecteur :
	    vec4 bary1 = ust*(v0P0+v1P0+v2P0);
    	//float proba = (nrm_e1+nrm_e2)*0.5f*density;
    	float r = 0.5*rand(bary1.xy);
    	if (r < proba)
    	{
		    vec4 bnorm= ust*(vertex[0].normal+vertex[1].normal+vertex[2].normal);
    		vec4 t = ust*(vertex[0].translation+vertex[1].translation+vertex[2].translation);
    		vec4 bvert= ust*(vertex[0].position+vertex[1].position+vertex[2].position);
	    	vec4 bcol = ust*(vertex[0].color+vertex[1].color+vertex[2].color);

	    	gl_Position = bary1;
    		color  = bcol;
    		Nv     = bnorm.xyz;
	    	P      = bary1.xyz;
    		vert   = bvert;
    		gAlpha = 0.8f;
	    	EmitVertex();

	    	gl_Position =  bary1+t;
    		color  = bcol;
    		Nv     = bnorm.xyz;
	    	P      = bary1.xyz;
    		vert   = bvert;
    		gAlpha = 0.f;
	    	EmitVertex();
    		EndPrimitive();
    	}
    }
}
