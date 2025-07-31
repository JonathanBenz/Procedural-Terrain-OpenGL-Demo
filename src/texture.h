#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Texture
{
public:
	unsigned int _textureID;

	Texture(const char* textureName, bool clamp = false);
	Texture(std::vector<unsigned char> heightMap, int textureSize);
	Texture(std::vector<glm::vec3> normalMap, int textureSize);
	Texture(std::vector<std::string> faces);
};