#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out uint pick;
flat in uint pick_to_frag;
uniform uint pick_base;

void main() {
  outputColor = vec4(1,0,0,1);
  gl_FragDepth =  gl_FragCoord.z *(1-0.001);
  pick = pick_base + pick_to_frag;
}
