#version 330
in vec2 p0;
in vec2 p1;
in vec2 p2;
in vec3 color;
in int flags;

out vec2 p0_to_geom;
out vec2 p1_to_geom;
out vec2 p2_to_geom;
out vec3 color_to_geom;
out int flags_to_geom;


void main() {
	p0_to_geom = p0;
	p1_to_geom = p1;
	p2_to_geom = p2;
	color_to_geom = color;
	flags_to_geom = flags;
}
