#include "AudioTester.h"

#include <LambdaCommand.h>

#include <InputManager.h>
#include <Keyboard.h>

#include <SceneGraph/GameObject.h>
#include <Components/Audio/AudioSource.h>

#include <Leap.h>

void unag::AudioTester::Awake()
{
	auto pCommand{ std::make_unique<leap::LambdaCommand>([this]() { GetGameObject()->GetComponent<leap::AudioSource>()->Mute(); }) };
	leap::LeapEngine::GetInputManager().GetKeyboard()->AddCommand(
		pCommand.get(), 
		leap::input::InputManager::InputType::EventPress, 
		leap::input::Keyboard::Key::KeyM
	);
	m_pCommands.emplace_back(std::move(pCommand));

	pCommand = std::make_unique<leap::LambdaCommand>([this]() { GetGameObject()->GetComponent<leap::AudioSource>()->Unmute(); });
	leap::LeapEngine::GetInputManager().GetKeyboard()->AddCommand(
		pCommand.get(),
		leap::input::InputManager::InputType::EventPress,
		leap::input::Keyboard::Key::KeyU
	);
	m_pCommands.emplace_back(std::move(pCommand));

	pCommand = std::make_unique<leap::LambdaCommand>([this]() { GetGameObject()->GetComponent<leap::AudioSource>()->Pause(); });
	leap::LeapEngine::GetInputManager().GetKeyboard()->AddCommand(
		pCommand.get(),
		leap::input::InputManager::InputType::EventPress,
		leap::input::Keyboard::Key::KeyP
	);
	m_pCommands.emplace_back(std::move(pCommand));

	pCommand = std::make_unique<leap::LambdaCommand>([this]() { GetGameObject()->GetComponent<leap::AudioSource>()->Resume(); });
	leap::LeapEngine::GetInputManager().GetKeyboard()->AddCommand(
		pCommand.get(),
		leap::input::InputManager::InputType::EventPress,
		leap::input::Keyboard::Key::KeyR
	);
	m_pCommands.emplace_back(std::move(pCommand));

	pCommand = std::make_unique<leap::LambdaCommand>([this]() { GetGameObject()->GetComponent<leap::AudioSource>()->Play(); });
	leap::LeapEngine::GetInputManager().GetKeyboard()->AddCommand(
		pCommand.get(),
		leap::input::InputManager::InputType::EventPress,
		leap::input::Keyboard::Key::KeyN
	);
	m_pCommands.emplace_back(std::move(pCommand));
}

void unag::AudioTester::OnDestroy()
{
	for (const auto& pCommand : m_pCommands)
	{
		leap::LeapEngine::GetInputManager().GetKeyboard()->RemoveCommand(pCommand.get());
	}
}
