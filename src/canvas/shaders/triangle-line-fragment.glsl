#version 330
##triangle-ubo

out vec4 outputColor;
smooth in vec3 color_to_fragment;
smooth in float alpha_to_fragment;
smooth in vec2 round_pos_to_fragment;
flat in float discard_threshold;

void main() {
  if(length(round_pos_to_fragment) > discard_threshold && abs(round_pos_to_fragment.x)>0)
    discard;
  float my_alpha = alpha;
  bool in_border = length(round_pos_to_fragment) > 1;
  if((in_border || discard_threshold < 0) && layer_mode != LAYER_MODE_FILL_ONLY) { // and not in fill only mode
    if(stencil_mode == 1U)
      discard;
    my_alpha = 1;
  }
  else { //filled area
    if(get_discard(gl_FragCoord.xy)) {
      if(stencil_mode == 1U)
        my_alpha = 0;
      else
        discard;
    }
  }
  outputColor = vec4(color_to_fragment, my_alpha);
}
