#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 64) out;


in vec2 p0_to_geom[1];
in vec2 p1_to_geom[1];
in vec2 p2_to_geom[1];
in int color_to_geom[1];
in int color2_to_geom[1];
in int lod_to_geom[1];
smooth out vec3 color_to_fragment;

##triangle-ubo

vec4 t(vec2 p) {
    return vec4((screenmat*viewmat*vec3(p, 1)), 1);
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

	color_to_fragment = get_color(color_to_geom[0], color2_to_geom[0]);

	float width = min_line_width*.5/scale;
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
