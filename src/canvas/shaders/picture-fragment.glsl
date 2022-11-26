#version 330
out vec4 outputColor;
in vec2 texcoord;
in vec2 border_pos;
uniform sampler2D tex;
uniform float opacity;
uniform vec3 border_color;
uniform float line_width;
uniform vec2 size;
uniform float scale;

void main() {
	vec4 c = texture(tex, texcoord);
	outputColor = vec4(c.rgb, c.a*opacity);
	if((abs(border_pos.x) > (size.x - line_width/scale)) || (abs(border_pos.y) > (size.y - line_width/scale)))
		outputColor.rgba = vec4(border_color,1);
}
