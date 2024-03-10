#pragma once

#include <functional>

class GLFWwindow;

namespace leap
{
	class GameContext;

	namespace graphics
	{
		class IRenderer;
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

		// [TBD]: Should this be a free function?
		static GameContext& GetGameContext();

	private:
		GLFWwindow* m_pWindow{};
	};
}