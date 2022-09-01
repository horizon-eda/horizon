#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out int pick;

in vec3 pos_to_fragment;
uniform vec4 layer_color; 
uniform vec3 cam_normal;
uniform vec3 cam_pos;
uniform float specular_intensity;
uniform float specular_power;
uniform float ambient_intensity;
uniform float diffuse_intensity;
uniform vec3 light_pos;
uniform vec3 light_color;

void main() {
  vec3 normal =  vec3(0,0,1);
  //float shade = pow(min(1, abs(dot(cam_normal, normal))+.1), 1/2.2);
  // outputColor = vec4(layer_color.rgb*shade, layer_color.a);

  // See https://learnopengl.com/Lighting/Basic-Lighting
  vec3 ambient = ambient_intensity * light_color;
  vec3 light_dir = normalize(light_pos - pos_to_fragment);
  float diff = max(dot(normal, light_dir), 0.0);
  vec3 diffuse = diffuse_intensity * diff * light_color;

  vec3 view_dir = normalize(cam_pos - pos_to_fragment);
  vec3 reflect_dir = reflect(-light_dir, normal);
  float spec = pow(max(dot(cam_normal, reflect_dir), 0.0), specular_power);
  vec3 specular = specular_intensity * spec * light_color;


  outputColor = vec4(layer_color.rgb*(ambient + diffuse + specular), layer_color.a);
  pick = 1;
}
