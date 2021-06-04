#version 330
##triangle-ubo

out vec4 outputColor;
smooth in vec3 color_to_fragment;
smooth in vec2 round_pos_to_fragment;
flat in float border_threshold;
flat in float a0;
flat in float a1;

void main() {
  if(length(round_pos_to_fragment)>1)
    discard;

  float my_alpha = 1;
  if(layer_flags == 3) { //force alpha for stencil mode
    my_alpha = alpha;
  }

  float phi = atan(round_pos_to_fragment.y, round_pos_to_fragment.x);
  if(phi < 0)
    phi += 2*PI;

  float my_a0 = phi-a0;
  if(my_a0 < 0)
    my_a0 += 2*PI;

  if(my_a0 > a1)
   discard;

  if(length(round_pos_to_fragment) < border_threshold) {
    discard;
  }
  outputColor = vec4(color_to_fragment, my_alpha);
}
