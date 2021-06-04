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
out vec2 pos_to_fragment;
out float r0_to_fragment;
out float r1_to_fragment;
out float a0_to_fragment;
out float dphi_to_fragment;
out float origin_size_to_fragment;

##selectable-ubo

vec4 t(vec2 p) {
    return vec4((screenmat*viewmat*vec3(p, 1)), 1);
}

mat2 rotate2d(float _angle){
	return mat2(cos(_angle),-sin(_angle),
		sin(_angle),cos(_angle));
}

vec2 p2r(float phi, float l) {
	return vec2(cos(phi), sin(phi))*l;
}

void main() {
	vec2 center = origin_to_geom[0];
    float r0 = box_center_to_geom[0].x;
    float r1 = box_center_to_geom[0].y;
    float a0 = box_dim_to_geom[0].x;
    float dphi = box_dim_to_geom[0].y;
    uint flags = flags_to_geom[0];

    origin_size_to_fragment = 10;
    if((flags & uint(4|8))!=uint(0)) { //always or preview
		color_to_fragment = color_always.rgb;
		origin_size_to_fragment = 7;
	}
	if((flags & uint(1))!=uint(0)) { //selected
		color_to_fragment = color_outer.rgb;
		origin_size_to_fragment = 10;
	}
	if((flags & uint(2))!=uint(0)) { //prelight
		color_to_fragment = color_prelight.rgb;
	}

    float r = r1+(4/scale);
	float h = r*2;
	float z = 2;

    r0_to_fragment = r0*scale;
    r1_to_fragment = r1*scale;
    a0_to_fragment = a0;
    dphi_to_fragment = dphi;
	
	for(int i = 0; i<3; i++) {
		float phi = PI*(2./3)*i;
		gl_Position = t(center+p2r(phi, h));
        pos_to_fragment = p2r(phi, h*scale);
		EmitVertex();
	}
	
	
	EndPrimitive();

	
}
