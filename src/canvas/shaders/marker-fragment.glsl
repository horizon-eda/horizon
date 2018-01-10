#version 330
uniform float scale;
out vec4 outputColor;

flat in vec3 color_to_fragment;
smooth in vec2 draw_pos_to_fragment;

void main() {
  bool disc = false;
  vec2 p = draw_pos_to_fragment;
  vec4 c = vec4(1,1,1,1);
  float k1 = 6;
  float k2 = 2;
  float k3 = 14;
  if(p.x+p.y>k1) {
    disc=true;
  }
  if(abs(p.x-p.y)<k2) {
    disc=false;
  }
  if(p.x+p.y>k3) {
    disc=true;
  }
  
  float border = .5;
  if(p.x+p.y<(k1-border*1.41) && p.x > border && p.y>border) {
    c = vec4(color_to_fragment, .7);
  }
  if(abs(p.x-p.y)<(k2-border*1.41) && p.x+p.y>(k1-2*border) && p.x+p.y<(k3-border*1.41)) {
    c = vec4(color_to_fragment, .7);
  }

  if(disc)
    discard;
  outputColor = c;
}
