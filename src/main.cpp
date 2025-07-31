/* Author: Jonathan Benz
 * Date: 07/31/2025
 * Description: My attempt at the Acerola 2025 Dirtjam!
 * Features: 
 *		- Heightmap generation done with random Simplex noise and fractional brownian motion.
 *		- Normal map generation done by calculating the gradients of each point of the heightmap.
 *		- Parallax Occlusion Mapping to give the illusion of a flat quad looking like it has depth.
 *		- Blinn-Phong Lighting Model. 
 *		- Height function which determines when to use the Snow Texture --> used for texturing snowy mountain peaks. 
 *		- Fog effect when camera is far away from the scene. 
 *		- Skybox cubemap. 
 *		- Gaussian Blurring using a downsampled framebuffer object for increased performance. 
 *		- Post-Processing Effects such as HDR & Tonemapping, Bloom, and Screen-Space Lens-Flares. 
 * 
 * You can checkout my other projects at https://github.com/JonathanBenz, or visit my website at https://sites.google.com/view/jonathan-benz! 
 */

#include <iostream>
#include <vector>
#include <glad/glad.h>    // For getting OpenGL function pointers from drivers
#include <GLFW/glfw3.h>   // Windowing and User Input
#include <glm/glm.hpp>    // Matrix Math
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include "SimplexNoise.h" // Sébastien Rombauts' SimplexNoise implementation: https://github.com/SRombauts/SimplexNoise
#include "shader.h"       // Helper Class for binding shaders and updating Uniforms
#include "texture.h"      // Helper Class for loading textures and creating textures
#include "mesh.h"

// ------------------------------------ Prototype Functions -------------------------------------------------------
int Init();
void ProcessInput(GLFWwindow* window);
float FBM(float x, float y, int octaves, float lacunarity, float persistence);
std::vector<unsigned char> GenerateHeightMap(int textureSize, float scale = 0.005f, int octaves = 6, float persistence = 0.5f, float lacunarity = 2.0f);
std::vector<glm::vec3> GenerateNormalMap(const std::vector<unsigned char>& heightMap, int textureSize);
float EaseInOutSine(float x);
void RenderPostProcessQuad();
void RenderProceduralTerrain(Shader& proceduralTerrain, glm::mat4& projection, glm::mat4& view, Texture& diffuseMapTextureRocks, Texture& diffuseMapTextureSnow, Texture& normalMapTexture, Texture& heightMapTexture, Mesh& terrainMesh);
void RenderSun(Shader& sunShader, glm::mat4& projection, glm::mat4& view, glm::mat4& model, Mesh& sun);
void RenderSkybox(Shader& skyboxShader, glm::mat4& view, glm::mat4& projection, Mesh& skybox, Texture& skyboxTexture);
void AnimateSun(Shader& sunShader);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// ----------------------------------------------------------------------------------------------------------------

// ------------------------------------ Global Variables ----------------------------------------------------------
// --- Screen Settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
GLFWwindow* window = nullptr;

