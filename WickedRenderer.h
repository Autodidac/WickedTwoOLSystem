#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <random>
#include <WickedEngine.h>
#include <DirectXMath.h>
#include "TwoOLSystem.h" // Include your L-system library header

using namespace wi;

// Forward declaration of LSystemNode
class LSystemNode;

using LSystemGeneration = std::vector<LSystemNode>;

class WickedRenderer {
public:
	WickedRenderer();
	~WickedRenderer();

	void CreateTree(wi::scene::Scene& scene, const std::string& filename, const std::vector<std::vector<LSystemNode>>& generations);
	void SaveTree(const std::vector<LSystemGeneration>& generations, const std::string& filename);
	void LoadTree(const std::string& filename, std::vector<LSystemGeneration>& generations);

private:
	ecs::Entity entity = ecs::INVALID_ENTITY;
	ecs::Entity partEntity = ecs::INVALID_ENTITY;
	scene::TransformComponent tfm;
	/*
	DirectX::XMFLOAT4 colorsaddleBrownEmissive;
	DirectX::XMFLOAT3 colorBrownSubsurface;
	DirectX::XMFLOAT4 colorBrown;
	DirectX::XMFLOAT4 colorSaddleBrown;
*/
	int segments = 16;
	std::uniform_real_distribution<float> disAngle{ 0.0f, 45.0f };

	// Private members already initialized in the header
	DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 rotation{ 0.0f, 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scale{ 0.0f, 0.0f, 0.0f };

	std::vector<DirectX::XMFLOAT3> startPositions;
	std::vector<DirectX::XMFLOAT3> endPositions;

	std::vector<DirectX::XMFLOAT3> allVertexPositions;
	std::vector<DirectX::XMFLOAT3> allVertexNormals;
	std::vector<DirectX::XMFLOAT2> allVertexUVs;
	std::vector<uint32_t> allIndices;
	uint32_t vertexOffset = 0;

	std::vector<DirectX::XMFLOAT3> vertex_positions;
	std::vector<DirectX::XMFLOAT3> vertex_normals;
	std::vector<DirectX::XMFLOAT2> vertex_uvs;
	std::vector<uint32_t> indices;

	std::vector<DirectX::XMFLOAT3> weldedVertexPositions;
	std::vector<DirectX::XMFLOAT3> weldedVertexNormals;
	std::vector<DirectX::XMFLOAT2> weldedVertexUVs;
	std::vector<uint32_t> weldedIndices;
};
