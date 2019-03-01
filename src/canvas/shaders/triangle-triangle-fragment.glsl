#version 330
##ubo

out vec4 outputColor;
smooth in vec3 color_to_fragment;
smooth in float striper_to_fragment;

void main() {
  if(mod(striper_to_fragment,20)>10) { //HATCH
    discard;
  }
  outputColor = vec4(color_to_fragment, alpha);
}
