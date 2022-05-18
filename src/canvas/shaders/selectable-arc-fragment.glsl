#version 330
out vec4 outputColor;
in vec3 color_to_fragment;
in vec2 dot_to_fragment;
in vec2 size_to_fragment;
in vec2 pos_to_fragment;
in float r0_to_fragment;
in float r1_to_fragment;
in float a0_to_fragment;
in float dphi_to_fragment;
in float origin_size_to_fragment;
flat in uint flags_to_fragment;
##selectable-ubo

bool diamond(vec2 shift)
{
    float lc = abs(pos_to_fragment.x-shift.x)+abs(pos_to_fragment.y-shift.y);
    if(lc < origin_size_to_fragment/2) {
        outputColor = color_inner;
        return true;
    }
    else if(lc < origin_size_to_fragment) {
        outputColor = vec4(color_to_fragment, 1);
        return true;
    }
    return false;
}

vec2 p2r(float phi, float l) {
	return vec2(cos(phi), sin(phi))*l;
}

void main() {
  outputColor = vec4(color_to_fragment, 1);
  float l = length(pos_to_fragment);
  float phi = atan(pos_to_fragment.y, pos_to_fragment.x);
  bool diamond_at_midpoint = (flags_to_fragment & uint(16))!=uint(0);
  if(phi < 0)
    phi += 2*PI;
  float rs;
  if (l > (r1_to_fragment+r0_to_fragment)/2)
    rs = r1_to_fragment;
  else
    rs = r0_to_fragment;

  float c = rs*phi;
  float m = (rs*2*PI)/round(rs*2*PI/20);
  if(mod(c, m) < m/2)
    outputColor = color_inner;

  float a0_2pi = phi-a0_to_fragment;
  if(a0_2pi < 0)
    a0_2pi += 2*PI;

  float a0_pi = a0_2pi;
  if(a0_pi > PI)
    a0_pi -= 2*PI;

  float sp = 4;
  if(r0_to_fragment == r1_to_fragment)
    sp = 1;

  if(diamond_at_midpoint) {
    if(diamond(p2r(a0_to_fragment+dphi_to_fragment/2, (r0_to_fragment+r1_to_fragment)/2)))
      return;
  }

  if(l > (r1_to_fragment+sp))
    discard;
  if(l < (r0_to_fragment-sp)) {
    if(!diamond_at_midpoint && diamond(vec2(0,0)))
      return;
    else
      discard;
  }
  if(r0_to_fragment == r1_to_fragment) {
    if(a0_2pi > dphi_to_fragment)
        discard;
    return;
  }
  bool disc = l > (r0_to_fragment-2) && l < (r1_to_fragment+2);
 
  float lb1 = sin(a0_2pi)*l;
  float lb2 = sin(a0_2pi-dphi_to_fragment)*l;
  if(lb1 > -4 && lb1 < 0 && (abs(a0_pi) < PI/2)) {
    if(lb1 < -2) {
      if(mod(l, 20) < 10)
        outputColor = vec4(color_to_fragment, 1);
      else
        outputColor = color_inner;
    }
    else if(disc)
        discard;
  }
  else if(lb2 < 4 && lb2 > 0 && (abs(a0_2pi-dphi_to_fragment) < PI/2)) {
    if(lb2 > 2) {
      if(mod(l, 20) < 10)
        outputColor = vec4(color_to_fragment, 1);
      else
        outputColor = color_inner;
    }
    else if(disc)
      discard;
  }
  else if(disc)
    discard;
  else if(a0_2pi > dphi_to_fragment)
    discard;
}
