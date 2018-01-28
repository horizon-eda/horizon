#version 330
in vec2 position;
in vec3 color;
in int flags;

out vec2 position_to_geom;
out vec3 color_to_geom;
out int flags_to_geom;


void main() {
	position_to_geom = position;
	color_to_geom = color;
	flags_to_geom = flags;
}