// --- Camera Settings
glm::vec3 cameraPos = glm::vec3(-3.08f, 3.07f, 3.26f);
glm::vec3 cameraFront = glm::vec3(0.51f, -0.66f, -0.55f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
bool firstMouse = true;                                    // To prevent huge jump on screen when first starting program
float pitch = -45.0f;                                      // Up-Down camera rotation (around the X-axis)
float yaw = -45.0f;                                        // Left-Right camera rotation (around the Y-axis). Init to face forward. 
float lastX = SCR_WIDTH / 2.0f, lastY = SCR_HEIGHT / 2.0f; // Init to middle of the screen
float FOV = 50.0f;                                         // Default FOV value
float cameraSpeed = 2.5f;
float cameraSensitivity = 0.1f;

// --- Delta Time 
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float timer = 0.0f;

// --- Terrain Params
const float scaleAmt = 2.0f; // scale in the xy-direction
const float heightScale = 0.15f;
const float snowThreshold = 0.69f;
const float fogDensity = 0.1f;
glm::vec3 fogColor(0.8f, 0.8f, 0.8f);

// --- Lighting Params
const float gaussianBlurIntensity = 100.0f;
const float bloomFactor = 5.0f;
const glm::vec3 WHITE = glm::vec3(1.0f, 0.85f, 0.55f) * bloomFactor;
const glm::vec3 ORANGE = glm::vec3(1.0f, 0.5f, 0.05f) * bloomFactor;
glm::vec3 lightPos(0.0f, 0.1f, 0.0f);
glm::vec3 lightAmbience(0.1f, 0.05f, 0.35f);
glm::vec3 lightDiffuse = WHITE;
glm::vec3 lightSpecular = glm::vec3(0.9f, 0.7f, 0.4f) * bloomFactor;
const float maxExposure = 0.75f;
const float minExposure = 0.05f;
float exposure = maxExposure;

// --- Sun Params
const float sunRadius = 1.7f; // Radius of orbit from center
float sunDesiredSpeed = 0.4f; // The speed we want to lerp to
float sunAngle = 999.99f;     // Default init value
float sunVel = 0.0f;          // Sun's speed + direction
float idleTime = 1.0f;
const bool bIsSunStationary = false;
bool bReverseSun = false;
bool bWaiting = false;
// ----------------------------------------------------------------------------------------------------------------

int main()
{
	// Initialize GLFW window and GLAD function pointers. Exit out of program early and terminate if -1 is returned
	if (Init() == -1) return -1;
	
	// --------------------------------- TEXTURES -----------------------------------------------------------------
	// Generate a heightmap
	int textureSize = 512;
	std::vector<unsigned char> simplexHeightMap = GenerateHeightMap(textureSize);
	Texture heightMapTexture(simplexHeightMap, textureSize);

	// Generate a normal map
	std::vector<glm::vec3> normalMap = GenerateNormalMap(simplexHeightMap, textureSize);
	Texture normalMapTexture(normalMap, textureSize);

	// Load diffuse textures
	Texture diffuseMapTextureRocks("textures/aerial_rocks/aerial_rocks_04_diff_8k.jpg");
	Texture diffuseMapTextureSnow("textures/snow/snow_field_aerial_diff_8k.jpg");

	// Load cubemap skybox texture
	std::vector<std::string> faces
	{
		std::string("textures/skybox/right.png"),
		std::string("textures/skybox/left.png"),
		std::string("textures/skybox/up.png"),
		std::string("textures/skybox/down.png"),
		std::string("textures/skybox/front.png"),
		std::string("textures/skybox/back.png")
	};
	Texture skyboxTexture(faces);
	
	// Load lens flare textures
	Texture colorGradientTex("textures/lens_flare/colorGradient.png", true);
	Texture lensDirtTex("textures/lens_flare/lensDirt.png", true);
	Texture starBurstTex("textures/lens_flare/starBurst.png");
	// -----------------------------------------------------------------------------------------------------------

	// --------------------------------- SHADERS -----------------------------------------------------------------
	// --- Build and compile shaders
	Shader proceduralTerrain("shaders/procTerrain.VERT", "shaders/procTerrain.FRAG");
	Shader skyboxShader("shaders/skybox.VERT", "shaders/skybox.FRAG");
	Shader sunShader("shaders/sun.VERT", "shaders/sun.FRAG");
	Shader downSampleShader("shaders/downSample.VERT", "shaders/downSample.FRAG");
	Shader blurShader("shaders/gaussianBlur.VERT", "shaders/gaussianBlur.FRAG");
	Shader postProcessShader("shaders/postProcess.VERT", "shaders/postProcess.FRAG");
	// -----------------------------------------------------------------------------------------------------------
	
	// ----------------------------- BUFFERS, MESH CREATION ------------------------------------------------------
	// --- Generate VBOs and VAOs for terrain, the skybox, and the spherical sun
	Mesh terrainMesh(proceduralTerrain);
	Mesh skybox(skyboxShader, MeshType::Skybox);
	Mesh sun(sunShader, MeshType::Sun);

	// --- Setup HDR framebuffer
	unsigned int hdrFBO;
	glGenFramebuffers(1, &hdrFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
	unsigned int colorBuffer[2];
	glGenTextures(2, colorBuffer);
	for (unsigned int i = 0; i < 2; i++) // Two color attachments, one for normal rendering, the other for our lights (needed for bloom)
	{
		glBindTexture(GL_TEXTURE_2D, colorBuffer[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL); // 16-bit floating point precision
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffer[i], 0);
	}
	unsigned int rboDepth; // create depth buffer (renderbuffer)
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth); // attach the texture and render buffers together
	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments); 
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	postProcessShader.use();
	postProcessShader.setInt("scene", 0);
	postProcessShader.setInt("bloomBlur", 1);
	postProcessShader.setInt("downSampleBrightPass", 2);
	postProcessShader.setInt("colorGradient", 3);
	postProcessShader.setInt("lensDirt", 4);
	postProcessShader.setInt("starBurst", 5);

	// FBO for two-pass gaussian blur (alternating between horizontal and vertical blurring)
	unsigned int pingpongFBO[2];
	unsigned int pingpongColorbuffers[2];
	glGenFramebuffers(2, pingpongFBO);
	glGenTextures(2, pingpongColorbuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
		glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
		// also check if framebuffers are complete (no need for depth buffer)
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
	}
	blurShader.use();
	blurShader.setInt("image", 0);

	// Downsampled FBO for bloom and lens flare
	const int downSampleFactor = 4;
	unsigned int downSampledFBO, downSampledTex;
	glGenFramebuffers(1, &downSampledFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, downSampledFBO);
	glGenTextures(1, &downSampledTex);
	glBindTexture(GL_TEXTURE_2D, downSampledTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH / downSampleFactor, SCR_HEIGHT / downSampleFactor, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, downSampledTex, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Flare framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	downSampleShader.use();
	downSampleShader.setInt("sourceTex", 0);
	// -----------------------------------------------------------------------------------------------------------

	// --- Init one-time Uniform values 
	// Set terrain
	proceduralTerrain.use();
	proceduralTerrain.setInt("rocksDiffuseMap", 0);
	proceduralTerrain.setInt("snowDiffuseMap", 1);
	proceduralTerrain.setInt("normalMap", 2);
	proceduralTerrain.setInt("depthMap", 3);
	proceduralTerrain.setFloat("heightScale", heightScale);
	proceduralTerrain.setFloat("snowThreshold", snowThreshold);
	proceduralTerrain.setFloat("fogDensity", fogDensity);
	proceduralTerrain.setVec3("fogColor", fogColor);
	// Rotate terrain to lay flat on the XY-plane
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(scaleAmt, scaleAmt, scaleAmt));
	proceduralTerrain.setMat4("model", model);
	// Set lighting 
	proceduralTerrain.setVec3("light.ambient", lightAmbience);
	proceduralTerrain.setVec3("light.diffuse", lightDiffuse);
	proceduralTerrain.setVec3("light.specular", lightSpecular);

	// Set skybox
	skyboxShader.use();
	skyboxShader.setInt("skybox", 0);

	// Set sun
	sunShader.use();
	sunShader.setVec3("glowColor", lightDiffuse);
	sunShader.setFloat("fogDensity", fogDensity);
	sunShader.setVec3("fogColor", fogColor);
	// -----------------------------------------------------------------------------------------------------------

	// Enable depth testing, MSAA, and gamma correction
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_FRAMEBUFFER_SRGB);

	// -------------------------------- MAIN RENDER LOOP ---------------------------------------------------------
	while (!glfwWindowShouldClose(window))
	{
		// Calculate delta time
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Get user input
		ProcessInput(window);

		// ------------------------------ Render stuff here... ---------------------------------------------------
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Render the regular scene into the hdr floating point framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glm::mat4 projection = glm::perspective(glm::radians(FOV), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
			glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
			RenderProceduralTerrain(proceduralTerrain, projection, view, diffuseMapTextureRocks, diffuseMapTextureSnow, normalMapTexture, heightMapTexture, terrainMesh);
			RenderSun(sunShader, projection, view, model, sun);
			RenderSkybox(skyboxShader, view, projection, skybox, skyboxTexture);
			AnimateSun(sunShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Downsample the bright pass to a smaller FBO for better performance
		glViewport(0, 0, SCR_WIDTH / downSampleFactor, SCR_HEIGHT / downSampleFactor);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindFramebuffer(GL_FRAMEBUFFER, downSampledFBO);
			downSampleShader.use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, colorBuffer[1]); 
			RenderPostProcessQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Perform bloom blurring on bright lights with a two-pass Gaussian Blur, using the downsampled texture
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		bool horizontal = true, first_iteration = true;
		unsigned int amount = gaussianBlurIntensity;
		blurShader.use();
		for (unsigned int i = 0; i < amount; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
			blurShader.setInt("horizontal", horizontal);
			glBindTexture(GL_TEXTURE_2D, first_iteration ? downSampledTex : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
			RenderPostProcessQuad();
			horizontal = !horizontal;
			if (first_iteration)
				first_iteration = false;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Render the floating point hdr color buffer to a 2D quad and tonemap the HDR colors in addition to other post-process effects (e.g., lens flare)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		postProcessShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorBuffer[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, downSampledTex);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, colorGradientTex._textureID);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, lensDirtTex._textureID);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, starBurstTex._textureID);
		postProcessShader.setFloat("exposure", exposure);
		postProcessShader.setFloat("starburstOffset", glfwGetTime() * deltaTime);
		postProcessShader.setFloat("aspectRatio", SCR_WIDTH / SCR_HEIGHT);
		RenderPostProcessQuad();
		// ----------------------------- Rendering Complete ------------------------------------------------------
		
		// Check and call events/ callback functions, then swap the buffer
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwTerminate();
}

