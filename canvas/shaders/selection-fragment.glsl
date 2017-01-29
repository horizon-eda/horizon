#version 330
out vec4 outputColor;
in vec2 x;
in vec2 dim;
uniform float scale;

void main() {
	float border = 3/scale;
	float alpha = .2;
	if((x.x < border) || (x.x > dim.x-border)) {
		alpha = 1;
	}
	if((x.y < border) || (x.y > dim.y-border)) {
		alpha = 1;
	}
	outputColor = vec4(1,0,0,alpha);
}
