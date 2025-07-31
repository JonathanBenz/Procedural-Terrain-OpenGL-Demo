#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"
#include "texture.h"

struct Material
{
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;
};

enum MeshType : uint8_t
{
	Skybox, Sun
};

/// <summary>
/// This class handles generating and buffering Mesh data to their respective buffer objects and drawing them.
/// </summary>
class Mesh
{
public:	
	unsigned int VAO, VBO;
	Material material;

	Mesh(Shader& shader);
	Mesh(Shader& shader, MeshType type);
	void DrawQuad() const;
	void DrawSphere() const;

private:
	unsigned int indexCount;
	MeshType _type;
};