// ------------------------------------ Functions ----------------------------------------------------------------
int Init()
{
	// Initialize GLFW, tell it we are using OpenGL 4.3 Core
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4); // Enable MSAA

	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Jonathan Benz Acerola Dirtjam", NULL, NULL);

	// Handle Errors if window fails to be created
	if (window == NULL)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Make sure GLAD is initialized so that it can manage OpenGL Function Pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// Initialize the viewport
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	// Set a callback function for resizing the window
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
}

/// <summary>
/// Poll for any of the specified inputs. Mainly used for exiting the program with Escape, controlling the camera with WASD, and to move faster with Left Shift. 
/// Used with GLFW. 
/// </summary>
void ProcessInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) // ESC
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // UP
		cameraPos += cameraFront * cameraSpeed * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // LEFT
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // DOWN
		cameraPos -= cameraFront * cameraSpeed * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // RIGHT
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) // LEFT SHIFT
		cameraSpeed = 10.0f;
	else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
		cameraSpeed = 2.5f;
}

/// <summary>
/// Fractional Brownian Motion function. Used to make terrain look less terrible by layering octaves of noise values on top of each other. 
/// </summary>
/// <param name="x"> Heightmap X Value. </param>
/// <param name="y"> Heightmap Y Value. </param>
/// <param name="octaves"> Think of this like the layers of an onion bro. </param>
/// <param name="lacunarity"> Controls increase in frequency between the octaves. </param>
/// <param name="persistence"> Controls decrease in amplitude between the octaves. </param>
float FBM(float x, float y, int octaves, float lacunarity, float persistence)
{
	float total = 0.0f;
	float amplitude = 1.0f;
	float frequency = 1.0f;
	float maxValue = 0.0f;

	for (int i = 0; i < octaves; ++i)
	{
		total += SimplexNoise::noise(x * frequency, y * frequency) * amplitude;
		maxValue += amplitude;

		amplitude *= persistence;
		frequency *= lacunarity;
	}
	float fbmValue = total / maxValue; // -1 to 1 range

	return (fbmValue + 1.0f) * 0.5f;  // 0 to 1 range
}

