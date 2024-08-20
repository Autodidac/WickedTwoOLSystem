#pragma once

#include <DirectXMath.h>
#include "TwoOLSystem.h" // Include your L-system library header
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>

std::string LSystemNode::serialize() const {
	std::ostringstream oss;
	oss << static_cast<int>(type) << " " << nodeid << " " << parentid << " "
		<< stage << " " << length << " " << radius << " " << angle << " "
		<< static_cast<float>(position.x) << " " << static_cast<float>(position.y) << " " << static_cast<float>(position.z) << " "
		<< static_cast<float>(rotation.x) << " " << static_cast<float>(rotation.y) << " " << static_cast<float>(rotation.z) << " " << static_cast<float>(rotation.w);
	return oss.str();
}

LSystemNode LSystemNode::deserialize(const std::string& data) {
	LSystemNode node{};
	std::istringstream iss(data);
	int typeInt;

	// Output the input data for debugging
	wi::backlog::post("Deserializing data: [" + data + "]", wi::backlog::LogLevel::Default);

	// Check for leading and trailing whitespaces
	if (data.empty()) {
		wi::backlog::post("Input data is empty", wi::backlog::LogLevel::Error);
		return node;
	}

	if (!(iss >> typeInt)) {
		wi::backlog::post("Failed to read type. Input data: [" + data + "]", wi::backlog::LogLevel::Error);
		return node;
	}
	//wi::backlog::post("Read type: " + std::to_string(typeInt), wi::backlog::LogLevel::Default);

	if (!(iss >> node.nodeid)) {
		wi::backlog::post("Failed to read nodeid", wi::backlog::LogLevel::Error);
		return node;
	}
	//wi::backlog::post("Read nodeid: " + std::to_string(node.nodeid), wi::backlog::LogLevel::Default);

	if (!(iss >> node.parentid)) {
		wi::backlog::post("Failed to read parentid", wi::backlog::LogLevel::Error);
		return node;
	}
	//wi::backlog::post("Read parentid: " + std::to_string(node.parentid), wi::backlog::LogLevel::Default);

	if (!(iss >> node.stage)) {
		wi::backlog::post("Failed to read stage", wi::backlog::LogLevel::Error);
		return node;
	}
	//wi::backlog::post("Read stage: " + std::to_string(node.stage), wi::backlog::LogLevel::Default);

	if (!(iss >> node.length)) {
		wi::backlog::post("Failed to read length", wi::backlog::LogLevel::Error);
		return node;
	}
	//wi::backlog::post("Read length: " + std::to_string(node.length), wi::backlog::LogLevel::Default);

	if (!(iss >> node.radius)) {
		wi::backlog::post("Failed to read radius", wi::backlog::LogLevel::Error);
		return node;
	}
	//wi::backlog::post("Read radius: " + std::to_string(node.radius), wi::backlog::LogLevel::Default);

	if (!(iss >> node.angle)) {
		wi::backlog::post("Failed to read angle", wi::backlog::LogLevel::Error);
		return node;
	}
	//wi::backlog::post("Read angle: " + std::to_string(node.angle), wi::backlog::LogLevel::Default);

	if (!(iss >> node.position.x >> node.position.y >> node.position.z)) {
		wi::backlog::post("Failed to read position", wi::backlog::LogLevel::Error);
		return node;
	}
	//wi::backlog::post("Read position: " + std::to_string(node.position.x) + ", " + std::to_string(node.position.y) + ", " + std::to_string(node.position.z), wi::backlog::LogLevel::Default);

	if (!(iss >> node.rotation.x >> node.rotation.y >> node.rotation.z >> node.rotation.w)) {
		wi::backlog::post("Failed to read rotation", wi::backlog::LogLevel::Error);
		return node;
	}
	//wi::backlog::post("Read rotation: " + std::to_string(node.rotation.x) + ", " + std::to_string(node.rotation.y) + ", " + std::to_string(node.rotation.z) + ", " + std::to_string(node.rotation.w), wi::backlog::LogLevel::Default);

	node.type = static_cast<NodeType>(typeInt);

	// Output the nodeid after deserialization for debugging
	wi::backlog::post("Deserialized node ID: " + std::to_string(node.nodeid), wi::backlog::LogLevel::Default);

	return node;
}

