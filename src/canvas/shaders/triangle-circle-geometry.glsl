#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 64) out;


in vec2 p0_to_geom[1];
in vec2 p1_to_geom[1];
in vec2 p2_to_geom[1];
in int color_to_geom[1];
in int lod_to_geom[1];
smooth out vec3 color_to_fragment;
smooth out vec2 round_pos_to_fragment;
smooth out float striper_to_fragment;
flat out float border_threshold;

##ubo

int mode = layer_flags;

vec4 t(vec2 p) {
    return vec4((screenmat*viewmat*vec3(p, 1)), 1);
}

float t2(vec2 p) {
	if(mode==1) //HATCH
		return scale*p.x -scale*p.y;
	return 0.0;
}

vec2 p2r(float phi, float l) {
	return vec2(cos(phi), sin(phi))*l;
}

void main() {
	vec2 p0 = p0_to_geom[0];
	vec2 p1 = p1_to_geom[0];
	vec2 p2 = p2_to_geom[0];
	
	color_to_fragment = colors[color_to_geom[0]];
	
	float r = p1.x;
	float h = r*2;
	float z = 2;
	
	float border_width = min_line_width;
	float ym = 1/(1-border_width/(h*scale));
	border_threshold = 1-border_width/(r*scale);
	
	for(int i = 0; i<3; i++) {
		float phi = PI*(2./3)*i;
		gl_Position = t(p0+p2r(phi, h));
		round_pos_to_fragment = p2r(phi,z);
		striper_to_fragment = t2(p0+p2r(phi, h));
		EmitVertex();
	}
	
	
	EndPrimitive();
	
}
