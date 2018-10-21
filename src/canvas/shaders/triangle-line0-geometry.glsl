#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 64) out;


in vec2 p0_to_geom[1];
in vec2 p1_to_geom[1];
in vec2 p2_to_geom[1];
in int oid_to_geom[1];
in int type_to_geom[1];
in int color_to_geom[1];
in int flags_to_geom[1];
in int lod_to_geom[1];
smooth out vec3 color_to_fragment;

layout (std140) uniform layer_setup
{ 
	vec3 colors[15];
	mat3 screenmat;
	mat3 viewmat;
	vec3 layer_color;
	float alpha;
	float scale;
	uint types_visible;
	uint types_force_outline;
	int layer_flags;
	int highlight_mode;
	float highlight_dim;
	float highlight_shadow;
	float highlight_lighten;
};

int mode = layer_flags;

#define PI 3.1415926535897932384626433832795

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
	if(lod_to_geom[0] != 0) {
		float lod_size = lod_to_geom[0]*.02e6;
		float lod_size_px = lod_size*scale;
		if(lod_size_px<10)
			return;
	}


	vec2 p0 = p0_to_geom[0];
	vec2 p1 = p1_to_geom[0];
	vec2 p2 = p2_to_geom[0];
	color_to_fragment = vec3(1,0,0);
	
	int flags = flags_to_geom[0];
	if((flags & (1<<0)) != 0) { //hidden
		return;
	}
	int type = type_to_geom[0];
	
	if((types_visible & uint(1<<type)) == uint(0))
		return;
	
	vec3 color;
	if(color_to_geom[0] == 0) {
		color = layer_color;
	}
	else {
		color = colors[color_to_geom[0]];
	}
	
	bool highlight = (type == 1 || ((flags & (1<<1)) != 0));
	
	if(highlight_mode == 0) {
		if(highlight)
			color += highlight_lighten; //ugly lighten
	}
	else if(highlight_mode == 1) { //dim
		if(!highlight)
			color *= highlight_dim; //ugly darken
		else if(type == 1)
			color += highlight_lighten; //ugly lighten
	}
	else if(highlight_mode == 2) { //shadow
		if(!highlight)
			color = vec3(1, 1, 1)*highlight_shadow;
		else if(type == 1)
			color += highlight_lighten; //ugly lighten
	}
	
	color_to_fragment = color;
	

	float width = .5/scale;
	vec2 v = p1-p0;
	vec2 o = vec2(-v.y, v.x);
	o /= length(o);
	o *= width;
	vec2 vw = (v/length(v))*width;
	vec2 p0x = p0-vw;
	vec2 p1x = p1+vw;
	
	gl_Position = t(p0x-o);
	EmitVertex();
	
	gl_Position = t(p0x+o);
	EmitVertex();

	gl_Position = t(p1x-o);
	EmitVertex();
	
	gl_Position = t(p1x+o);
	EmitVertex();
	
	EndPrimitive();
	
}
