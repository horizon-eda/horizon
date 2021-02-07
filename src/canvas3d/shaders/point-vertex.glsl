#version 330

in vec3 position;
uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;
uniform float z_offset;
flat out uint pick_to_frag;

void main() {
  gl_Position = proj*view*(model*vec4(position, 1) + vec4(0,0,z_offset, 0));
  pick_to_frag = uint(gl_VertexID);
}
