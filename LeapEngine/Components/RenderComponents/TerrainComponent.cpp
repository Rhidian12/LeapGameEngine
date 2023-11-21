#include "TerrainComponent.h"

#include <Interfaces/ITexture.h>
#include <Interfaces/IRenderer.h>
#include <Interfaces/IMeshRenderer.h>
#include <Data/Vertex.h>
#include <Shaders/Heightmap.h>

#include "../../ServiceLocator/ServiceLocator.h"

#include "../../Graphics/Shader.h"

//#include "TerrainComponent.h"
//
//#include "MeshRenderer.h"
//
//#include "../../SceneGraph/GameObject.h"
//#include "../../ServiceLocator/ServiceLocator.h"
//
//#include <Interfaces/IRenderer.h>
//
//#include <Data/CustomMesh.h>
//#include <Shaders/Heightmap.h>
//#include <Shaders/Pos3D.h>
//
//#include <Interfaces/IMaterial.h>
//#include <Interfaces/ITexture.h>
//
//#include "../../Graphics/Mesh.h"
//
//#include <Data/Vertex.h>
//
//#include <Debug.h>
//
//#include <sstream>
//
//void leap::TerrainComponent::SetTexture(leap::graphics::ITexture* pTexture)
//{
//	// Cache terrain size
//	m_Size = pTexture->GetSize().x;
//
//	// Cache the texture
//	m_pTexture = pTexture;
//
//	// Initialize the terrain mesh
//	Init();
//
//	// Apply the texture to the mesh
//	ApplyTexture();
//}
//
//void leap::TerrainComponent::SetSize(unsigned int size)
//{
//	// Cache terrain size
//	m_Size = size;
//
//	const unsigned int sizeInVertices{ m_Size + 1 };
//
//	// Create height information
//	m_Heights.resize(static_cast<size_t>(sizeInVertices * sizeInVertices));
//
//	// Create texture
//	m_pTexture = ServiceLocator::GetRenderer().CreateTexture(sizeInVertices, sizeInVertices);
//
//	// Initialize the terrain mesh
//	Init();
//
//	// Apply the texture to the mesh
//	ApplyTexture();
//}
//
//std::vector<float>& leap::TerrainComponent::GetHeights()
//{
//	// Initialize the terrain mesh
//	Init();
//
//	return m_Heights;
//}
//
//void leap::TerrainComponent::ApplyHeights()
//{
//	m_pTexture->SetData(m_Heights.data(), static_cast<unsigned int>(m_Heights.size() * sizeof(float)));
//
//	CheckExistenceMeshRenderer();
//	ApplyTexture();
//}
//
//void leap::TerrainComponent::ApplyTexture()
//{
//	// Apply texture
//	m_pMeshRenderer->GetMaterial()->SetTexture("gHeightMap", m_pTexture);
//}
//
//void leap::TerrainComponent::Init()
//{
//	if (m_IsInitialized) return;
//
//	// Set a default size, if none is applied
//	if (m_Size == 0)
//	{
//		constexpr unsigned int defaultSize{ 1000 };
//		SetSize(defaultSize);
//	}
//
//	m_IsInitialized = true;
//
//	CheckExistenceMeshRenderer();
//	ApplyTexture();
//
//	// Create mesh
//	static Mesh plane{};
//	if (!m_MeshInitialized)
//	{
//		m_MeshInitialized = true;
//
//		plane.SetWritable(true);
//
//		std::vector<leap::graphics::VertexPosUV> vertices{};
//		std::vector<unsigned> indices{};
//
//		for (unsigned int x{}; x < m_Size; ++x)
//		{
//			for (unsigned int y{}; y < m_Size; ++y)
//			{
//				const unsigned int startIdx{ static_cast<unsigned>(vertices.size()) };
//
//				vertices.emplace_back(leap::graphics::VertexPosUV{ { x, 0.0f, y + 1.0f }, { x, y + 1.0f } });
//				vertices.emplace_back(leap::graphics::VertexPosUV{ { x + 1.0f, 0.0f, y + 1.0f }, { x + 1.0f, y + 1.0f } });
//				vertices.emplace_back(leap::graphics::VertexPosUV{ { x + 1.0f, 0.0f, y }, { x + 1.0f, y } });
//				vertices.emplace_back(leap::graphics::VertexPosUV{ { x, 0.0f, y }, { x, y } });
//
//				indices.emplace_back(startIdx);
//				indices.emplace_back(startIdx + 1);
//				indices.emplace_back(startIdx + 2);
//
//				indices.emplace_back(startIdx);
//				indices.emplace_back(startIdx + 2);
//				indices.emplace_back(startIdx + 3);
//			}
//		}
//
//		plane.SetData(vertices, std::move(indices));
//	}
//
//	m_pMeshRenderer->SetMesh(plane);
//}
//
//void leap::TerrainComponent::CheckExistenceMeshRenderer()
//{
//	if (!m_pMeshRenderer)
//	{
//		m_pMeshRenderer = GetGameObject()->AddComponent<MeshRenderer>();
//
//		// Create material
//		std::stringstream ss{};
//		ss << "Heightmap" << this;
//		graphics::IMaterial* pMaterial{ ServiceLocator::GetRenderer().CreateMaterial(graphics::shaders::Heightmap::GetShader(), ss.str()) };
//		m_pMeshRenderer->SetMaterial(pMaterial);
//	}
//}

