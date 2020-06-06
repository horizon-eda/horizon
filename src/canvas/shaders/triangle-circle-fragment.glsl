#version 330
##ubo

out vec4 outputColor;
smooth in vec3 color_to_fragment;
smooth in vec2 round_pos_to_fragment;
flat in float border_threshold;
smooth in float striper_to_fragment;
flat in int force_outline;


void main() {
  if(length(round_pos_to_fragment)>1)
    discard;
  float my_alpha = alpha;
  if(length(round_pos_to_fragment) > border_threshold && layer_flags != 3) {
    my_alpha = 1;
  }
  else { //filled area
    if(mod(striper_to_fragment,20)>10 || layer_flags==0 || force_outline!=0) {
      discard;
    }
  }
  outputColor = vec4(color_to_fragment, my_alpha);
}
