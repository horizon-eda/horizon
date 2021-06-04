layout (std140) uniform ubo
{
    vec4 color_inner;
    vec4 color_outer;
    vec4 color_always;
    vec4 color_prelight;
    mat3 screenmat;
    mat3 viewmat;
    float scale;
    float min_size;
};

#define PI 3.1415926535897932384626433832795
