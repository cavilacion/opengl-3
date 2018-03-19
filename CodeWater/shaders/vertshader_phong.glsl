#version 330 core

// Define constants
#define M_PI 3.141593

// Specify the input locations of attributes
layout (location = 0) in vec3 vertCoordinates_in;
layout (location = 1) in vec3 vertNormals_in;

// Specify the Uniforms of the vertex shader
uniform mat4 modelViewTransform;
uniform mat4 projectionTransform;
uniform vec3 lightPosition;
uniform mat3 normalTransform;

// Wave properties
uniform float amplitude[6];
uniform float phase[6];
uniform float frequency[6];
uniform int numwaves;
uniform float t;

// Specify the output of the vertex stage
out vec3 vertNormal;
out vec3 vertPosition;
out vec3 relativeLightPosition;
out vec2 uv;
out float h;

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
    vertPosition = vertCoordinates_in;
    uv = vec2(vertPosition);

    float z = 0; // z value to be added
    float derSum = 0;
    float A = 0; // total amplitude

    for(int i = 0; i < numwaves; i++)
    {
        A = A + amplitude[i];
        z = z + waveHeight(i,uv.x);
        derSum = derSum + waveDU(i, uv.x);
    }
    vertPosition.z = vertPosition.z + z;
    h = (z/A+1.0)/2.0; // map height to [0,1]
    gl_Position = projectionTransform * modelViewTransform * vec4(vertPosition, 1.0);


    // Pass the required information to the fragment stage.
    relativeLightPosition = vec3(modelViewTransform * vec4(lightPosition, 1));
    vertPosition = vec3(modelViewTransform * vec4(vertPosition, 1));
    vertNormal   = normalTransform * normalize(vec3(-derSum, 0, 1.0));
}
