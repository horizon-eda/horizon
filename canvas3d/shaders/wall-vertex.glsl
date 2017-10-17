#version 330

in vec2 position;
out vec2 position_to_geom;


void main() {
  //gl_Position = proj*view*vec4(position, 1, 1);
  //color_to_fragment = vec3(1,0,0);
  position_to_geom = position/1e6;
}
