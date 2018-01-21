#version 330

layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 6) out;

out vec4 color_to_fragment;
out vec3 normal_to_fragment;
in vec2 position_to_geom[4];
uniform mat4 view;
uniform mat4 proj;
uniform float layer_thickness;
uniform float layer_offset;
uniform vec4 layer_color;

void main() {
	const int i0 = 1;
	const int i1 = 2;
	
	for(int i = 0; i<4; i++) {
		if(isnan(position_to_geom[i].x))
			return;
	}
	
	color_to_fragment = layer_color;
	
	vec2 d_this = position_to_geom[1]-position_to_geom[2];
	vec3 n_this = vec3(normalize(vec2(-d_this.y, d_this.x)), 0);
	
	vec2 d_before = position_to_geom[0]-position_to_geom[1];
	vec3 n_before = vec3(normalize(vec2(-d_before.y, d_before.x)), 0);
	
	vec2 d_after = position_to_geom[2]-position_to_geom[3];
	vec3 n_after = vec3(normalize(vec2(-d_after.y, d_after.x)), 0);
	
	vec3 n0 = normalize(n_this+n_before);
	vec3 n1 = normalize(n_this+n_after);
	
	float a0 = abs(dot(d_before, d_this)/(length(d_before)*length(d_this)));
	float a1 = abs(dot(d_after, d_this)/(length(d_after)*length(d_this)));
	
	const float thre = sqrt(3.0/4.0); //cos(30)
	if(a0 < thre || a1 < thre) {
		n0 = n_this;
		n1 = n0;
	}
	
	normal_to_fragment = n0;
	gl_Position = proj*view*vec4(position_to_geom[i0], layer_offset, 1);
	EmitVertex();
	
	normal_to_fragment = n1;
	gl_Position = proj*view*vec4(position_to_geom[i1], layer_offset, 1);
	EmitVertex();
	
	normal_to_fragment = n0;
	gl_Position = proj*view*vec4(position_to_geom[i0], layer_offset+layer_thickness, 1);
	EmitVertex();
	
	normal_to_fragment = n1;
	gl_Position = proj*view*vec4(position_to_geom[i1], layer_offset+layer_thickness, 1);
	EmitVertex();
	
	EndPrimitive();
}
