layout (std140) uniform layer_setup
{ 
	vec3 colors[19];
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
