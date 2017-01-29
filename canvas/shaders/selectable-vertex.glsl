#version 330
in vec2 origin;
in vec4 bb;
in uint flags;

out vec2 origin_to_geom;
out vec4 bb_to_geom;
out uint flags_to_geom;

void main() {
	
	origin_to_geom = origin;
	bb_to_geom = bb;
	flags_to_geom = flags;
	
	
}
