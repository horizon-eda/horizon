#version 330
in vec2 position;
uniform mat3 screenmat;
uniform float scale;
uniform vec2 offset;
uniform float grid_size;
uniform vec2 grid_0;
uniform int grid_mod;
uniform float mark_size;

void main() {
	vec2 pos;
	int gr_x = int(mod(gl_InstanceID, grid_mod));
	int gr_y = int(gl_InstanceID/grid_mod);
	pos.x =  scale*(grid_0.x+grid_size*gr_x)+offset.x+position.x*mark_size;
	pos.y = -scale*(grid_0.y+grid_size*gr_y)+offset.y+position.y*mark_size;
	gl_Position = vec4((screenmat*vec3(pos, 1)), 1);
}
