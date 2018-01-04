#version 330

layout(lines) in;
layout(triangle_strip, max_vertices = 6) out;

in vec2 position_to_geom[2];
uniform mat3 screenmat;
uniform float scale;
uniform vec2 offset;

vec4 t(vec2 p) {
    return vec4((screenmat*vec3(scale*p.x+offset.x , -scale*p.y+offset.y, 1)), 1);
}

void main() {
	
	vec2 p0 = position_to_geom[0];
	vec2 p1 = position_to_geom[1];
	
	vec2 v = p1-p0;
	vec2 vn = v/length(v);
	
	vec2 perp = vec2(-vn.y, vn.x)*(2/scale);
	
	
	
	gl_Position = t(p0-perp);
	EmitVertex();
	
	gl_Position = t(p1-perp);
	EmitVertex();
	
	gl_Position = t(p0+perp);
	EmitVertex();
	
	gl_Position = t(p1+perp);
	EmitVertex();
	
	EndPrimitive();
}
