#version 330
out vec4 outputColor;
in vec2 texcoord;
uniform sampler2D tex;
uniform float opacity;

void main() {
	vec4 c = texture(tex, texcoord);
	outputColor = vec4(c.rgb, c.a*opacity);
}
