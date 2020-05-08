layout (std140) uniform layer_setup
{ 
	vec3 colors[18];
	mat3 screenmat;
	mat3 viewmat;
	vec3 layer_color;
	float alpha;
	float scale;
	float min_line_width;
	uint types_visible;
	uint types_force_outline;
	int layer_flags;
	int highlight_mode;
	float highlight_dim;
	float highlight_lighten;
};

const uint color_shadow = 15U;

vec3 apply_highlight(vec3 color, bool highlight, int type) {
	if(highlight_mode == 0) {
		if(highlight)
			return color + highlight_lighten; //ugly lighten
		else
			return color;
	}
	else if(highlight_mode == 1) { //dim
		if(!highlight)
			return color * highlight_dim; //ugly darken
		else if(type == 1)
			return color + highlight_lighten; //ugly lighten
		else
			return color;
	}
	else if(highlight_mode == 2) { //shadow
		if(!highlight)
			return colors[color_shadow];
		else if(type == 1)
			return color + highlight_lighten; //ugly lighten
		else
			return color;
	}
	return color;
}
