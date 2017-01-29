#version 330
uniform vec3 color;
out vec4 outputColor;

void main() {
  outputColor = vec4(color,1);
}
