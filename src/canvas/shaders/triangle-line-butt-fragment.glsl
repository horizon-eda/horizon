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
smooth in float striper_to_fragment;
smooth in float alpha_to_fragment;
smooth in vec2 round_pos_to_fragment;
flat in int force_outline;
flat in int flags_to_fragment;

void main() {
  float my_alpha = alpha;
  bool in_border = abs(round_pos_to_fragment.x) > 1 || abs(round_pos_to_fragment.y) > 1;
  if(in_border && layer_flags != 3) { // and not in stencil mode
    my_alpha = 1;
  }
  else { //filled area
    if(mod(striper_to_fragment,20)>10 || layer_flags==0 || force_outline!=0) {
      discard;
    }
  }
  outputColor = vec4(color_to_fragment, my_alpha);
}