leap::TerrainComponent::TerrainComponent()
{
	m_pRenderer = ServiceLocator::GetRenderer().CreateMeshRenderer();
	if (!m_pMaterial) m_pMaterial = std::make_unique<Material>("Heightmap", Shader<graphics::shaders::Heightmap>(), true);
	m_pRenderer->SetMaterial(m_pMaterial->GetInternal());
}

leap::TerrainComponent::~TerrainComponent()
{
	auto& meshData{ m_Meshes[m_Size] };
	--meshData.useCounter;
	if (meshData.useCounter == 0) m_Meshes.erase(m_Size);
}

void leap::TerrainComponent::SetSize(unsigned int size)
{
	if (m_Size == size) return;

	m_Size = size;

	auto pTexture{ ServiceLocator::GetRenderer().CreateTexture(size,size) };
	std::vector<float> heights{};
	heights.resize(size * size);
	pTexture->SetData(heights.data(), static_cast<unsigned int>(heights.size() * sizeof(float)));

	m_pRenderer->GetMaterial()->SetTexture("gHeightMap", pTexture/*ServiceLocator::GetRenderer().CreateTexture(m_Size, m_Size)*/);

	if (!m_Meshes.contains(size)) CreateMesh(size);

	TerrainMesh& mesh{ m_Meshes[size] };
	++mesh.useCounter;
	m_pRenderer->SetMesh(mesh.mesh.GetInternal());
}

void leap::TerrainComponent::OnDestroy()
{
	ServiceLocator::GetRenderer().RemoveMeshRenderer(m_pRenderer);
}

void leap::TerrainComponent::CreateMesh(unsigned int size)
{
	Mesh mesh{};

	mesh.SetWritable(true);

	std::vector<leap::graphics::VertexPosUV> vertices{};
	std::vector<unsigned> indices{};

	for (unsigned int x{}; x < size; ++x)
	{
		for (unsigned int y{}; y < size; ++y)
		{
			const unsigned int startIdx{ static_cast<unsigned>(vertices.size()) };

			vertices.emplace_back(leap::graphics::VertexPosUV{ { x, 0.0f, y + 1.0f }, { x, y + 1.0f } });
			vertices.emplace_back(leap::graphics::VertexPosUV{ { x + 1.0f, 0.0f, y + 1.0f }, { x + 1.0f, y + 1.0f } });
			vertices.emplace_back(leap::graphics::VertexPosUV{ { x + 1.0f, 0.0f, y }, { x + 1.0f, y } });
			vertices.emplace_back(leap::graphics::VertexPosUV{ { x, 0.0f, y }, { x, y } });

			indices.emplace_back(startIdx);
			indices.emplace_back(startIdx + 1);
			indices.emplace_back(startIdx + 2);

			indices.emplace_back(startIdx);
			indices.emplace_back(startIdx + 2);
			indices.emplace_back(startIdx + 3);
		}
	}

	mesh.SetData(vertices, std::move(indices));

	m_Meshes[size] = TerrainMesh{ size, std::move(mesh) };
}
