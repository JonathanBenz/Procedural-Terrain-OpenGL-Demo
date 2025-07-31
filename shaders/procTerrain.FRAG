#version 330 core

// -------------------- Structs -----------------------------
struct Material 
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

struct Light
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
// ----------------------------------------------------------

// -------------------- Variables ---------------------------
// --- Ins
in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;          
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec3 WorldViewPos;
} fs_in;

// --- Uniforms
uniform float heightScale;
uniform float snowThreshold;
uniform float fogDensity;
uniform vec3 fogColor;

uniform Material material;
uniform Light light;

uniform sampler2D rocksDiffuseMap;
uniform sampler2D snowDiffuseMap;
uniform sampler2D normalMap;
uniform sampler2D depthMap;

// --- Outs
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;
// ----------------------------------------------------------

// -------------------- Prototype Functions -----------------
float CalculateFogFactor(float fogDensity);
vec3 CalculateLight(Light light, vec3 normal, vec3 viewDir, vec2 texCoords, float fogFactor);
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir);
// ----------------------------------------------------------

void main()
{           
    // offset texture coordinates with Parallax Mapping
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec2 texCoords = fs_in.TexCoords;
    
    texCoords = ParallaxMapping(fs_in.TexCoords,  viewDir);       
    if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard;

    // obtain normal from normal map (normals will be in TBN space)
    vec3 normal = texture(normalMap, texCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);  

    // Calculate fog value and final light results
    float fogFactor = CalculateFogFactor(fogDensity);
	vec3 lightResults = CalculateLight(light, normal, viewDir, texCoords, fogFactor);

	FragColor = vec4(lightResults, 1.0f);

    // Anything above brightness threshold gets sent to the second color attachment (bright lights only)
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}

// Exonential Fog Function, compares depth to camera position to determine fog
float CalculateFogFactor(float fogDensity) 
{
    float distanceToCamera = length(fs_in.FragPos.xz - fs_in.WorldViewPos.xz);
    float fogFactor = exp(-pow(fogDensity * distanceToCamera, 3.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    return fogFactor;
}

// Blinn-Phong lighting
vec3 CalculateLight(Light light, vec3 normal, vec3 viewDir, vec2 texCoords, float fogFactor)
{
    vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir); // Halfway Vector for Blinn-Phong

	// diffuse shading
	float diff = max(dot(normal, lightDir), 0.0);

	// Determine which texture to use (rocks vs. snow) based on a height function
    float height = texture(depthMap, texCoords).r;
    float blendFactor = smoothstep(snowThreshold, 0, height);

    vec3 rockColor = texture(rocksDiffuseMap, texCoords).rgb;
    vec3 snowColor = texture(snowDiffuseMap, texCoords).rgb;
    vec3 diffuseColor = mix(rockColor, snowColor, blendFactor);

	// specular shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
	vec3 specularColor = material.specular;

	// combine results
	vec3 ambientLightTerm = light.ambient * diffuseColor;
	vec3 diffuseLightTerm = light.diffuse * diff * diffuseColor;
	vec3 specularLightTerm = light.specular * spec * specularColor;

    // Incorporate fog into the final light results
    vec3 ambient = fogFactor * ambientLightTerm;
    vec3 diffuse = fogFactor * diffuseLightTerm;
    vec3 specular = fogFactor * specularLightTerm;

    vec3 lighting = ambient + diffuse + specular;

    // Mix in fog with the final lighting
    vec3 finalColor = mix(fogColor, lighting * diffuseColor, fogFactor);

	return finalColor;
}

// Occlusion Parallax Mapping (Taken and modified from https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/5.advanced_lighting/5.3.parallax_occlusion_mapping/5.3.parallax_mapping.fs)
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    // have number of depth layers dynamically change with the viewing angle:
    //  - more layers needed with a parallel viewing angle, and less layers needed with a perpendicular viewing angle
    const float minLayers = 8;
    const float maxLayers = 1024;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir))); 
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;

    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightScale; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(depthMap, currentTexCoords).r;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;

        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(depthMap, currentTexCoords).r;  

        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // Mitigate artifact effects:
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(depthMap, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}