/// <summary>
/// Generate a flattened 1D heightmap with 1-byte accuracy. 
/// </summary>
/// <param name="textureSize"> The size of one side of a quad texture. E.g., 512 for a 512x512 texture. </param>
/// <param name="scale"> Amount to scale the noise values by. Scaling down (e.g., using fractional values) will yield smoother results. </param>
std::vector<unsigned char> GenerateHeightMap(int textureSize, float scale, int octaves, float persistence, float lacunarity)
{
	std::vector<unsigned char> heightMap(textureSize * textureSize);

	for (int y = 0; y < textureSize; ++y)
	{
		for (int x = 0; x < textureSize; ++x)
		{
			// Divide int values to small fractional values for better noise results
			float dx = x * scale;
			float dy = y * scale;
			float noiseValue = FBM(dx, dy, octaves, lacunarity, persistence); // Apply fractal brownian motion 
			heightMap[(y * textureSize) + x] = static_cast<unsigned char>(noiseValue * 255);
		}
	}
	return heightMap;
}

/// <summary>
/// Given a height map of 1-byte accuracy, generate a normal map by calculating the partial derivatives at each point of the height map.
/// Math from this StackOverflow post helped me: https://stackoverflow.com/questions/5281261/generating-a-normal-map-from-a-height-map.
/// </summary>
std::vector<glm::vec3> GenerateNormalMap(const std::vector<unsigned char>& heightMap, int textureSize)
{
	std::vector<glm::vec3> normalMap(heightMap.size());

	for (int y = 1; y < textureSize - 1; ++y)
	{
		for (int x = 1; x < textureSize - 1; ++x)
		{
			float heightLeft = heightMap[y * textureSize + (x - 1)] / 255.0f;
			float heightRight = heightMap[y * textureSize + (x + 1)] / 255.0f;
			float heightDown = heightMap[(y + 1) * textureSize + x] / 255.0f;
			float heightUp = heightMap[(y - 1) * textureSize + x] / 255.0f;

			float dx = heightLeft - heightRight;
			float dy = heightUp - heightDown;

			glm::vec3 normal = glm::normalize(glm::vec3(dx, dy, 1.0f));
			normalMap[(y * textureSize) + x] = normal * 0.5f + 0.5f; // normalize between 0-1
		}
	}
	return normalMap;
}

