#version 330
out vec4 outputColor;
in vec3 color_to_fragment;

void main() {
	outputColor = vec4(color_to_fragment,1);
}
