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

void main() {
  if(mod(striper_to_fragment,20)>10) { //HATCH
    discard;
  }
  outputColor = vec4(color_to_fragment, alpha);
}
