#include "CameraComponent.h"

#include <Camera.h>
#include <Interfaces/IRenderer.h>
#include <glfw3.h>

#include "../../ServiceLocator/ServiceLocator.h"
#include "../Transform/Transform.h"
#include "../../GameContext/GameContext.h"
#include "../../GameContext/Window.h"
#include "../LeapEngine/Leap.h"

#include <functional>

#include <Debug.h>

leap::CameraComponent::CameraComponent()
{
	const auto window = LeapEngine::GetGameContext().GetWindow();
	window->AddListener(this);
	const auto& size = window->GetWindowSize();
	constexpr float fov = 90.f;
	m_pCamera = std::make_unique<graphics::Camera>(static_cast<float>(size.x), static_cast<float>(size.y), fov);
}

leap::CameraComponent::~CameraComponent()
{
}

void leap::CameraComponent::Notify(const glm::ivec2& data)
{
	m_pCamera->SetAspectRatio(data);
}

void leap::CameraComponent::Awake()
{
	GetTransform()->OnPositionChanged.AddListener(this);
	GetTransform()->OnRotationChanged.AddListener(this);

	UpdateTransform();
}

void leap::CameraComponent::OnDestroy()
{
	LeapEngine::GetGameContext().GetWindow()->RemoveListener(this);

	GetTransform()->OnPositionChanged.RemoveListener(this);
	GetTransform()->OnRotationChanged.RemoveListener(this);
}

void leap::CameraComponent::Notify()
{
	UpdateTransform();
}

void leap::CameraComponent::UpdateTransform() const
{
	// Only update the internal camera data if the camera is currently active
	if (ServiceLocator::GetRenderer().GetCamera() != m_pCamera.get()) return;

	// Update internal camera data with transform
	Transform* pTransform{ GetTransform() };
	m_pCamera->SetTransform(
		{
			pTransform->GetRight(),
			pTransform->GetUp(),
			pTransform->GetForward(),
			pTransform->GetWorldPosition()
		});
}

void leap::CameraComponent::SetAsActiveCamera(bool active) const
{
	graphics::IRenderer& renderer{ ServiceLocator::GetRenderer() };

	if (active)
	{
		renderer.SetActiveCamera(m_pCamera.get());
	}
	else
	{
		if (renderer.GetCamera() == m_pCamera.get()) renderer.SetActiveCamera(nullptr);
	}
}
