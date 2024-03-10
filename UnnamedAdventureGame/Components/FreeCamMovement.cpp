#include "FreeCamMovement.h"
#include "InputManager.h"
#include "Keyboard.h"
#include "LambdaCommand.h"
#include "Mouse.h"
#include "Components/Transform/Transform.h"
#include <Leap.h>
#include "GameContext/GameContext.h"
#include "GameContext/Timer.h"
#include "SceneGraph/GameObject.h"

void unag::FreeCamMovement::Awake()
{
	const auto newParent = GetGameObject()->GetParent()->CreateChild("Camera");
	newParent->GetTransform()->SetWorldPosition(GetGameObject()->GetTransform()->GetWorldPosition());
	GetGameObject()->SetParent(newParent);
}

void unag::FreeCamMovement::Start()
{
	const auto& input = leap::LeapEngine::GetInputManager();
	const auto keyboard = input.GetKeyboard();
	const auto mouse = input.GetMouse();

	// Link keyboard input
	auto command = std::make_unique<leap::LambdaCommand>([=]()
		{
			GetGameObject()->GetParent()->GetTransform()->Translate(-GetTransform()->GetRight() * 100.0f * leap::LeapEngine::GetGameContext().GetTimer()->GetDeltaTime());
		});
	keyboard->AddCommand(command.get(), leap::input::InputManager::InputType::EventRepeat, leap::input::Keyboard::Key::KeyA);
	m_Commands.push_back(std::move(command));

	command = std::make_unique<leap::LambdaCommand>([=]()
		{
			GetGameObject()->GetParent()->GetTransform()->Translate(GetTransform()->GetRight() * 100.0f * leap::LeapEngine::GetGameContext().GetTimer()->GetDeltaTime());
		});
	keyboard->AddCommand(command.get(), leap::input::InputManager::InputType::EventRepeat, leap::input::Keyboard::Key::KeyD);
	m_Commands.push_back(std::move(command));

	command = std::make_unique<leap::LambdaCommand>([=]()
		{
			GetGameObject()->GetParent()->GetTransform()->Translate(GetTransform()->GetForward() * 100.0f * leap::LeapEngine::GetGameContext().GetTimer()->GetDeltaTime());
		});
	keyboard->AddCommand(command.get(), leap::input::InputManager::InputType::EventRepeat, leap::input::Keyboard::Key::KeyW);
	m_Commands.push_back(std::move(command));

	command = std::make_unique<leap::LambdaCommand>([=]()
		{
			GetGameObject()->GetParent()->GetTransform()->Translate(-GetTransform()->GetForward() * 100.0f * leap::LeapEngine::GetGameContext().GetTimer()->GetDeltaTime());
		});
	keyboard->AddCommand(command.get(), leap::input::InputManager::InputType::EventRepeat, leap::input::Keyboard::Key::KeyS);
	m_Commands.push_back(std::move(command));

	// Link mouse input
	command = std::make_unique<leap::LambdaCommand>([=]()
		{
			const auto mouseDelta = static_cast<glm::vec2>(leap::LeapEngine::GetInputManager().GetMouse()->GetDelta());
			const auto deltaTime = leap::LeapEngine::GetGameContext().GetTimer()->GetDeltaTime();
			constexpr float mouseSpeed = 15.f;

			GetTransform()->Rotate(mouseDelta.y * deltaTime * mouseSpeed, 0.0f, 0.0f);
			GetGameObject()->GetParent()->GetTransform()->Rotate(0.0f, mouseDelta.x * deltaTime * mouseSpeed, 0.0f);
		});
	mouse->AddCommand(command.get(), leap::input::InputManager::InputType::EventRepeat, leap::input::Mouse::Button::LeftButton);
	m_Commands.push_back(std::move(command)); 
}

void unag::FreeCamMovement::OnDestroy()
{
	const auto& input = leap::LeapEngine::GetInputManager();
	for (auto& command : m_Commands)
	{
		input.GetKeyboard()->RemoveCommand(command.get());
		input.GetMouse()->RemoveCommand(command.get());
	}
}