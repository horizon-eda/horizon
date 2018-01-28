#version 330
in vec2 p0;
in vec2 p1;
in vec2 p2;
in int oid;
in int type;
in int color;
in int flags;
in int lod;

out vec2 p0_to_geom;
out vec2 p1_to_geom;
out vec2 p2_to_geom;
out int oid_to_geom;
out int type_to_geom;
out int color_to_geom;
out int flags_to_geom;
out int lod_to_geom;


void main() {
	p0_to_geom = p0;
	p1_to_geom = p1;
	p2_to_geom = p2;
	oid_to_geom = oid;
	type_to_geom = type;
	color_to_geom = color;
	flags_to_geom = flags;
	lod_to_geom = lod;
}
