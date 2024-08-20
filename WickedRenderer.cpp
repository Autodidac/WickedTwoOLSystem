#include "WickedRenderer.h"
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <DirectXMath.h>
#include <iostream>
#include <random>
#include <vector>

//using namespace wi;
using namespace wi::ecs;
using namespace wi::scene;
using namespace DirectX;

WickedRenderer::WickedRenderer() {
    // Constructor implementation
}

WickedRenderer::~WickedRenderer() {
    // Destructor implementation
}

struct Vertex {
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 uv;

	bool operator==(const Vertex& other) const {
		return position.x == other.position.x && position.y == other.position.y && position.z == other.position.z &&
			normal.x == other.normal.x && normal.y == other.normal.y && normal.z == other.normal.z &&
			uv.x == other.uv.x && uv.y == other.uv.y;
	}
};

namespace std {
	template <>
	struct hash<Vertex> {
		size_t operator()(const Vertex& v) const {
			size_t h1 = hash<float>()(v.position.x);
			size_t h2 = hash<float>()(v.position.y);
			size_t h3 = hash<float>()(v.position.z);
			size_t h4 = hash<float>()(v.normal.x);
			size_t h5 = hash<float>()(v.normal.y);
			size_t h6 = hash<float>()(v.normal.z);
			size_t h7 = hash<float>()(v.uv.x);
			size_t h8 = hash<float>()(v.uv.y);
			return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5) ^ (h7 << 6) ^ (h8 << 7);
		}
	};
}

static void WeldVertices(
	const std::vector<XMFLOAT3>& vertex_positions,
	const std::vector<XMFLOAT3>& vertex_normals,
	const std::vector<XMFLOAT2>& vertex_uvs,
	std::vector<XMFLOAT3>& weldedVertexPositions,
	std::vector<XMFLOAT3>& weldedVertexNormals,
	std::vector<XMFLOAT2>& weldedVertexUVs,
	std::vector<uint32_t>& weldedIndices
) {
	std::unordered_map<Vertex, uint32_t> uniqueVertices;
	for (size_t i = 0; i < vertex_positions.size(); ++i) {
		Vertex vertex = { vertex_positions[i], vertex_normals[i], vertex_uvs[i] };
		auto it = uniqueVertices.find(vertex);
		if (it == uniqueVertices.end()) {
			uint32_t newIndex = static_cast<uint32_t>(weldedVertexPositions.size());
			uniqueVertices[vertex] = newIndex;

			weldedVertexPositions.push_back(vertex_positions[i]);
			weldedVertexNormals.push_back(vertex_normals[i]);
			weldedVertexUVs.push_back(vertex_uvs[i]);
			weldedIndices.push_back(newIndex);
		}
		else {
			weldedIndices.push_back(it->second);
		}
	}
}

static void GenerateMesh(const std::vector<LSystemNode>& nodes, std::vector<XMFLOAT3>& vertex_positions, std::vector<XMFLOAT3>& vertex_normals, std::vector<XMFLOAT2>& vertex_uvs, std::vector<uint32_t>& indices) {
	std::vector<XMFLOAT3> weldedVertexPositions;
	std::vector<XMFLOAT3> weldedVertexNormals;
	std::vector<XMFLOAT2> weldedVertexUVs;
	std::vector<uint32_t> weldedIndices;

	// Prepare for mesh generation
	for (const auto& node : nodes) {
		float radius = node.radius;
		float height = node.length;

		// Current node's position and rotation
		DirectX::XMFLOAT3 position = node.position;
		DirectX::XMFLOAT4 rotation = node.rotation;

		// Calculate forward vector
		DirectX::XMVECTOR forwardVec = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0, height, 0, 0), DirectX::XMLoadFloat4(&rotation));
		DirectX::XMVECTOR currentPosVec = DirectX::XMLoadFloat3(&position);
		DirectX::XMVECTOR newPosVec = DirectX::XMVectorAdd(currentPosVec, forwardVec);
		DirectX::XMStoreFloat3(&position, newPosVec);

		const uint32_t segments = 16; // Number of segments around the cylinder
		float segment_angle = XM_2PI / static_cast<float>(segments);

		DirectX::XMVECTOR bottomPosVec = currentPosVec;
		DirectX::XMVECTOR topPosVec = newPosVec;
		DirectX::XMFLOAT3 topPos; DirectX::XMStoreFloat3(&topPos, topPosVec);

		for (uint32_t i = 0; i <= segments; ++i) {
			float angle = segment_angle * i;
			float x = radius * cos(angle);
			float z = radius * sin(angle);

			// Bottom vertex
			XMFLOAT3 bottomVertex = XMFLOAT3(x, 0.0f, z);
			DirectX::XMVECTOR rotatedBottomVertex = DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&bottomVertex), DirectX::XMLoadFloat4(&rotation));
			DirectX::XMStoreFloat3(&bottomVertex, DirectX::XMVectorAdd(rotatedBottomVertex, bottomPosVec));
			vertex_positions.push_back(bottomVertex);

			// Top vertex
			XMFLOAT3 topVertex = XMFLOAT3(x, height, z);
			DirectX::XMVECTOR rotatedTopVertex = DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&topVertex), DirectX::XMLoadFloat4(&rotation));
			DirectX::XMStoreFloat3(&topVertex, DirectX::XMVectorAdd(rotatedTopVertex, topPosVec));
			vertex_positions.push_back(topVertex);

			// Normals pointing outwards
			XMFLOAT3 normal = XMFLOAT3(x, 0.0f, z);
			DirectX::XMStoreFloat3(&normal, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&normal)));
			vertex_normals.push_back(normal); // Bottom vertex normal
			vertex_normals.push_back(normal); // Top vertex normal

			// UV coordinates
			vertex_uvs.push_back(XMFLOAT2(static_cast<float>(i) / segments, 0.0f)); // Bottom vertex UV
			vertex_uvs.push_back(XMFLOAT2(static_cast<float>(i) / segments, 1.0f)); // Top vertex UV
		}

		// Indices
		for (uint32_t i = 0; i < segments; ++i) {
			uint32_t base = i * 2;
			uint32_t next = ((i + 1) % segments) * 2;
			indices.push_back(base);
			indices.push_back(next);
			indices.push_back(base + 1);

			indices.push_back(next);
			indices.push_back(next + 1);
			indices.push_back(base + 1);
		}
	}
}

