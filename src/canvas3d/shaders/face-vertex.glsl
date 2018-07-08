#version 330

in vec3 position;
in vec3 color;
in vec2 offset;
in uint flags;
in float angle;

in vec3 model_offset;
in vec3 model_rotation;

out vec3 position_to_geom;
out vec3 color_to_geom;

uniform float z_top;
uniform float z_bottom;
uniform float highlight_intensity;

mat4 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

float t(float x) {
  return x*2*3.14159;
}

void main() {
  //gl_Position = proj*view*vec4(position, 1, 1);
  bool flip = (flags&1u)!=0u;
  bool highlight = (flags&2u)!=0u;
  if(highlight)
    color_to_geom = mix(color, vec3(1,0,0), highlight_intensity);
  else
    color_to_geom = color;
  vec4 p4 = vec4(position, 1);

  mat4 rot = rotationMatrix(vec3(1,0,0), t(model_rotation.x));
  rot = rot*rotationMatrix(vec3(0,1,0), t(model_rotation.y));
  rot = rot*rotationMatrix(vec3(0,0,1), t(model_rotation.z));
  p4 = rot*p4;

  p4.xyz += model_offset;

  float angle_inv = 1;
  if(!flip) {
    angle_inv = -1;
  }
  p4 = rotationMatrix(vec3(0,0,1), angle_inv*t(angle))*p4;
  float z = 0;
  if(!flip) {
    z = z_top;
  }
  else {
    z = z_bottom;
    p4 = rotationMatrix(vec3(0,1,0), 3.14159)*p4;
   }
  position_to_geom = p4.xyz+vec3(offset, z);
}
