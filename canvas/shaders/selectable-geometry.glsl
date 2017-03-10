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
out vec2 dot_to_fragment;

vec4 t(vec2 p) {
    return vec4((screenmat*vec3(scale*p.x+offset.x , -scale*p.y+offset.y, 1)), 1);
}

void fbox(vec2 bl, vec2 tr) {
	float wp = abs((tr.x-bl.x)*scale);
	float hp = abs((tr.y-bl.y)*scale);
	
	
	gl_Position = t(bl);
	dot_to_fragment = vec2(0,0);
	EmitVertex();

	gl_Position = t(vec2(tr.x, bl.y));
	dot_to_fragment = vec2(wp,0);
	EmitVertex();
	
	gl_Position = t(vec2(bl.x, tr.y));
	dot_to_fragment = vec2(0,hp);
	EmitVertex();
	
	gl_Position = t(tr);
	dot_to_fragment = vec2(wp,hp);
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
	
	vec2 p = origin_to_geom[0];
	vec2 bb_bl = bb_to_geom[0].xy;
	vec2 bb_tr = bb_to_geom[0].zw;
	
	const float min_sz = 10;
	bool no_bb = (bb_tr.x==bb_bl.x)&&(bb_tr.y == bb_bl.y);
	float origin_size = 10/scale;
	if(no_bb)
		origin_size = 5/scale;
	
	
	if(abs(bb_tr.x - bb_bl.x) < min_sz/scale) {
		bb_tr.x += .5*min_sz/scale;
		bb_bl.x -= .5*min_sz/scale;
	}
	if(abs(bb_tr.y - bb_bl.y) < min_sz/scale) {
		bb_tr.y += .5*min_sz/scale;
		bb_bl.y -= .5*min_sz/scale;
	}
	
	float border_width = 3;
	bb_tr.x += border_width/scale;
	bb_bl.x -= border_width/scale;
	bb_tr.y += border_width/scale;
	bb_bl.y -= border_width/scale;
	
	uint flags = flags_to_geom[0];
	if(flags == uint(0)) {
		return;
	}
	color_to_fragment = vec3(1,0,1);
	
	if((flags & uint(4))!=uint(0)) { //always
		color_to_fragment = vec3(1,1,0);
	}
	if((flags & uint(1))!=uint(0)) { //selected
		color_to_fragment = vec3(1,0,1);
	}
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
	
	if(!no_bb)
		hbox(bb_bl, bb_tr);

	EndPrimitive();
	
}
