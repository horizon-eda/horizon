#version 330
in vec2 position;
in vec3 color;
in uint flags;

out vec2 position_to_geom;
out vec3 color_to_geom;
out uint flags_to_geom;


void main() {
	position_to_geom = position;
	color_to_geom = color;
	flags_to_geom = flags;
}
