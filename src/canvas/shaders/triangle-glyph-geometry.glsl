#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;


in vec2 p0_to_geom[1];
in vec2 p1_to_geom[1];
in vec2 p2_to_geom[1];
in int oid_to_geom[1];
in int type_to_geom[1];
in int color_to_geom[1];
in int flags_to_geom[1];
in int lod_to_geom[1];
smooth out vec3 color_to_fragment;
smooth out vec2 texcoord_to_fragment;
out float lod_alpha;

##ubo

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
	lod_alpha = 1;
	if(lod_to_geom[0] != 0) {
		float lod_size = lod_to_geom[0]*.02e6;
		float lod_size_px = lod_size*scale;
		const float th1 = 15;
		const float th2 = 5;
		if(lod_size_px<th1)
			lod_alpha = (lod_size_px-th2)/(th1-th2);
		if(lod_size_px<th2)
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
	color_to_fragment = apply_highlight(color, highlight, type);
	

	float aspect = p2.x;
	uint bits = floatBitsToUint(p2.y);
	float glyph_x = (bits>>22)&uint(0x3ff);
	float glyph_y = (bits>>12)&uint(0x3ff);
	float glyph_w = (bits>>6)&uint(0x3f);
	float glyph_h = (bits>>0)&uint(0x3f);
	
	vec2 o = vec2(-p1.y, p1.x);
	o /= length(o);
	o *= length(p1)*aspect;
	
	gl_Position = t(p0);
	texcoord_to_fragment = vec2(glyph_x,glyph_y)/1024;
	EmitVertex();
	
	gl_Position = t(p0+p1);
	texcoord_to_fragment = vec2(glyph_x+glyph_w,glyph_y)/1024;
	EmitVertex();

	gl_Position = t(p0+o);
	texcoord_to_fragment = vec2(glyph_x,glyph_y+glyph_h)/1024;
	EmitVertex();
	
	gl_Position = t(p0+p1+o);
	texcoord_to_fragment = vec2(glyph_x+glyph_w,glyph_y+glyph_h)/1024;
	EmitVertex();
	
	EndPrimitive();
	
}
