#version 330 core

// Define constants
#define M_PI 3.141593

// Specify the input locations of attributes
layout (location = 0) in vec3 vertCoordinates_in;
layout (location = 1) in vec3 vertNormals_in;

// Specify the Uniforms of the vertex shader
uniform mat4 modelViewTransform;
uniform mat4 projectionTransform;
uniform mat3 normalTransform;

// Wave properties
uniform float amplitude[6];
uniform float phase[6];
uniform float frequency[6];
uniform int numwaves;
uniform float t;

// Specify the output of the vertex stage
out vec3 vertNormal;
out vec2 uv;
out vec3 vertCoords;

float waveHeight(int waveIdx, float uvalue)
{
    return amplitude[waveIdx] * sin(frequency[waveIdx]*M_PI*uv.x + phase[waveIdx] + t);
}

float waveDU(int waveIdx, float uvalue)
{
    return frequency[waveIdx] * amplitude[waveIdx] * M_PI * cos(frequency[waveIdx]*M_PI*uv.x + phase[waveIdx] + t);
}

void main()
{

    vertCoords = vertCoordinates_in;
    uv = vec2(vertCoords);
    float derSum = 0;

    for(int i = 0; i < numwaves; i++)
    {
        vertCoords.z = vertCoords.z + waveHeight(i,uv.x);
        derSum = derSum + waveDU(i, uv.x);
    }

    gl_Position = projectionTransform * modelViewTransform * vec4(vertCoords, 1.0);
    vertNormal = normalize(vec3(-derSum, 0, 1.0));

}

