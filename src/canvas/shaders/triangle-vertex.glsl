#version 330
layout(location=0) in vec2 p0;
layout(location=1) in vec2 p1;
layout(location=2) in vec2 p2;
layout(location=3) in int color;
layout(location=4) in int lod;
layout(location=5) in int color2;

out vec2 p0_to_geom;
out vec2 p1_to_geom;
out vec2 p2_to_geom;
out int color_to_geom;
out int color2_to_geom;
out int lod_to_geom;


void main() {
	p0_to_geom = p0;
	p1_to_geom = p1;
	p2_to_geom = p2;
	color_to_geom = color;
	color2_to_geom = color2;
	lod_to_geom = lod;
}
