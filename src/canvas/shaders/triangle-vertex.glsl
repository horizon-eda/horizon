#version 330
layout(location=0) in vec2 p0;
layout(location=1) in vec2 p1;
layout(location=2) in vec2 p2;
layout(location=3) in int type;
layout(location=4) in int color;
layout(location=5) in int flags;
layout(location=6) in int lod;

out vec2 p0_to_geom;
out vec2 p1_to_geom;
out vec2 p2_to_geom;
out int type_to_geom;
out int color_to_geom;
out int flags_to_geom;
out int lod_to_geom;


void main() {
	p0_to_geom = p0;
	p1_to_geom = p1;
	p2_to_geom = p2;
	type_to_geom = type;
	color_to_geom = color;
	flags_to_geom = flags;
	lod_to_geom = lod;
}