void WickedRenderer::CreateTree(scene::Scene& scene, const std::string& name, const std::vector<LSystemGeneration>& generations) {
	// Create the entity
	ecs::Entity treeEntity = ecs::CreateEntity();
	if (!name.empty()) {
		scene.names.Create(treeEntity) = name + " tree ";
	}

	scene.layers.Create(treeEntity);
	TransformComponent& treeTransform = scene.transforms.Create(treeEntity);
	ObjectComponent& object = scene.objects.Create(treeEntity);

	// Random number generator for rotation
	std::random_device rd;
	std::mt19937 generate(rd());
	std::uniform_real_distribution<float> disAngle(0.0f, DirectX::XM_2PI);

	std::vector<XMFLOAT3> vertex_positions;
	std::vector<XMFLOAT3> vertex_normals;
	std::vector<XMFLOAT2> vertex_uvs;
	std::vector<uint32_t> indices;
	std::vector<XMFLOAT3> weldedVertexPositions;
	std::vector<XMFLOAT3> weldedVertexNormals;
	std::vector<XMFLOAT2> weldedVertexUVs;
	std::vector<uint32_t> weldedIndices;

	// Generate mesh data
	for (const auto& generation : generations) {
		GenerateMesh(generation, vertex_positions, vertex_normals, vertex_uvs, indices);
	}

	// Weld vertices
	WeldVertices(vertex_positions, vertex_normals, vertex_uvs, weldedVertexPositions, weldedVertexNormals, weldedVertexUVs, weldedIndices);

	// Create mesh component
	MeshComponent& mesh = scene.meshes.Create(treeEntity);
	MeshComponent::MeshSubset& subset = mesh.subsets.emplace_back();
	subset.indexCount = static_cast<uint32_t>(weldedIndices.size());

	scene::MaterialComponent& material = scene.materials.Create(treeEntity);
	material.SetDoubleSided(true);
	subset.materialID = treeEntity;

	mesh.vertex_positions = std::move(weldedVertexPositions);
	mesh.vertex_normals = std::move(weldedVertexNormals);
	mesh.vertex_uvset_0 = std::move(weldedVertexUVs);
	mesh.indices = std::move(weldedIndices);

	mesh.CreateRenderData();
	object.meshID = treeEntity;

	// Debug output
	wi::backlog::post("Created tree with " + std::to_string(mesh.vertex_positions.size()) + " vertices and " + std::to_string(mesh.indices.size()) + " indices.", wi::backlog::LogLevel::Default);
}

void WickedRenderer::SaveTree(const std::vector<LSystemGeneration>& generations, const std::string& filename) {
	std::ofstream file(filename, std::ios::binary);
	if (!file) {
		std::cerr << "Failed to open file for writing: " << filename << std::endl;
		return;
	}

	for (const auto& generation : generations) {
		size_t nodeCount = generation.size();
		file.write(reinterpret_cast<const char*>(&nodeCount), sizeof(nodeCount));
		for (const auto& node : generation) {
			file.write(reinterpret_cast<const char*>(&node), sizeof(node)); // Customize as needed
		}
	}
}

void WickedRenderer::LoadTree(const std::string& filename, std::vector<LSystemGeneration>& generations) {
	std::ifstream file(filename, std::ios::binary);
	if (!file) {
		std::cerr << "Failed to open file for reading: " << filename << std::endl;
		return;
	}

	generations.clear();
	while (file) {
		LSystemGeneration generation;
		size_t nodeCount;
		file.read(reinterpret_cast<char*>(&nodeCount), sizeof(nodeCount));
		if (!file) break;

		generation.resize(nodeCount);
		for (size_t i = 0; i < nodeCount; ++i) {
			file.read(reinterpret_cast<char*>(&generation[i]), sizeof(LSystemNode)); // Customize as needed
		}
		generations.push_back(std::move(generation));
	}
}
