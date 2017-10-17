#version 330

layout(lines) in;
layout(triangle_strip, max_vertices = 6) out;

out vec4 color_to_fragment;
out vec3 normal_to_fragment;
in vec2 position_to_geom[2];
uniform mat4 view;
uniform mat4 proj;
uniform float layer_thickness;
uniform float layer_offset;
uniform vec4 layer_color;

void main() {
	color_to_fragment = layer_color;
	
	vec2 d = position_to_geom[1]-position_to_geom[0];
	normal_to_fragment = vec3(normalize(vec2(-d.y, d.x)), 0);
	
	gl_Position = proj*view*vec4(position_to_geom[0], layer_offset, 1);
	EmitVertex();
	
	gl_Position = proj*view*vec4(position_to_geom[1], layer_offset, 1);
	EmitVertex();
	
	gl_Position = proj*view*vec4(position_to_geom[0], layer_offset+layer_thickness, 1);
	EmitVertex();
	
	gl_Position = proj*view*vec4(position_to_geom[1], layer_offset+layer_thickness, 1);
	EmitVertex();
	
	EndPrimitive();
}
