precision highp float;

uniform int _t;
uniform sampler2D fontTexture; 

void main() {
    vec2 uv = (gl_FragCoord.xy / vec2(960.0, 600.0)) * 2.0 - 1.0; 
    float time = float(_t) * 0.001;

    gl_FragColor = vec4(mix(vec3(0.22, 0.25, 0.28), vec3(0.05, 0.06, 0.07), pow(uv.y * 0.5 + 0.5, 0.5)) * (0.85 + 0.2 * sin(uv.y * 7.0 + sin(uv.x * 2.0 + time * 0.3) + uv.x * 0.5)), 1.0);
}