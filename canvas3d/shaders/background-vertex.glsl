#version 330

in vec2 position;
out vec3 color_to_fragment;
uniform vec3 color_top;
uniform vec3 color_bottom;

void main() {
	if(gl_VertexID < 2) {
		color_to_fragment = color_top;
	}
	else {
		color_to_fragment = color_bottom;
	}
	gl_Position = vec4(position, -1, 1);
}
