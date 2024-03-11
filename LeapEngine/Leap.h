#pragma once

#include <functional>

class GLFWwindow;

namespace leap
{
	class GameContext;
	class SceneManager;

	namespace input
	{
		class InputManager;
	}

	namespace graphics
	{
		class IRenderer;
		class DirectXMeshLoader;
		class DirectXDefaults;
	}

	class LeapEngine final
	{
	public:
		explicit LeapEngine(int width, int height, const char* title);
		~LeapEngine();
		LeapEngine(const LeapEngine& other) = delete;
		LeapEngine(LeapEngine&& other) = delete;
		LeapEngine& operator=(const LeapEngine& other) = delete;
		LeapEngine& operator=(LeapEngine&& other) = delete;

		void Run(const std::function<void()>& afterInitialize, int desiredFPS);

		// [TBD]: Should these be free functions?
		static GameContext& GetGameContext();
		static input::InputManager& GetInputManager();
		static SceneManager& GetSceneManager();
		static graphics::DirectXMeshLoader& GetDirectXMeshLoader(); // Why is this even a singleton to begin with?
		static graphics::DirectXDefaults& GetDirectXDefaults();

	private:
		GLFWwindow* m_pWindow{};
	};
}