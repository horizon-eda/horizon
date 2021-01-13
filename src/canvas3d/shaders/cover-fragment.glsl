#version 330

out vec4 outputColor;
uniform vec4 layer_color; 
uniform vec3 cam_normal;

void main() {
  vec3 normal =  vec3(0,0,1);
  float shade = pow(min(1, abs(dot(cam_normal, normal))+.1), 1/2.2);
  outputColor = vec4(layer_color.rgb*shade, layer_color.a);
}
