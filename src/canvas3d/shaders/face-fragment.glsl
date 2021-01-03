#version 330

out vec4 outputColor;
in vec3 color_to_fragment;
in vec3 normal_to_fragment;
uniform vec3 cam_normal;

void main() {
  float shade = pow(min(1, abs(dot(cam_normal, normal_to_fragment))+.1), 1/2.2);
  outputColor = vec4(color_to_fragment*shade, 1);
}
