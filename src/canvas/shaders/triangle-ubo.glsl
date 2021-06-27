layout (std140) uniform layer_setup
{ 
	vec3 colors[20];
	vec3 colors2[256];
	mat3 screenmat;
	mat3 viewmat;
	float alpha;
	float scale;
	vec2 offset;
	float min_line_width;
	uint layer_mode;
	uint stencil_mode;
};

#define LAYER_MODE_OUTLINE (0U)
#define LAYER_MODE_HATCH (1U)
#define LAYER_MODE_FILL (2U)
#define LAYER_MODE_FILL_ONLY (3U)

#define PI 3.1415926535897932384626433832795

bool get_discard(vec2 fc) {
	if(layer_mode == LAYER_MODE_OUTLINE)
		return true;

	if(layer_mode != LAYER_MODE_HATCH)
		return false;

	vec2 f = fc - offset*vec2(1,-1);
	float striper = f.x - f.y;
	return mod(striper,20)>10;
}

vec3 get_color(int color, int color2) {
	if(color2 != 0 && (color == 0 || color == 12 /*airwire*/)) {
		return colors2[color2];
	}
	else {
		return colors[color];
	}
}
