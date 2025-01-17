#include "DirectXMaterial.h"

#include <d3d11.h>
#include <d3dx11effect.h>
#include <d3dcompiler.h>

#include <sstream>
#include <vector>

#include "Debug.h"
#include "DirectXTexture.h"

glm::mat4x4 leap::graphics::DirectXMaterial::m_ViewProjMatrix{};

leap::graphics::DirectXMaterial::DirectXMaterial(ID3D11Device* pDevice, const std::string& assetFile, std::function<std::vector<D3D11_INPUT_ELEMENT_DESC>()> vertexDataFunction)
	: m_pEffect{ LoadEffect(pDevice, assetFile) }
	, m_VertexDataFunction{ vertexDataFunction }
	, m_AssetFile{ assetFile }
{
	// Save the technique of the effect as a member variable
	m_pTechnique = m_pEffect->GetTechniqueByName("DefaultTechnique");
	if (!m_pTechnique->IsValid()) Debug::Log("DirectXEngine Error: Failed to load .fx file while creating a material");

	// Save the worldviewprojection and world variable of the effect as a member variable
	ID3DX11EffectVariable* pWorldViewProj{ m_pEffect->GetVariableByName("gWorldViewProj") };
	if(pWorldViewProj) m_pMatWorldViewProjVariable = pWorldViewProj->AsMatrix();
	ID3DX11EffectVariable* pWorld{ m_pEffect->GetVariableByName("gWorld") };
	if(pWorld) m_pMatWorldVariable = pWorld->AsMatrix();

	m_pInputLayout = LoadInputLayout(pDevice);
}

leap::graphics::DirectXMaterial::~DirectXMaterial()
{
	if (m_pEffect) m_pEffect->Release();
	if (m_pInputLayout) m_pInputLayout->Release();
}

ID3D11InputLayout* leap::graphics::DirectXMaterial::LoadInputLayout(ID3D11Device* pDevice) const
{
	std::vector<D3D11_INPUT_ELEMENT_DESC> vertexDesc{ m_VertexDataFunction() };

	// Create input layout
	D3DX11_PASS_DESC passDesc{};
	m_pTechnique->GetPassByIndex(0)->GetDesc(&passDesc);

	ID3D11InputLayout* pInputLayout;

	const HRESULT result{ pDevice->CreateInputLayout
		(
			vertexDesc.data(),
			static_cast<UINT>(vertexDesc.size()),
			passDesc.pIAInputSignature,
			passDesc.IAInputSignatureSize,
			&pInputLayout
		) };
	if (FAILED(result)) Debug::LogError("DirectXEngine Error: Failed to load input layout while creating a material");

	return pInputLayout;
}

ID3DX11EffectTechnique* leap::graphics::DirectXMaterial::GetTechnique() const
{
	return m_pTechnique;
}

void leap::graphics::DirectXMaterial::SetViewProjectionMatrix(const glm::mat4x4& viewProjMatrix)
{
	m_ViewProjMatrix = viewProjMatrix;
}

void leap::graphics::DirectXMaterial::SetWorldMatrix(const glm::mat4x4& worldMatrix)
{
	const glm::mat4x4 wvpMatrix{ m_ViewProjMatrix * worldMatrix };
	if(m_pMatWorldViewProjVariable) m_pMatWorldViewProjVariable->SetMatrix(reinterpret_cast<const float*>(&wvpMatrix));
	if(m_pMatWorldVariable) m_pMatWorldVariable->SetMatrix(reinterpret_cast<const float*>(&worldMatrix));
}

void leap::graphics::DirectXMaterial::SetBool(const std::string& varName, bool data)
{
	auto var = m_pEffect->GetVariableByName(varName.c_str());
	if (var->IsValid()) 
	{
		var->AsScalar()->SetBool(data);
		m_MaterialVariables[varName] = MaterialVariable{ data, sizeof(data) };
	}
}

void leap::graphics::DirectXMaterial::SetFloat(const std::string& varName, float data)
{
	auto var = m_pEffect->GetVariableByName(varName.c_str());
	if (var->IsValid())
	{
		var->AsScalar()->SetFloat(data);
		m_MaterialVariables[varName] = MaterialVariable{ data, sizeof(data) };
	}
}

void leap::graphics::DirectXMaterial::SetFloat2(const std::string& varName, const glm::vec2& data)
{
	auto var = m_pEffect->GetVariableByName(varName.c_str());
	if (var->IsValid())
	{
		var->AsVector()->SetFloatVector(reinterpret_cast<const float*>(&data));
		m_MaterialVariables[varName] = MaterialVariable{ data, sizeof(data) };
	}
}

void leap::graphics::DirectXMaterial::SetFloat3(const std::string& varName, const glm::vec3& data)
{
	auto var = m_pEffect->GetVariableByName(varName.c_str());
	if (var->IsValid())
	{
		var->AsVector()->SetFloatVector(reinterpret_cast<const float*>(&data));
		m_MaterialVariables[varName] = MaterialVariable{ data, sizeof(data) };
	}
}

