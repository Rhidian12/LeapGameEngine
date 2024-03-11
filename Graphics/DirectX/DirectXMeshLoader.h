#pragma once

#include "Singleton.h"

#include "../Data/Vertex.h"

#include <unordered_map>
#include <string>
#include <vector>

struct ID3D11Buffer;
struct ID3D11Device;

namespace leap::graphics
{
	class CustomMesh;

	class DirectXMeshLoader final
	{
	public:
		~DirectXMeshLoader();

		struct DirectXMeshDefinition
		{
			ID3D11Buffer* vertexBuffer{};
			ID3D11Buffer* indexBuffer{};
			unsigned int nrIndices{};
			unsigned int vertexSize{};
		};

		const DirectXMeshDefinition& LoadMesh(const std::string& dataPath, ID3D11Device* pDevice);
		const DirectXMeshDefinition& LoadMesh(const CustomMesh& mesh, ID3D11Device* pDevice);

		void RemoveCustomMesh(ID3D11Buffer* pVertexBuffer);

		void Reload(ID3D11Device* pDevice);

	private:
		friend class LeapEngine;

		DirectXMeshLoader() = default;

		DirectXMeshDefinition CreateMesh(const std::string& dataPath, ID3D11Device* pDevice) const;
		DirectXMeshDefinition CreateMesh(const std::vector<Vertex> vertices, const std::vector<unsigned int> indices, ID3D11Device* pDevice) const;
		DirectXMeshDefinition CreateMesh(const std::vector<unsigned char>& vertexData, unsigned int vertexSize, const std::vector<unsigned int>& indices, ID3D11Device* pDevice) const;

		std::unordered_map<std::string, DirectXMeshDefinition> m_Meshes{};
		std::vector<DirectXMeshDefinition> m_CustomMeshes{};

		friend Singleton<DirectXMeshLoader>;
	};
}