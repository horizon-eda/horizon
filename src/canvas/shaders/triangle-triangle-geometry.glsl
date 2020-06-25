#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 3) out;


in vec2 p0_to_geom[1];
in vec2 p1_to_geom[1];
in vec2 p2_to_geom[1];
in int color_to_geom[1];
in int color2_to_geom[1];
in int lod_to_geom[1];
smooth out vec3 color_to_fragment;

##ubo

int mode = layer_flags;

vec4 t(vec2 p) {
    return vec4((screenmat*viewmat*vec3(p, 1)), 1);
}

vec2 p2r(float phi, float l) {
	return vec2(cos(phi), sin(phi))*l;
}

void main() {
	vec2 p0 = p0_to_geom[0];
	vec2 p1 = p1_to_geom[0];
	vec2 p2 = p2_to_geom[0];

	color_to_fragment = get_color(color_to_geom[0], color2_to_geom[0]);

	gl_Position = t(p0);
	EmitVertex();
	
	gl_Position = t(p1);
	EmitVertex();
	
	gl_Position = t(p2);
	EmitVertex();
	
	
	EndPrimitive();
	
}
