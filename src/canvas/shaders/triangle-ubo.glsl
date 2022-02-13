layout (std140) uniform layer_setup
{
	vec3 colors[21];
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
#define LAYER_MODE_DOTTED (4U)

#define PI 3.1415926535897932384626433832795
#define COS_PI_SIXTH 0.86602540378443864676372317075

bool get_discard(vec2 fc) {
	if(layer_mode == LAYER_MODE_OUTLINE)
		return true;

	if(layer_mode == LAYER_MODE_HATCH) {
		vec2 f = fc - offset*vec2(1,-1);
		float striper = f.x - f.y;
		return mod(striper,20)>10;
	} else if(layer_mode == LAYER_MODE_DOTTED) {
		float base_spacing = 10;
		vec2 f = fc - offset*vec2(1,-1);
		vec2 spacing = vec2(COS_PI_SIXTH * 2 * base_spacing, base_spacing);
		vec2 f1 = f - round(f / spacing) * spacing;
		vec2 ff = f + vec2(base_spacing * COS_PI_SIXTH, base_spacing / 2);
		vec2 f2 = ff - round(ff / spacing) * spacing;
		return (length(f1) < 2.5) || (length(f2) < 2.5);
	} else {
		return false;
	}
}

vec3 get_color(int color, int color2) {
	if(color2 != 0 && (color == 0 || color == 12 /*airwire*/)) {
		return colors2[color2];
	}
	else {
		return colors[color];
	}
}
