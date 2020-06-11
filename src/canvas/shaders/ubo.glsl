layout (std140) uniform layer_setup
{ 
	vec3 colors[19];
	mat3 screenmat;
	mat3 viewmat;
	float alpha;
	float scale;
	float min_line_width;
	int layer_flags;
};

#define PI 3.1415926535897932384626433832795