/// <summary>
/// Ease Out function used for linearly interpolating --> glm::mix(a, b, easeInOutSine(x)). 
/// </summary>
/// <param name="x"> Usually will be set to deltaTime. </param>
float EaseInOutSine(float x) // This website has good references for different easing functions: https://easings.net/#easeInOutSine
{
	return -(glm::cos(glm::pi<float>() * x) - 1) / 2;
}

/// <summary>
/// Render a basic screen-wide quad, useful function for doing a final post-process render pass.
/// </summary>
unsigned int postProcessVAO = 0;
unsigned int postProcessVBO;
void RenderPostProcessQuad()
{
	if (postProcessVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &postProcessVAO);
		glGenBuffers(1, &postProcessVBO);
		glBindVertexArray(postProcessVAO);
		glBindBuffer(GL_ARRAY_BUFFER, postProcessVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(postProcessVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

/// <summary>
/// Renders the parallax mapped procedural terrain quad with the appropriate required textures. 
/// </summary>
void RenderProceduralTerrain(Shader& proceduralTerrain, glm::mat4& projection, glm::mat4& view, Texture& diffuseMapTextureRocks, Texture& diffuseMapTextureSnow, Texture& normalMapTexture, Texture& heightMapTexture, Mesh& terrainMesh)
{
	proceduralTerrain.use();
	proceduralTerrain.setMat4("projection", projection);
	proceduralTerrain.setMat4("view", view);
	proceduralTerrain.setVec3("viewPos", cameraPos);
	proceduralTerrain.setVec3("lightPos", lightPos);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuseMapTextureRocks._textureID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, diffuseMapTextureSnow._textureID);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, normalMapTexture._textureID);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, heightMapTexture._textureID);
	terrainMesh.DrawQuad();
}

/// <summary>
/// Renders the sun as a sphere. 
/// </summary>
void RenderSun(Shader& sunShader, glm::mat4& projection, glm::mat4& view, glm::mat4& model, Mesh& sun)
{
	sunShader.use();
	sunShader.setMat4("projection", projection);
	sunShader.setMat4("view", view);
	model = glm::mat4(1.0f);
	model = glm::translate(model, lightPos);
	model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
	sunShader.setMat4("model", model);
	sunShader.setVec3("viewPos", cameraPos);
	sun.DrawSphere();
}

/// <summary>
/// Renders a cubemapped skybox. 
/// </summary>
void RenderSkybox(Shader& skyboxShader, glm::mat4& view, glm::mat4& projection, Mesh& skybox, Texture& skyboxTexture)
{
	glDepthFunc(GL_LEQUAL);
	skyboxShader.use();
	view = glm::mat4(glm::mat3(glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp))); // Needed to make skybox appear to extend infinitely
	skyboxShader.setMat4("projection", projection);
	skyboxShader.setMat4("view", view);
	glBindVertexArray(skybox.VAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture._textureID);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
}

