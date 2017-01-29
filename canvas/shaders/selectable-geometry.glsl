#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 32) out;
uniform mat3 screenmat;
uniform float scale;
uniform vec2 offset;
in vec2 origin_to_geom[1];
in vec4 bb_to_geom[1];
in uint flags_to_geom[1];
out vec3 color_to_fragment;

vec4 t(vec2 p) {
    return vec4((screenmat*vec3(scale*p.x+offset.x , -scale*p.y+offset.y, 1)), 1);
}

void fbox(vec2 bl, vec2 tr) {
	gl_Position = t(bl);
	EmitVertex();

	gl_Position = t(vec2(tr.x, bl.y));
	EmitVertex();
	
	gl_Position = t(vec2(bl.x, tr.y));
	EmitVertex();
	
	gl_Position = t(tr);
	EmitVertex();

	EndPrimitive();
}


void hline(float x, float y0, float y1) {
	float w = 1/scale;
	fbox(vec2(x-w, y0), vec2(x+w, y1));
}

void vline(float y, float x0, float x1) {
	float w = 1/scale;
	fbox(vec2(x0, y-w), vec2(x1, y+w));
}

void hbox(vec2 bl, vec2 tr) {
	hline(bl.x, bl.y, tr.y);
	hline(tr.x, bl.y, tr.y);
	vline(bl.y, bl.x, tr.x);
	vline(tr.y, bl.x, tr.x);
	
}

void main() {
	float origin_size = 10/scale;
	vec2 p = origin_to_geom[0];
	vec2 bb_bl = bb_to_geom[0].xy;
	vec2 bb_tr = bb_to_geom[0].zw;
	
	const float min_sz = 10;
	if(abs(bb_tr.x - bb_bl.x) < min_sz/scale) {
		bb_tr.x += .5*min_sz/scale;
		bb_bl.x -= .5*min_sz/scale;
	}
	if(abs(bb_tr.y - bb_bl.y) < min_sz/scale) {
		bb_tr.y += .5*min_sz/scale;
		bb_bl.y -= .5*min_sz/scale;
	}
	uint flags = flags_to_geom[0];
	if(flags == uint(0)) {
		return;
	}
	color_to_fragment = vec3(1,0,1);
	if((flags & uint(2))!=uint(0)) { //prelight
		color_to_fragment = vec3(.5,0,.5);
	}
	vec3 c_save = color_to_fragment;
	
	float os = origin_size;
	
	gl_Position = t(p+vec2(0, os));
	EmitVertex();

	gl_Position = t(p+vec2(os, 0));
	EmitVertex();
	
	gl_Position = t(p+vec2(-os, 0));
	EmitVertex();
	
	gl_Position = t(p+vec2(0, -os));
	EmitVertex();
	EndPrimitive();
	
	os*=.5;
	color_to_fragment = vec3(0,0,0);
	
	gl_Position = t(p+vec2(0, os));
	EmitVertex();

	gl_Position = t(p+vec2(os, 0));
	EmitVertex();
	
	gl_Position = t(p+vec2(-os, 0));
	EmitVertex();
	
	gl_Position = t(p+vec2(0, -os));
	EmitVertex();
	
	EndPrimitive();
	
	color_to_fragment = c_save;
	
	hbox(bb_bl, bb_tr);

	EndPrimitive();
	
}
