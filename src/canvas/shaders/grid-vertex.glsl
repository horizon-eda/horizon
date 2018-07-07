#version 330
in vec2 position;
uniform mat3 screenmat;
uniform mat3 viewmat;
uniform float grid_size;
uniform vec2 grid_0;
uniform int grid_mod;
uniform float mark_size;

void main() {
	int gr_x = int(mod(gl_InstanceID, grid_mod));
	int gr_y = int(gl_InstanceID/grid_mod);
	vec2 pos = (viewmat*vec3(grid_0+grid_size*vec2(gr_x, gr_y), 1)).xy+position*mark_size;
	gl_Position = vec4((screenmat*vec3(pos, 1)), 1);
}
