#version 330

out vec4 outputColor;
uniform vec3 color;

void main() {
  outputColor = vec4(color,1);
}
