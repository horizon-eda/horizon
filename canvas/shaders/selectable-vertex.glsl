#version 330
in vec2 origin;
in vec2 box_center;
in vec2 box_dim;
in float angle;
in uint flags;

out vec2 origin_to_geom;
out vec2 box_center_to_geom;
out vec2 box_dim_to_geom;
out float angle_to_geom;
out uint flags_to_geom;

void main() {
	origin_to_geom = origin;
	box_center_to_geom = box_center;
	box_dim_to_geom = box_dim;
	angle_to_geom = angle;
	flags_to_geom = flags;
}
