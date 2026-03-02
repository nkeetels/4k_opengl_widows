#version 430
layout (local_size_x = 64) in;
layout (std430, binding = 0) buffer B{float O[];};
uniform int sampleRate, bufferSize;

void main() {
    uint i = (gl_WorkGroupID.y * 65535 + gl_WorkGroupID.x) * 64 + gl_LocalInvocationID.x;
    if (i >= bufferSize * 2) return;
    
    float index = float(i) / 2;
    int channel = i & 1;

    O[i] = 0.5 * sin(index/sampleRate * 880.0 * (channel + 1));
}