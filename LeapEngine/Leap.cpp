#include "Leap.h"

#include "InputManagerGLFW3.h"
#include "Renderer.h"

#include "ServiceLocator.h"
#include "Systems/FmodAudioSystem.h"

#include <iostream>
#include <glfw3.h>

leap::LeapEngine::LeapEngine()
{
    std::cout << "Engine created\n";

    /* Initialize the library */
    if (!glfwInit())
        return;

    /* Create a windowed mode window and its OpenGL context */
    m_pWindow = glfwCreateWindow(640, 480, "Hello World", nullptr, nullptr);
    if (!m_pWindow)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create window");
    }

    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize glfw");
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(m_pWindow);
}

leap::LeapEngine::~LeapEngine()
{
    delete m_pRenderer;
}

void leap::LeapEngine::Run()
{
	std::cout << "Engine startup\n";
    InputManagerGLFW3 input{m_pWindow};

    m_pRenderer = new Renderer(m_pWindow);
    m_pRenderer->Initialize();

    ServiceLocator::RegisterAudioSystem<leap::audio::FmodAudioSystem>();
    auto& audio{ ServiceLocator::GetAudio() };

    const int soundId{ audio.LoadSound("Data/Sound.wav") };
    audio.PlaySound2D(soundId, 1.0f, {});

    while (!glfwWindowShouldClose(m_pWindow))
    {
        /* Poll for and process events */
        glfwPollEvents();
        input.ProcessInput();

        /* Render here */
        glClearColor(0.2f, 0.7f, 0.5f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT);

        /* Swap front and back buffers */
        glfwSwapBuffers(m_pWindow);

        /* Update audio system */
        audio.Update();
    }

    glfwTerminate();
}
