#version 330
uniform mat3 screenmat;
uniform mat3 viewmat;
out vec2 x;
out vec2 dim;
uniform vec2 a,b;

void main() {
	vec2 a_screen = (viewmat*vec3(a,1)).xy;
	vec2 b_screen = (viewmat*vec3(b,1)).xy;
	vec2 bl = min(a_screen,b_screen);
	vec2 tr = max(a_screen,b_screen);
	vec2 t = vec2(0,0);
	if(gl_VertexID == 0) {
		t = vec2(bl.x, bl.y);
	}
	else if(gl_VertexID == 1) {
		t = vec2(bl.x, tr.y);
	}
	else if(gl_VertexID == 2) {
		t = vec2(tr.x, bl.y);
	}
	else if(gl_VertexID == 3) {
		t = vec2(tr.x, tr.y);
	}
	dim = abs(tr - bl);
	x=t-bl;
	
	gl_Position = vec4((screenmat*vec3(t, 1)), 1);
}
