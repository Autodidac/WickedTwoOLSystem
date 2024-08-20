#pragma once

#include "framework.h"
#include <string>
#include <vector>
#include <DirectXMath.h>
#include "FileManagerWin32.h"
#include "TimeSimulator.h"
#include "WickedRenderer.h"

// Enum to represent different types of nodes
enum class NodeType {
	Base,
	Forward,
	Branch,
	Twig,
	Leaf,
	Decal
};

// Class representing a node in the L-system
class LSystemNode {
public:
	NodeType type;
	int parentid;      // ID of the parent node
	int nodeid;        // Unique ID for the node
	float stage;       // Stage of growth or age
	float length;      // Length of the node
	float radius;      // Radius of the node
	float angle;       // Angle or orientation of the node
	DirectX::XMFLOAT3 position; // Position of the node
	DirectX::XMFLOAT4 rotation; // Rotation quaternion of the node

	std::string serialize() const;
	static LSystemNode deserialize(const std::string& data);
};

// Structure for representing a generation (collection of nodes)
using LSystemGeneration = std::vector<LSystemNode>;

// Function declarations for operations with L-system generations
void saveGenerationsToFile(const std::vector<LSystemGeneration>& generations, const std::string& filename);
std::vector<LSystemGeneration> loadGenerationsFromFile(const std::string& filename);
void simulateGrowth(std::vector<LSystemGeneration>& generations, double elapsedTime);
void simulateNegativeGrowth(std::vector<LSystemGeneration>& generations, double elapsedTime);
