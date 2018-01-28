#version 330
out vec4 outputColor;
in vec3 color_to_fragment;
in vec2 dot_to_fragment;
in vec2 size_to_fragment;

void main() {
  float border = 3;
  if(size_to_fragment.x > 0) {
    bool x_inside = ((dot_to_fragment.x > border) && (dot_to_fragment.x < (size_to_fragment.x-border)));
    bool y_inside = ((dot_to_fragment.y > border) && (dot_to_fragment.y < (size_to_fragment.y-border)));
    if(x_inside && y_inside)
      discard;

    outputColor = vec4(color_to_fragment ,1); 
    if((!x_inside && mod(dot_to_fragment.y, 20) > 10) || (!y_inside && mod(dot_to_fragment.x, 20) > 10)) {
        outputColor = vec4(0,0,0 ,1);
    }

  }
  else {
    outputColor = vec4(color_to_fragment ,1);
  }
}
