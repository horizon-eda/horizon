#version 330
##triangle-ubo

out vec4 outputColor;
smooth in vec3 color_to_fragment;
smooth in vec2 round_pos_to_fragment;
flat in float border_threshold;

void main() {
  if(length(round_pos_to_fragment)>1)
    discard;
  float my_alpha = alpha;
  if(length(round_pos_to_fragment) > border_threshold && layer_flags != 3) {
    my_alpha = 1;
  }
  else { //filled area
    if(get_striper_discard(gl_FragCoord.xy) || layer_flags == 0) { //HATCH
      discard;
    } 
  }
  outputColor = vec4(color_to_fragment, my_alpha);
}
