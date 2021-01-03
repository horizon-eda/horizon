#version 330
in vec2 position;
uniform mat3 screenmat;
uniform mat3 viewmat;
uniform float angle;
uniform vec2 grid_size;
uniform vec2 grid_0;
uniform int grid_mod;
uniform float mark_size;

void divmod(int a, int b, out int d, out int m) {
	d = a/b;
	m = a - b*d;
}

mat2 rotate2d(float _angle){
	return mat2(cos(_angle),-sin(_angle),
		sin(_angle),cos(_angle));
}

void main() {
	int gr_x;
	int gr_y;
	divmod(gl_InstanceID, grid_mod, gr_y, gr_x);
	float size = mark_size;
	if(gl_VertexID >= 4) {
		size = 20.5;
	}
	vec2 vertex_pos = rotate2d(angle)*position;
	vec2 pos = round((viewmat*vec3(grid_0+grid_size*vec2(gr_x, gr_y), 1)).xy)+vec2(.5,.5)+vertex_pos*size;
	gl_Position = vec4((screenmat*vec3(pos, 1)), 1);
}
