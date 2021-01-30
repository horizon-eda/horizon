#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out uint pick;
in vec3 color_to_fragment;
in vec3 normal_to_fragment;
flat in uint instance_to_fragment;
uniform vec3 cam_normal;
uniform uint pick_base;

void main() {
  float shade = pow(min(1, abs(dot(cam_normal, normal_to_fragment))+.1), 1/2.2);
  outputColor = vec4(color_to_fragment*shade, 1);
  pick = pick_base + instance_to_fragment;
}
