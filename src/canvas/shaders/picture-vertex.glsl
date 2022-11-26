#version 330
uniform mat3 screenmat;
uniform mat3 viewmat;
uniform vec2 shift;
uniform vec2 size;
uniform float angle;
in vec2 position;
out vec2 texcoord;
out vec2 border_pos;

mat2 rotate2d(float _angle){
	return mat2(cos(_angle),-sin(_angle),
		sin(_angle),cos(_angle));
}

void main() {
	
	vec2 t = rotate2d(-angle)*(position*size)+shift;
	texcoord = (position*vec2(1,-1)+1)/2;
	border_pos = position*size;
	gl_Position = vec4((screenmat*viewmat*vec3(t, 1)), 1);
}
