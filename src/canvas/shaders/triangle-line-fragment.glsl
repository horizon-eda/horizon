#version 330
##ubo

out vec4 outputColor;
smooth in vec3 color_to_fragment;
smooth in float striper_to_fragment;
smooth in float alpha_to_fragment;
smooth in vec2 round_pos_to_fragment;
flat in int flags_to_fragment;
flat in float discard_threshold;

void main() {
  if(length(round_pos_to_fragment) > discard_threshold && abs(round_pos_to_fragment.x)>0)
    discard;
  float my_alpha = alpha;
  bool in_border = length(round_pos_to_fragment) > 1;
  if((in_border || discard_threshold < 0) && layer_flags != 3) { // and not in stencil mode
    my_alpha = 1;
  }
  else { //filled area
    if(mod(striper_to_fragment,20)>10 || layer_flags==0) {
      discard;
    }
  }
  outputColor = vec4(color_to_fragment, my_alpha);
}
