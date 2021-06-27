#version 330
##triangle-ubo

out vec4 outputColor;
smooth in vec3 color_to_fragment;

void main() {
  float my_alpha = 1;
  if(layer_mode == LAYER_MODE_FILL_ONLY) { //force alpha for fill only mode
    my_alpha = alpha;
  }
  outputColor = vec4(color_to_fragment, my_alpha);
}
