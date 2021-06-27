#version 330
##triangle-ubo

out vec4 outputColor;
smooth in vec3 color_to_fragment;
smooth in float alpha_to_fragment;
smooth in vec2 round_pos_to_fragment;

void main() {
  float my_alpha = alpha;
  bool in_border = abs(round_pos_to_fragment.x) > 1 || abs(round_pos_to_fragment.y) > 1;
  if(in_border && layer_mode != LAYER_MODE_FILL_ONLY) { // and not in stencil mode
    my_alpha = 1;
  }
  else { //filled area
    if(get_discard(gl_FragCoord.xy)) {
      discard;
    }
  }
  outputColor = vec4(color_to_fragment, my_alpha);
}
