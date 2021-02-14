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
	int layer_flags;
};

#define PI 3.1415926535897932384626433832795

bool get_striper_discard(vec2 fc) {
	if(layer_flags != 1)
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
