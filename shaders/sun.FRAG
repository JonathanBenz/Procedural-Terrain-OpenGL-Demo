#version 330 core

in vec3 FragPos;

uniform vec3 viewPos;
uniform vec3 glowColor;
uniform vec3 fogColor;
uniform float fogDensity;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

float CalculateFogFactor(float fogDensity) 
{
    float distanceToCamera = length(FragPos.xz - viewPos.xz);
    float fogFactor = exp(-pow(fogDensity * distanceToCamera, 3.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    return fogFactor;
}

void main()
{
    float fogFactor = CalculateFogFactor(fogDensity);
    vec3 finalColor = mix(fogColor, glowColor, fogFactor);
    FragColor = vec4(finalColor, 1.0);

    // Anything above brightness threshold gets sent to the second color attachment (bright lights only)
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}