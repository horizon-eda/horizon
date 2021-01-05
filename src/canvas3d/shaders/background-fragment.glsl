#version 330
out vec4 outputColor;
in vec3 color_to_fragment;

void main() {
	outputColor = vec4(pow(color_to_fragment, vec3(1/2.2)),1);
}
