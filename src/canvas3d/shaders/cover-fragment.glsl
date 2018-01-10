#version 330

out vec4 outputColor;
in vec4 color_to_fragment;
in vec3 normal_to_fragment;
uniform vec3 cam_normal;

void main() {
  outputColor = color_to_fragment*abs(dot(cam_normal, normal_to_fragment));
}
