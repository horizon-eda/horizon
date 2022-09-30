#version 330

in vec2 position;
out vec3 pos_to_fragment;
out vec3 normal_to_fragment;
uniform float layer_offset;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 cam_pos;

void main() {
  pos_to_fragment = vec3(position/1e6, layer_offset);
  gl_Position = proj*view*vec4(pos_to_fragment, 1);

  // If looking above the normal is the top face otherwise the bottom
  if (pos_to_fragment.z < cam_pos.z) {
    normal_to_fragment = vec3(0,0,1);
  } else {
    normal_to_fragment = vec3(0,0,-1);
  }
}
