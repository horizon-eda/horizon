#version 330

in vec3 position;
in vec3 color;
in vec2 offset;
in int flip;
in float angle;
out vec3 position_to_geom;
out vec3 color_to_geom;

uniform float z_top;
uniform float z_bottom;

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

void main() {
  //gl_Position = proj*view*vec4(position, 1, 1);
  color_to_geom = color;
  vec4 p4 = vec4(position, 1);
  float angle_inv = 1;
  if(flip==0) {
    angle_inv = -1;
  }
  p4 = rotationMatrix(vec3(0,0,1), angle_inv*angle*2*3.14159)*p4;
  float z = 0;
  if(flip==0) {
    z = z_top;
  }
  else {
    z = z_bottom;
    p4 = rotationMatrix(vec3(0,1,0), 3.14159)*p4;
   }
  position_to_geom = p4.xyz+vec3(offset, angle/1e6)+vec3(0,0,z);
}
