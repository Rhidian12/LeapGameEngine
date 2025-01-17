#pragma once
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11Texture2D;
struct ID3D11DepthStencilView;
struct ID3D11Resource;
struct ID3D11RenderTargetView;

#include "../Interfaces/IRenderer.h"

#include "../Data/RenderData.h"
#include "../DirectionalLight.h"

#include "DirectXRenderTarget.h"
#include "DirectXTexture.h"
#include "DirectXMeshRenderer.h"
#include "DirectXMaterial.h"

#include "DirectXShadowRenderer.h"
#include "DirectXSpriteRenderer.h"

#include <vector>
#include <memory>
#include <unordered_map>

#include "../Data/CustomMesh.h"

class GLFWwindow;

namespace leap::graphics
{
	class Camera;
	class IMeshRenderer;
	class IMaterial;

	class DirectXEngine final : public IRenderer
	{
	public:
		DirectXEngine(GLFWwindow* pWindow);
		~DirectXEngine() override;
		DirectXEngine(const DirectXEngine& other) = delete;
		DirectXEngine(DirectXEngine&& other) = delete;
		DirectXEngine& operator=(const DirectXEngine& other) = delete;
		DirectXEngine& operator=(DirectXEngine&& other) = delete;

		// Internal functions
		virtual void Initialize() override;
		virtual void Draw() override;
		virtual void GuiDraw() override;

		// Renderer settings
		virtual void SetAntiAliasing(AntiAliasing antiAliasing) override;
		virtual void SetWindowSize(const glm::ivec2& size) override;
		virtual void SetShadowMapData(unsigned int shadowMapWidth, unsigned int shadowMapHeight, float orthoSize, float nearPlane, float farPlane) override;

		// Graphics space objects
		virtual void SetActiveCamera(Camera* pCamera) override { m_pCamera = pCamera; }
		virtual Camera* GetCamera() const override { return m_pCamera; }
		virtual void SetDirectionLight(const glm::mat3x3& transform) override;

		// Meshes
		virtual IMeshRenderer* CreateMeshRenderer() override;
		virtual void RemoveMeshRenderer(IMeshRenderer* pMeshRenderer) override;

		// Sprites
		virtual void AddSprite(Sprite* pSprite) override;
		virtual void RemoveSprite(Sprite* pSprite) override;

		// Materials & Textures
		virtual IMaterial* CreateMaterial(std::unique_ptr<Shader, ShaderDelete> pShader, const std::string& name) override;
		virtual IMaterial* CloneMaterial(const std::string& original, const std::string& clone) override;
		virtual ITexture* CreateTexture(const std::string& path) override;
		virtual ITexture* CreateTexture(int width, int height) override;

		// Debug rendering
		virtual void DrawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>& triangles) override;
		virtual void DrawLine(const glm::vec3& start, const glm::vec3& end) override;

	private:
		void Release();
		void ReloadDirectXEngine();
		void RenderCameraView() const;
		void SetupNonCameraView() const;

		AntiAliasing m_AntiAliasing{ AntiAliasing::X16 };

		GLFWwindow* m_pWindow;
		ID3D11Device* m_pDevice{ nullptr };
		ID3D11DeviceContext* m_pDeviceContext{ nullptr };
		IDXGISwapChain* m_pSwapChain{ nullptr };
		DirectXRenderTarget m_RenderTarget{};
		DirectXShadowRenderer m_ShadowRenderer{};
		DirectXSpriteRenderer m_SpriteRenderer{};

		std::vector<std::unique_ptr<DirectXMeshRenderer>> m_pRenderers{};
		std::unordered_map<std::string, std::unique_ptr<DirectXMaterial>> m_pMaterials{};
		std::unordered_map<std::string, std::unique_ptr<DirectXTexture>> m_pTextures{};
		std::vector<std::unique_ptr<DirectXTexture>> m_pUniqueTextures{};

		bool m_IsInitialized{};
		Camera* m_pCamera{};
		DirectionalLight m_DirectionalLight{};

		CustomMesh m_DebugDrawings{};
		IMeshRenderer* m_pDebugRenderer{};
	};
}