/// <summary>
/// Checks if Sun should be stationary or animated. 
/// If animated, the sun travels in a semicircle trajectory from one side of the horizon of the terrain to the the other side. 
/// As the sun approaches either sides of the horizon, it decelerates, its color changes to an orange hue, and the brightness of the scene also dims. 
/// As the sun approaches the zenith, it accelerates, its color changes to a white hue, and the brightness of the scene increases.
/// When the sun hits the horizon, it waits in place for the specified idleTime until it can reverse its direction and start moving again. 
/// </summary>
void AnimateSun(Shader& sunShader)
{
	if (bIsSunStationary)
		lightPos = glm::vec3(1.0f, 0.75f, 1.0f);

	// Update position of the sun to orbit 
	else 
	{
		// Sun waits for a bit before reversing direction and moving again. 
		if (bWaiting)
		{
			timer += deltaTime;
			if (timer >= idleTime)
			{
				bWaiting = false;
				timer = 0.0f;
			}
		}

		// Sun is in motion. 
		else
		{
			sunAngle = glm::degrees(glm::acos(glm::dot(glm::normalize(lightPos), glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)))));
			float dir = bReverseSun ? -1.0f : 1.0f;
			float targetSpeed = sunDesiredSpeed * dir;

			// If sun is approaching the horizon
			if ((sunAngle < 89.9f && !bReverseSun) || (sunAngle > 90.1f && bReverseSun))
			{
				sunVel += glm::mix(deltaTime * sunDesiredSpeed * dir, 0.0f, EaseInOutSine(deltaTime) * 120);	  // Slow down
				lightDiffuse = glm::mix(lightDiffuse, ORANGE, EaseInOutSine(deltaTime) * 10);					  // Appear orange
				exposure = glm::mix(exposure, minExposure, EaseInOutSine(deltaTime) * 10);						  // Make scene dimmer
			}

			// If sun is approaching the zenith
			else
			{
				sunVel += glm::mix(deltaTime * sunDesiredSpeed * dir, targetSpeed, EaseInOutSine(deltaTime) * 3); // Speed up
				lightDiffuse = glm::mix(lightDiffuse, WHITE, EaseInOutSine(deltaTime) * 10);					  // Appear white
				exposure = glm::mix(exposure, maxExposure, EaseInOutSine(deltaTime) * 10);						  // Make scene brighter
			}

			// Update the Sun's color to the shader. 
			sunShader.use();
			sunShader.setVec3("glowColor", lightDiffuse);

			// Travel diagonally across the terrain in a semicircle
			lightPos.x = sunRadius * -cos(sunVel);
			lightPos.z = sunRadius * -cos(sunVel);
			lightPos.y = sunRadius * sin(sunVel);
		}

		// Check if sun is at either horizon
		if (sunAngle < 5.0f && !bReverseSun)
		{
			bWaiting = true;
			bReverseSun = true;
		}
		else if (sunAngle > 175.0f && bReverseSun)
		{
			bWaiting = true;
			bReverseSun = false;
		}
	}
}

/// <summary>
/// GLFW callback function for checking if the window needs to resize. 
/// </summary>
void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }

/// <summary>
/// GLFW callback function for mouse input.
/// </summary>
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	// To prevent huge jump on screen when first starting program
	if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }

	// Calculate mouse offset since last frame
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed: y ranges from bottom to top
	lastX = xpos;
	lastY = ypos;

	// Adjust according to camera sensitivity
	xoffset *= cameraSensitivity;
	yoffset *= cameraSensitivity;

	// Add the offsets to the yaw and pitch
	yaw += xoffset;
	pitch += yoffset;

	// Prevent LookAt Flip if pitch matches the World Up vector
	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;

	// Do trig calculations to determine the direction vector
	glm::vec3 direction;
	direction.x = glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
	direction.y = glm::sin(glm::radians(pitch));
	direction.z = glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
	cameraFront = glm::normalize(direction);
}