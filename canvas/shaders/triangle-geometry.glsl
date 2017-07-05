#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 64) out;
uniform mat3 screenmat;
uniform float scale;
uniform vec2 offset;
uniform float alpha;
uniform int work_layer;
uniform uint types_visible;
in vec2 p0_to_geom[1];
in vec2 p1_to_geom[1];
in vec2 p2_to_geom[1];
in int oid_to_geom[1];
in int type_to_geom[1];
in int color_to_geom[1];
in int layer_to_geom[1];
smooth out vec3 color_to_fragment;
smooth out float striper_to_fragment;
smooth out float alpha_to_fragment;
smooth out vec2 round_pos_to_fragment;
flat out float line_length_to_fragment;
flat out float line_height_px_to_fragment;
flat out int flags_to_fragment;

struct layer_display {
	vec3 color;
	int flags;
};


layout (std140) uniform layer_setup
{ 
	vec3 colors[11];
	layer_display layers[36];
};

int layer = layer_to_geom[0];
int mode = (layers[layer].flags>>1)&3;

#define PI 3.1415926535897932384626433832795

vec4 t(vec2 p) {
    return vec4((screenmat*vec3(scale*p.x+offset.x , -scale*p.y+offset.y, 1)), 1);
}

float t2(vec2 p) {
	if(mode==1)
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
	color_to_fragment = vec3(1,0,0);
	
	if((types_visible & uint(1<<type_to_geom[0])) == uint(0))
		return;
	
	if(layer != work_layer && (layers[layer].flags & int(1)) == 0) {
		return;
	}
	
	if(color_to_geom[0] == 0) {
		color_to_fragment = layers[layer].color;
	}
	else {
		color_to_fragment = colors[color_to_geom[0]];
	}
	
	flags_to_fragment = 0;
	if(mode == 0) {
		flags_to_fragment = 2;
	}
	if(!isnan(p2.y)) {
		round_pos_to_fragment = vec2(0,0);
		line_height_px_to_fragment = 10;
		line_length_to_fragment = 10;
		
		gl_Position = t(p0);
		striper_to_fragment = t2(p0);
		alpha_to_fragment = alpha;
		EmitVertex();
		
		gl_Position = t(p1);
		striper_to_fragment = t2(p1);
		alpha_to_fragment = alpha;
		EmitVertex();
		
		gl_Position = t(p2);
		striper_to_fragment = t2(p2);
		alpha_to_fragment = alpha;
		EmitVertex();
	}
	else {
		float width = p2.x/2;
		
		width = max(width, .5/scale);
		vec2 v = p1-p0;
		vec2 o = vec2(-v.y, v.x);
		o /= length(o);
		o *= width;
		vec2 vw = (v/length(v))*width;
		vec2 p0x = p0-vw;
		vec2 p1x = p1+vw;
		
		alpha_to_fragment = alpha;
		float l = length(v)/width+2;
		line_height_px_to_fragment = width*scale;
		line_length_to_fragment = length(v)/width;
		
		striper_to_fragment = t2(p0x-o);
		gl_Position = t(p0x-o);
		round_pos_to_fragment = vec2(-l/2,-1);
		EmitVertex();
		
		striper_to_fragment = t2(p0x+o);
		gl_Position = t(p0x+o);
		round_pos_to_fragment = vec2(-l/2,1);
		EmitVertex();

		striper_to_fragment = t2(p1x-o);
		gl_Position = t(p1x-o);
		round_pos_to_fragment = vec2(l/2,-1);
		EmitVertex();
		
		striper_to_fragment = t2(p1x+o);
		gl_Position = t(p1x+o);
		round_pos_to_fragment = vec2(l/2,1);
		EmitVertex();
	
	}
	
	EndPrimitive();
	
}
