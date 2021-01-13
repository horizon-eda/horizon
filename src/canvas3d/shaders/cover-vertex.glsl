#version 330

in vec2 position;
uniform float layer_offset;
uniform mat4 view;
uniform mat4 proj;

void main() {
  gl_Position = proj*view*vec4(position/1e6, layer_offset, 1);
}
