#version 330

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

out vec3 color_to_fragment;
in vec3 position_to_geom[3];
in vec3 color_to_geom[3];
out vec3 normal_to_fragment;
uniform mat4 view;
uniform mat4 proj;

void main() {
	
	
	
	vec3 va = position_to_geom[1]-position_to_geom[0];
	vec3 vb = position_to_geom[2]-position_to_geom[0];
	vec3 normal = cross(vb, va);
	
	normal_to_fragment = normal/length(normal);
	for(int i = 0; i<3; i++) {
		gl_Position = proj*view*vec4(position_to_geom[i], 1);
		color_to_fragment = color_to_geom[i];
		EmitVertex();
	}
	EndPrimitive();
}
