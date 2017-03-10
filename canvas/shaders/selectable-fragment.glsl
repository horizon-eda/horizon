#version 330
out vec4 outputColor;
in vec3 color_to_fragment;
in vec2 dot_to_fragment;

void main() {
  if((mod(dot_to_fragment.x, 20) > 10) != (mod(dot_to_fragment.y, 20) > 10))
    outputColor = vec4(0,0,0 ,1);
  else
    outputColor = vec4(color_to_fragment ,1);
}