void saveGenerationsToFile(const std::vector<LSystemGeneration>& generations, const std::string& filename) {
	std::ofstream file(filename);
	if (file.is_open()) {
		for (const auto& generation : generations) {
			for (const auto& node : generation) {
				file << node.serialize() << "\n";
			}
			file << "---\n";  // Separate generations with a marker
		}
		file.close();
	}
	else {
		std::cerr << "Unable to open file for saving: " << filename << "\n";
	}
}

std::vector<LSystemGeneration> loadGenerationsFromFile(const std::string& filename) {
	std::vector<LSystemGeneration> generations;
	std::ifstream file(filename);
	if (file.is_open()) {
		LSystemGeneration currentGeneration;
		std::string line;
		while (std::getline(file, line)) {
			if (line == "---") {
				generations.push_back(std::move(currentGeneration));
				currentGeneration.clear();
			}
			else if (!line.empty()) {
				currentGeneration.push_back(LSystemNode::deserialize(line));
			}
		}
		if (!currentGeneration.empty()) {
			generations.push_back(std::move(currentGeneration));
		}
		file.close();
	}
	else {
		std::cerr << "Unable to open file for loading: " << filename << "\n";
	}
	return generations;
}

void simulateGrowth(std::vector<LSystemGeneration>& generations, double elapsedTime) {
	float timeScale = static_cast<float>(elapsedTime) / 1000000.0f; // Convert to seconds
	for (auto& gen : generations) {
		for (auto& node : gen) {
			switch (node.type) {
			case NodeType::Forward:
				// Example growth simulation: increase length and radius over time
				if (elapsedTime > node.stage) {
					node.length += timeScale * 0.100f; // Adjust growth rate as needed
					node.radius += timeScale * 0.100f;
				}
				break;
			case NodeType::Branch:
				// Example growth simulation: increase length and radius over time
				if (elapsedTime > node.stage) {
					node.length += timeScale * 0.050f; // Adjust growth rate as needed
					node.radius += timeScale * 0.050f;
				}
				break;
			case NodeType::Twig:
				// Example growth simulation: increase length and radius over time
				if (elapsedTime > node.stage) {
					node.length += timeScale * 0.010f; // Adjust growth rate as needed
					node.radius += timeScale * 0.010f;
				}
				break;
			case NodeType::Leaf:
				// Example growth simulation: increase length and radius over time
				if (elapsedTime > node.stage) {
					node.length += timeScale * 0.005f; // Adjust growth rate as needed
					node.radius += timeScale * 0.005f;
				}
				break;
			case NodeType::Decal:
				// Example growth simulation: increase length and radius over time
				if (elapsedTime > node.stage) {
					node.length += timeScale * 0.001f; // Adjust growth rate as needed
					node.radius += timeScale * 0.001f;
				}
				break;
			default:
				break;
			}
		}
	}
}

void simulateNegativeGrowth(std::vector<LSystemGeneration>& generations, double elapsedTime) {
	float timeScale = static_cast<float>(elapsedTime) / 1000000.0f; // Convert to microseconds
	for (auto& gen : generations) {
		for (auto& node : gen) {
			switch (node.type) {
			case NodeType::Forward:
				// Example negative growth simulation: decrease length and radius over time
				if (elapsedTime > node.stage) {
					node.length -= timeScale * 0.100f; // Adjust growth rate as needed
					node.radius -= timeScale * 0.100f;
				}
				break;
			case NodeType::Branch:
				// Example negative growth simulation: decrease length and radius over time
				if (elapsedTime > node.stage) {
					node.length -= timeScale * 0.050f; // Adjust growth rate as needed
					node.radius -= timeScale * 0.050f;
				}
				break;
			case NodeType::Twig:
				// Example negative growth simulation: decrease length and radius over time
				if (elapsedTime > node.stage) {
					node.length -= timeScale * 0.010f; // Adjust growth rate as needed
					node.radius -= timeScale * 0.010f;
				}
				break;
			case NodeType::Leaf:
				// Example negative growth simulation: decrease length and radius over time
				if (elapsedTime > node.stage) {
					node.length -= timeScale * 0.005f; // Adjust growth rate as needed
					node.radius -= timeScale * 0.005f;
				}
				break;
			case NodeType::Decal:
				// Example negative growth simulation: decrease length and radius over time
				if (elapsedTime > node.stage) {
					node.length -= timeScale * 0.001f; // Adjust growth rate as needed
					node.radius -= timeScale * 0.001f;
				}
				break;
			default:
				break;
			}
		}
	}
}
