#version 330
##ubo

out vec4 outputColor;
smooth in vec3 color_to_fragment;

void main() {
  if(get_striper_discard(gl_FragCoord.xy)) { //HATCH
    discard;
  }
  outputColor = vec4(color_to_fragment, alpha);
}
