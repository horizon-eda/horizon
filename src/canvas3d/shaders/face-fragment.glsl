#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out uint pick;
in vec3 color_to_fragment;
in vec3 normal_to_fragment;
in vec3 pos_to_fragment;
flat in uint instance_to_fragment;
uniform vec3 cam_normal;
uniform vec3 cam_pos;
uniform float specular_intensity;
uniform float ambient_intensity;
uniform vec3 light_pos;
uniform vec3 light_color;
uniform uint pick_base;

void main() {
  //float shade = pow(min(1, abs(dot(cam_normal, normal_to_fragment))+.1), 1/2.2);
  //outputColor = vec4(color_to_fragment*shade, 1);
  vec3 ambient = ambient_intensity * light_color;

  // See https://learnopengl.com/Lighting/Basic-Lighting
  vec3 light_dir = normalize(light_pos - pos_to_fragment);
  float diff = max(dot(normal_to_fragment, light_dir), 0.0);
  vec3 diffuse = diff * light_color;

  vec3 view_dir = normalize(cam_pos - pos_to_fragment);
  vec3 reflect_dir = reflect(-light_dir, normal_to_fragment);
  float spec = pow(max(dot(cam_normal, reflect_dir), 0.0), 32);
  vec3 specular = specular_intensity * spec * light_color;

  outputColor = vec4(color_to_fragment*(ambient + diffuse + specular), 1);
  pick = pick_base + instance_to_fragment;
}
