#version 330

out vec4 outputColor;
in vec4 color_to_fragment;
in vec3 normal_to_fragment;
uniform vec3 cam_normal;

void main() {
  if(isnan(normal_to_fragment).x)
    discard;
  outputColor = vec4(color_to_fragment.rgb*(abs(dot(cam_normal, normal_to_fragment))+.1), color_to_fragment.a);
}
