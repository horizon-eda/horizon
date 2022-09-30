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
uniform float specular_power;
uniform float ambient_intensity;
uniform float diffuse_intensity;
uniform vec3 light_pos;
uniform vec3 light_color;
uniform uint pick_base;

void main() {
  // See https://learnopengl.com/Lighting/Basic-Lighting
  vec3 ambient = ambient_intensity * light_color;
  vec3 light_dir = normalize(light_pos - pos_to_fragment);
  float diff = max(dot(normal_to_fragment, light_dir), 0.0);
  vec3 diffuse = diffuse_intensity * diff * light_color;

  vec3 view_dir = normalize(cam_pos - pos_to_fragment);
  vec3 halfway_dir = normalize(light_dir + view_dir);
  float spec = pow(max(dot(cam_normal, halfway_dir), 0.0), specular_power);
  vec3 specular = specular_intensity * spec * light_color;

  vec3 color = color_to_fragment*(ambient + diffuse + specular);

  // Gamma correction
  color = pow(color, vec3(1.0/1.5));

  outputColor = vec4(color, 1);
  pick = pick_base + instance_to_fragment;
}
