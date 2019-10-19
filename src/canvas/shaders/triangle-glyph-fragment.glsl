#version 330
##ubo

out vec4 outputColor;
smooth in vec3 color_to_fragment;
smooth in vec2 texcoord_to_fragment;
in float lod_alpha;

uniform sampler2D msdf;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    float derivative   = length( dFdx( texcoord_to_fragment ) ) * 1024 / 4;
    vec3 sample = texture(msdf, texcoord_to_fragment).rgb;
    float dist = median(sample.r, sample.g, sample.b);

    // use the derivative for zoom-adaptive filtering
    float opacity = smoothstep( 0.5 - derivative, 0.5 + derivative, dist );
    if(opacity > 0.99)
        discard;
    outputColor = vec4(color_to_fragment, lod_alpha*clamp((1-opacity), 0, 1));
}
