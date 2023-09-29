#include "glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "glfw3native.h"
#include "DirectXEngine.h"
#include "DirectXTex.h"

#include <glm.hpp>

#include "../Camera.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>

#include "DirectXShaderReader.h"
#include "DirectXTexture.h"
#include "DirectXMeshLoader.h"

leap::graphics::DirectXEngine::DirectXEngine(GLFWwindow* pWindow) : m_pWindow(pWindow)
{

}

leap::graphics::DirectXEngine::~DirectXEngine()
{
	Release();
}

void leap::graphics::DirectXEngine::Initialize()
{
	ReloadDirectXEngine();

	m_IsInitialized = true;
}

void leap::graphics::DirectXEngine::SetAntiAliasing(AntiAliasing antiAliasing)
{
	m_AntiAliasing = antiAliasing;

	if(m_IsInitialized) ReloadDirectXEngine();
}

leap::graphics::IMeshRenderer* leap::graphics::DirectXEngine::CreateMeshRenderer()
{
	m_pRenderers.push_back(std::make_unique<DirectXMeshRenderer>(m_pDevice, m_pDeviceContext));
	return m_pRenderers[m_pRenderers.size() - 1].get();
}

void leap::graphics::DirectXEngine::RemoveMeshRenderer(IMeshRenderer* pMeshRenderer)
{
	m_pRenderers.erase(std::remove_if(begin(m_pRenderers), end(m_pRenderers), [pMeshRenderer](const auto& pRenderer) { return pMeshRenderer == pRenderer.get(); }));
}

leap::graphics::IMaterial* leap::graphics::DirectXEngine::CreateMaterial(std::unique_ptr<Shader, ShaderDelete> pShader, const std::string& name)
{
	const DirectXShader shader{ DirectXShaderReader::GetShaderData(std::move(pShader)) };
	if (auto it{ m_pMaterials.find(name) }; it != end(m_pMaterials))
	{
		return it->second.get();
	}

	auto pMaterial{ std::make_unique<DirectXMaterial>(m_pDevice, shader.path, shader.vertexDataFunction) };
	auto pMaterialRaw{ pMaterial.get() };

	m_pMaterials[name] = std::move(pMaterial);
	return pMaterialRaw;
}

leap::graphics::ITexture* leap::graphics::DirectXEngine::CreateTexture(const std::string& path)
{
	if (auto it{ m_pTextures.find(path) }; it != end(m_pTextures))
	{
		return it->second.get();
	}

	auto pTexture{ std::make_unique<DirectXTexture>(m_pDevice, path) };
	auto pTextureRaw{ pTexture.get() };

	m_pTextures[path] = std::move(pTexture);
	return pTextureRaw;
}

void leap::graphics::DirectXEngine::SetDirectionLight(const glm::vec3& direction)
{
	for (const auto& pMaterial : m_pMaterials)
	{
		pMaterial.second->SetFloat3("gLightDirection", direction);
	}
}

void leap::graphics::DirectXEngine::Release()
{
	if (m_pSwapChain)
	{
		m_pSwapChain->Release();
		m_pSwapChain = nullptr;
	}
	if (m_pDeviceContext)
	{
		m_pDeviceContext->ClearState();
		m_pDeviceContext->Flush();
		m_pDeviceContext->Release();
		m_pDeviceContext = nullptr;
	}
	if (m_pDevice)
	{
		m_pDevice->Release();
		m_pDevice = nullptr;
	}
}

void leap::graphics::DirectXEngine::ReloadDirectXEngine()
{
	// Release the previous version of the graphics engine
	Release();

	/// Create device & Device context
	UINT createDeviceFlags = 0;
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;

#if defined(DEBUG) || defined(_DEBUG)
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	IDXGIFactory1* pFactory = nullptr;
	HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pFactory));
	HRESULT result{};

	if (SUCCEEDED(hr))
	{
		IDXGIAdapter1* pAdapter = nullptr;

		hr = pFactory->EnumAdapters1(0, &pAdapter);
		if (SUCCEEDED(hr))
		{
			result = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, createDeviceFlags, &featureLevel, 1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext);

			if (FAILED(result)) return;

			pAdapter->Release();
		}

		pFactory->Release();
	}

	// Check if the anti aliasing level is supported
	UINT supportedLevels{};
	result = m_pDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT>(m_AntiAliasing), &supportedLevels);
	while (FAILED(result) || supportedLevels == 0)
	{
		m_AntiAliasing = static_cast<AntiAliasing>(static_cast<UINT>(m_AntiAliasing) / 2);
		result = m_pDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT>(m_AntiAliasing), &supportedLevels);
	}

	// Get the window size to use for the swap chain
	int width, height;
	glfwGetWindowSize(m_pWindow, &width, &height);

	// Create swapchain
	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = static_cast<UINT>(m_AntiAliasing);
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;
	swapChainDesc.OutputWindow = glfwGetWin32Window(m_pWindow);

	IDXGIFactory1* pDxgiFactory{ nullptr };
	result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pDxgiFactory));
	if (FAILED(result)) return;

	result = pDxgiFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
	if (FAILED(result)) return;

	// Create render target
	ID3D11Texture2D* pRenderTarget{};
	result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pRenderTarget));
	if (FAILED(result)) return;

	DirectXRenderTarget::RTDesc renderTargetDesc{};
	renderTargetDesc.width = width;
	renderTargetDesc.height = height;
	renderTargetDesc.pOptionalColorBuffer = pRenderTarget;
	renderTargetDesc.hasDepthTexture = true;
	renderTargetDesc.hasColorTexture = true;
	renderTargetDesc.antiAliasing = m_AntiAliasing;

	m_RenderTarget.Create(m_pDevice, renderTargetDesc);

	/// Bind views
	ID3D11RenderTargetView* pRenderTargetView{ m_RenderTarget.GetColorView() };
	m_pDeviceContext->OMSetRenderTargets(1, &pRenderTargetView, m_RenderTarget.GetDepthView());

	/// Set viewport
	D3D11_VIEWPORT viewport{};
	viewport.Width = static_cast<FLOAT>(width);
	viewport.Height = static_cast<FLOAT>(height);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	m_pDeviceContext->RSSetViewports(1, &viewport);

	// Reload existing textures, materials & meshes using new video settings
	for (const auto& texturePair : m_pTextures)
	{
		texturePair.second->Reload(m_pDevice, texturePair.first);
	}

	for (const auto& materialPair : m_pMaterials)
	{
		materialPair.second->Reload(m_pDevice);
	}

	DirectXMeshLoader::GetInstance().Reload(m_pDevice);

	for (const auto& pRenderer : m_pRenderers)
	{
		pRenderer->Reload(m_pDevice, m_pDeviceContext);
	}
}

void leap::graphics::DirectXEngine::Draw()
{
	if (!m_IsInitialized) return;
	if (!m_pCamera) return;

	const glm::vec4& clearColor = m_pCamera->GetColor();
	m_pDeviceContext->ClearRenderTargetView(m_RenderTarget.GetColorView(), &clearColor.r);
	m_pDeviceContext->ClearDepthStencilView(m_RenderTarget.GetDepthView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	DirectXMaterial::SetViewProjectionMatrix(m_pCamera->GetProjectionMatrix() * m_pCamera->GetViewMatrix());

	for (const auto& pRenderer : m_pRenderers)
	{
		pRenderer->Draw();
	}

	m_pSwapChain->Present(0, 0);
}
