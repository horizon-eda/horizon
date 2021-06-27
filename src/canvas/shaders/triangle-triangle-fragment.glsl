#version 330
##triangle-ubo

out vec4 outputColor;
smooth in vec3 color_to_fragment;

void main() {
  if(get_discard(gl_FragCoord.xy)) {
    discard;
  }
  outputColor = vec4(color_to_fragment, alpha);
}
