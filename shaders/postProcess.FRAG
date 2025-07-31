#version 330 core

// --- Ins
in vec2 TexCoords;

// --- Uniforms
uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform sampler2D downSampleBrightPass;
uniform sampler2D colorGradient;
uniform sampler2D lensDirt;
uniform sampler2D starBurst;
uniform float exposure;

// lens flare uniforms
uniform float haloRadius = 0.25;
uniform float chromaticAberration = 7.5;
uniform float starburstOffset;
uniform float aspectRatio; 
uniform int ghosts = 6;

// --- Outs
out vec4 FragColor;

// --- Prototype Functions
vec3 ApplyThreshold(vec3 rgb, float threshold);
vec3 textureDistorted(sampler2D tex, vec2 texcoord, vec2 distortionDirection, vec3 distortionFactor);
vec3 CalculateLensFlare();
// ----------------------------------------------------------------------------------------------------------------

void main()
{             
    // Grab the HDR scene
    vec3 hdrColor = texture(scene, TexCoords).rgb;

    // Add bloom on top
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    hdrColor += bloomColor;

    // Create lens flare features, blur it and add lens dirt
    vec3 lensFlare = CalculateLensFlare();
    lensFlare *= bloomColor;
    lensFlare *= texture(lensDirt, TexCoords).rgb;

    // Add starburst effect to the lens flare
    vec2 centerVec = TexCoords - vec2(0.5);
    float d = length(centerVec);
    float radial = acos(centerVec.x / d);
    float mask = texture(starBurst, vec2(radial + starburstOffset * 1.0, 0.0)).r * texture(starBurst, vec2(radial - starburstOffset * 0.5, 0.0)).r;
    mask = clamp(mask + (1.0 - smoothstep(0.0, 0.3, d)), 0.0, 1.0); // fade the starburst towwards the center
    lensFlare *= mask;

    // Add lens flare on top
    hdrColor += lensFlare * 8; // Multiply to make the effect stronger

    // exposure tonemapping
    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    FragColor = vec4(result, 1.0);
}

// --- Functions
vec3 ApplyThreshold(vec3 rgb, float threshold) // Used to ignore values that don't meet the threshold
{
	return max(rgb - vec3(threshold), vec3(0.0));
}

vec3 textureDistorted(sampler2D tex, vec2 texcoord, vec2 distortionDirection, vec3 distortionFactor) // Used to apply chromatic abberation to a texture
{
   return vec3
   (
      texture(tex, texcoord + distortionDirection * distortionFactor.r).r,
      texture(tex, texcoord + distortionDirection * distortionFactor.g).g,
      texture(tex, texcoord + distortionDirection * distortionFactor.b).b
   );
}

vec3 CalculateLensFlare() // Calculates ghosts and halo for lens flare effect. Inspired by this article: https://john-chapman.github.io/2017/11/05/pseudo-lens-flare.html
{
    vec3 lensFlare = vec3(0.0);

    // Chromatic Abberation vars
    vec2 texelSize = 1.0 / vec2(textureSize(downSampleBrightPass, 0));
    vec3 distortionFactor = vec3(-texelSize.x * chromaticAberration, 0.0, texelSize.x * chromaticAberration);
    vec2 distortionDirection = vec2(0.5) - TexCoords;

    // GHOSTS
    vec2 uv = TexCoords;
    vec2 ghostVec = (vec2(0.5) - uv) * 2;
    for (int i = 1; i <= ghosts; ++i) 
    {
        vec2 suv = uv + ghostVec * vec2(i);
        float d = distance(suv, vec2(0.5));
        float weight = 1.0 - smoothstep(0.0, 0.75, d);
        vec3 s = textureDistorted(downSampleBrightPass, suv, distortionDirection, distortionFactor).rgb;
        s = ApplyThreshold(s, 1.0);
        lensFlare += s * weight;
        lensFlare *= textureDistorted(colorGradient, suv, distortionDirection, distortionFactor).rgb; 
    }

    // HALO
    vec2 haloVec = vec2(0.5) - uv;
    haloVec.x /= aspectRatio;
    haloVec = normalize(haloVec);
    haloVec.x *= aspectRatio;
    vec2 wuv = (uv - vec2(0.5, 0.0)) / vec2(aspectRatio, 1.0) + vec2(0.5, 0.0);
    float d = distance(wuv, vec2(0.5));
    haloVec *= haloRadius;
    float haloWeight = length(vec2(0.5) - fract(TexCoords + haloVec)) / length(vec2(0.5));
    haloWeight = pow(1.0 - haloWeight, 5.0);

    vec3 s = textureDistorted(downSampleBrightPass, TexCoords + haloVec, distortionDirection, distortionFactor).rgb;
    s = ApplyThreshold(s, 1.0);
    lensFlare += s * haloWeight;

    return lensFlare;
}