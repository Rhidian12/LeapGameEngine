#pragma once
#include <functional>
#include <memory>
#include "Renderer.h"

class GLFWwindow;
class Renderer;

namespace leap
{
	class LeapEngine final
	{
	public:
		LeapEngine();
		~LeapEngine() = default;
		LeapEngine(const LeapEngine& other) = delete;
		LeapEngine(LeapEngine&& other) = delete;
		LeapEngine& operator=(const LeapEngine& other) = delete;
		LeapEngine& operator=(LeapEngine&& other) = delete;

		void Run(const std::function<void()>& load, int desiredFPS);

	private:
		GLFWwindow* m_pWindow;
		std::unique_ptr<leap::graphics::Renderer> m_pRenderer;
	};
}