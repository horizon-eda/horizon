#version 330

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

out vec4 color_to_fragment;
in vec2 position_to_geom[3];
out vec3 normal_to_fragment;
uniform mat4 view;
uniform mat4 proj;


uniform float layer_thickness;
uniform float layer_offset;
uniform vec4 layer_color;

void main() {
	color_to_fragment = layer_color;
	
	
	for(int i = 0; i<3; i++) {
		gl_Position = proj*view*vec4(position_to_geom[i], layer_offset, 1);
		normal_to_fragment = vec3(0,0,-1);
		EmitVertex();
	}
	EndPrimitive();
	if(layer_color.a == 1) {
		for(int i = 0; i<3; i++) {
			gl_Position = proj*view*vec4(position_to_geom[i], layer_offset+layer_thickness, 1);
			normal_to_fragment = vec3(0,0,1);
			EmitVertex();
		}
	}
	EndPrimitive();
	
}
