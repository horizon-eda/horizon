#version 330
layout (std140) uniform layer_setup
{ 
	vec3 colors[15];
	mat3 screenmat;
	mat3 viewmat;
	vec3 layer_color;
	float alpha;
	float scale;
	uint types_visible;
	uint types_force_outline;
	int layer_flags;
	int highlight_mode;
	float highlight_dim;
	float highlight_shadow;
	float highlight_lighten;
};

out vec4 outputColor;
smooth in vec3 color_to_fragment;

void main() {
  float my_alpha = 1;
  if(layer_flags == 3) { //force alpha for stencil mode
    my_alpha = alpha;
  }
  outputColor = vec4(color_to_fragment, my_alpha);
}
