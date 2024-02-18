#version 330
##triangle-ubo

out vec4 outputColor;
smooth in vec3 color_to_fragment;
smooth in vec2 round_pos_to_fragment;
flat in float border_width;
flat in float line_width;
flat in float a0;
flat in float a1;

vec2 p2r(float phi, float l) {
  return vec2(cos(phi), sin(phi))*l;
}

void main() {
  bool disc = false;

  if(length(round_pos_to_fragment)>1)
    disc = true;

  float my_alpha = alpha;
  if(layer_mode == LAYER_MODE_FILL_ONLY) { //force alpha for fill only mode
    my_alpha = alpha;
  }

  float phi = atan(round_pos_to_fragment.y, round_pos_to_fragment.x);
  if(phi < 0)
    phi += 2*PI;

  float my_a0 = phi-a0;
  if(my_a0 < 0)
    my_a0 += 2*PI;


  float len = length(round_pos_to_fragment);
  if(((len < (1-line_width+border_width)) || (len > (1-border_width))) && layer_mode != LAYER_MODE_FILL_ONLY) {
    my_alpha = 1;
  }
  else {
    if(get_discard(gl_FragCoord.xy)) {
      disc = true;
    }
  }

  if(len < (1-line_width)) {
    disc = true;
  }

  if(my_a0 > a1) { //outside of arc
    vec2 p0 = p2r(a0, 1-line_width/2) - round_pos_to_fragment;
    vec2 p1 = p2r(a0+a1, 1-line_width/2) - round_pos_to_fragment;
    bool e0 = length(p0) < line_width/2;
    bool e1 = length(p1) < line_width/2;
    if(e0 && !e1) {
      if(length(p0) > line_width/2-border_width) {
        if(layer_mode != LAYER_MODE_FILL_ONLY)
          my_alpha = 1;
        disc = false;
      }
    }
    else if(e1 && !e0) {
      if(length(p1) > line_width/2-border_width) {
        if(layer_mode != LAYER_MODE_FILL_ONLY)
          my_alpha = 1;
        disc = false;
      }
    }
    else if (!e0 && !e1){
      disc = true;
    }
  }

  if(disc)
    discard;

  outputColor = vec4(color_to_fragment, my_alpha);
}
