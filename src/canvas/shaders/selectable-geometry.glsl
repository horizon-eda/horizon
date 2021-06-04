#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 32) out;
in vec2 origin_to_geom[1];
in vec2 box_center_to_geom[1];
in vec2 box_dim_to_geom[1];
in float angle_to_geom[1];
in uint flags_to_geom[1];
out vec3 color_to_fragment;
out vec2 dot_to_fragment;
out vec2 size_to_fragment;

##selectable-ubo

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
	
	vec2 size = abs(box_dim_to_geom[0]);
	bool no_bb = (size.x == 0) && (size.y == 0);
	float origin_size = 10/scale;
	if(no_bb)
		origin_size = 7/scale;
	
	
	if(size.x == 0 && size.y == 0) { //point
		//nop
	}
	else if(size.x == 0) {
		bb_tr.x += 1/scale;
		bb_bl.x -= 1/scale;
	}
	else if(size.y == 0) {
		bb_tr.y += 1/scale;
		bb_bl.y -= 1/scale;
	}
	else {
		vec2 border_width = vec2(4,4); //2 border, 2 space
		float sz = .5*min_size/scale;
		bb_tr.x += border_width.x/scale;
		bb_bl.x -= border_width.x/scale;
		bb_tr.y += border_width.y/scale;
		bb_bl.y -= border_width.y/scale;
		
		bb_tr = max(bb_tr, vec2(1,1)*sz);
		bb_bl = min(bb_bl, -vec2(1,1)*sz);
	}
	
	uint flags = flags_to_geom[0];
	color_to_fragment = vec3(1,0,1);
	
	if((flags & uint(4|8))!=uint(0)) { //always or preview
		color_to_fragment = color_always.rgb;
		origin_size = 7/scale;
	}
	if((flags & uint(1))!=uint(0)) { //selected
		color_to_fragment = color_outer.rgb;
		origin_size = 10/scale;
	}
	if((flags & uint(2))!=uint(0)) { //prelight
		color_to_fragment = color_prelight.rgb;
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
	color_to_fragment = color_inner.rgb;
	
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