void leap::graphics::DirectXMaterial::SetFloat4(const std::string& varName, const glm::vec4& data)
{
	auto var = m_pEffect->GetVariableByName(varName.c_str());
	if (var->IsValid())
	{
		var->AsVector()->SetFloatVector(reinterpret_cast<const float*>(&data));
		m_MaterialVariables[varName] = MaterialVariable{ data, sizeof(data) };
	}
}

void leap::graphics::DirectXMaterial::SetMat3x3(const std::string& varName, const glm::mat3x3& data)
{
	auto var = m_pEffect->GetVariableByName(varName.c_str());
	if (var->IsValid()) var->AsMatrix()->SetMatrix(reinterpret_cast<const float*>(&data));
}

void leap::graphics::DirectXMaterial::SetMat4x4(const std::string& varName, const glm::mat4x4& data)
{
	auto var = m_pEffect->GetVariableByName(varName.c_str());
	if (var->IsValid()) var->AsMatrix()->SetMatrix(reinterpret_cast<const float*>(&data));
}

void leap::graphics::DirectXMaterial::SetTexture(const std::string& varName, ITexture* pTexture)
{
	auto var = m_pEffect->GetVariableByName(varName.c_str());
	if (var->IsValid())
	{
		m_pTextures[varName] = static_cast<DirectXTexture*>(pTexture);
		var->AsShaderResource()->SetResource(static_cast<DirectXTexture*>(pTexture)->GetResource());
	}
}

void leap::graphics::DirectXMaterial::SetTexture(const std::string& varName, ID3D11ShaderResourceView* pSRV)
{
	auto var = m_pEffect->GetVariableByName(varName.c_str());
	if (var->IsValid())
	{
		var->AsShaderResource()->SetResource(pSRV);
	}
}

void leap::graphics::DirectXMaterial::Reload(ID3D11Device* pDevice)
{
	if (m_pEffect) m_pEffect->Release();
	if (m_pInputLayout) m_pInputLayout->Release();

	m_pEffect = LoadEffect(pDevice, m_AssetFile);

	// Save the technique of the effect as a member variable
	m_pTechnique = m_pEffect->GetTechniqueByName("DefaultTechnique");
	if (!m_pTechnique->IsValid()) return;

	// Save the worldviewprojection and world variable of the effect as a member variable
	m_pMatWorldViewProjVariable = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
	m_pMatWorldVariable = m_pEffect->GetVariableByName("gWorld")->AsMatrix();

	for (const auto& texturePair : m_pTextures)
	{
		SetTexture(texturePair.first, texturePair.second);
	}

	m_pInputLayout = LoadInputLayout(pDevice);

	// Set all material variables again
	for (const auto& nameVariablePair : m_MaterialVariables)
	{
		auto var = m_pEffect->GetVariableByName(nameVariablePair.first.c_str());

		const MaterialVariable varData{ nameVariablePair.second };
		var->SetRawValue(&varData.data, 0, varData.byteCount);
	}
}

std::unique_ptr<leap::graphics::DirectXMaterial> leap::graphics::DirectXMaterial::Clone(ID3D11Device* pDevice)
{
	auto pMaterial{ std::make_unique<DirectXMaterial>(pDevice, m_AssetFile, m_VertexDataFunction) };

	for (const auto& texturePair : m_pTextures)
	{
		pMaterial->SetTexture(texturePair.first, texturePair.second);
	}

	return std::move(pMaterial);
}

ID3DX11Effect* leap::graphics::DirectXMaterial::LoadEffect(ID3D11Device* pDevice, const std::string& assetFile)
{
	HRESULT result;
	ID3D10Blob* pErrorBlob{ nullptr };
	ID3DX11Effect* pEffect{};

	DWORD shaderFlags{ 0 };

#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	std::wstringstream wAssetFile{};
	wAssetFile << assetFile.c_str();

	// Load the effect form the file
	result = D3DX11CompileEffectFromFile
	(
		wAssetFile.str().c_str(),
		nullptr,
		nullptr,
		shaderFlags,
		0,
		pDevice,
		&pEffect,
		&pErrorBlob
	);

	// If loading the effect failed, print an error message
	if (FAILED(result))
	{
		if (pErrorBlob != nullptr)
		{
			const char* pErrors{ static_cast<char*>(pErrorBlob->GetBufferPointer()) };

			std::stringstream ss;
			ss << "DirectXEngine Error : ";
			for (unsigned int i{}; i < pErrorBlob->GetBufferSize(); ++i)
			{
				ss << pErrors[i];
			}

			OutputDebugString(ss.str().c_str());
			pErrorBlob->Release();
			pErrorBlob = nullptr;

			Debug::LogError(ss.str());
		}
		else
		{
			std::stringstream ss;
			ss << "DirectXEngine Error : EffectLoader failed to load effect file from path: " << assetFile;
			Debug::LogError(ss.str());
		}
	}

	return pEffect;
}
