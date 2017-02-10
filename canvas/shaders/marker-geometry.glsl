#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;
uniform mat3 screenmat;
uniform float scale;
uniform vec2 offset;
uniform float alpha;
in vec2 position_to_geom[1];
in vec3 color_to_geom[1];
in int flags_to_geom[1];
flat out vec3 color_to_fragment;
smooth out vec2 draw_pos_to_fragment;

vec4 t(vec2 p, vec2 q) {
    return vec4((screenmat*vec3(scale*p.x+offset.x+q.x , -scale*p.y+offset.y+q.y, 1)), 1);
}

void main() {
	vec2 position = position_to_geom[0];
	vec3 color = color_to_geom[0];
	vec2 size = vec2(40, 40);
	color_to_fragment = color;
	
	draw_pos_to_fragment = vec2(0,0);
	gl_Position = t(position, vec2(0,0));
	EmitVertex();
	
	draw_pos_to_fragment = vec2(8,0);
	gl_Position = t(position, vec2(size.x,0));
	EmitVertex();
	
	draw_pos_to_fragment = vec2(0,8);
	gl_Position = t(position, vec2(0,size.y));
	EmitVertex();
	
	draw_pos_to_fragment = vec2(8,8);
	gl_Position = t(position, size);
	EmitVertex();
	
	EndPrimitive();
}
