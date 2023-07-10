#version 330
uniform mat3 screenmat;
uniform vec2 cursor_position;
uniform vec2 size;
uniform float angle;
in vec2 position;
out vec2 texcoord;


void main() {   
	texcoord = (position*vec2(1,-1)+1)/2;
	gl_Position = vec4((screenmat*vec3(position*size+cursor_position, 1)), 1);
}
