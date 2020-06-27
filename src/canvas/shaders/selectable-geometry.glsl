#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 32) out;
uniform mat3 screenmat;
uniform mat3 viewmat;
uniform float scale;
uniform vec3 color_inner;
uniform vec3 color_outer;
uniform vec3 color_always;
uniform vec3 color_prelight;
in vec2 origin_to_geom[1];
in vec2 box_center_to_geom[1];
in vec2 box_dim_to_geom[1];
in float angle_to_geom[1];
in uint flags_to_geom[1];
out vec3 color_to_fragment;
out vec2 dot_to_fragment;
out vec2 size_to_fragment;

vec4 t(vec2 p) {
    return vec4((screenmat*viewmat*vec3(p, 1)), 1);
}

mat2 rotate2d(float _angle){
	return mat2(cos(_angle),-sin(_angle),
		sin(_angle),cos(_angle));
}

void main() {
	
	vec2 p = origin_to_geom[0];
	vec2 bb_bl = -(box_dim_to_geom[0]/2);
	vec2 bb_tr = (box_dim_to_geom[0]/2);
	vec2 box_center = box_center_to_geom[0];
	float angle = -angle_to_geom[0];
	
	const float min_sz = 10;
	vec2 size = abs(bb_tr-bb_bl);
	bool no_bb = (size.x == 0) && (size.y == 0);
	float origin_size = 10/scale;
	if(no_bb)
		origin_size = 7/scale;
	
	
	if((size.x > 0) && (size.x < min_sz/scale)) {
		bb_tr.x = +.5*min_sz/scale;
		bb_bl.x = -.5*min_sz/scale;
	}
	if((size.y > 0) && (size.y < min_sz/scale)) {
		bb_tr.y = +.5*min_sz/scale;
		bb_bl.y = -.5*min_sz/scale;
	}
	
	vec2 border_width = vec2(5,5);
	if(size.x == 0) {
		border_width.x = 1;
		border_width.y = 0;
	}
	if(size.y == 0) {
		border_width.y = 1;
		border_width.x = 0;
	}
		
		
	bb_tr.x += border_width.x/scale;
	bb_bl.x -= border_width.x/scale;
	bb_tr.y += border_width.y/scale;
	bb_bl.y -= border_width.y/scale;
	
	uint flags = flags_to_geom[0];
	if(flags == uint(0)) {
		return;
	}
	color_to_fragment = vec3(1,0,1);
	
	if((flags & uint(4))!=uint(0)) { //always
		color_to_fragment = color_always;
	}
	if((flags & uint(1))!=uint(0)) { //selected
		color_to_fragment = color_outer;
		origin_size = 10/scale;
	}
	if((flags & uint(2))!=uint(0)) { //prelight
		color_to_fragment = color_prelight;
	}
	dot_to_fragment = vec2(0,0);
	
	
	if(!no_bb) {
		float wp = abs((bb_tr.x-bb_bl.x)*scale);
		float hp = abs((bb_tr.y-bb_bl.y)*scale);
		size_to_fragment = vec2(wp, hp);
		
		gl_Position = t(box_center+(rotate2d(angle)*vec2(bb_bl.x, bb_tr.y)));
		dot_to_fragment = vec2(0,0);
		EmitVertex();
		
		gl_Position = t(box_center+(rotate2d(angle)*vec2(bb_tr.x, bb_tr.y)));
		dot_to_fragment = vec2(wp,0);
		EmitVertex();
		
		gl_Position = t(box_center+(rotate2d(angle)*vec2(bb_bl.x, bb_bl.y)));
		dot_to_fragment = vec2(0,hp);
		EmitVertex();
		
		gl_Position = t(box_center+(rotate2d(angle)*vec2(bb_tr.x, bb_bl.y)));
		dot_to_fragment = vec2(wp,hp);
		EmitVertex();
		
		EndPrimitive();
	}

	float os = origin_size;
	size_to_fragment = vec2(-1, -1);
	
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
	color_to_fragment = color_inner;
	
	gl_Position = t(p+vec2(0, os));
	EmitVertex();

	gl_Position = t(p+vec2(os, 0));
	EmitVertex();
	
	gl_Position = t(p+vec2(-os, 0));
	EmitVertex();
	
	gl_Position = t(p+vec2(0, -os));
	EmitVertex();
	
	EndPrimitive();
}
