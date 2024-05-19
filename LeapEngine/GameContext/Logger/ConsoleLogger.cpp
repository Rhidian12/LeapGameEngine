#include "ConsoleLogger.h"
#include <iostream>

leap::ConsoleLogger::ConsoleLogger()
{
    SetEnabled(true);
}

leap::ConsoleLogger::~ConsoleLogger()
{
    Debug::pOnEvent->RemoveListener(this);
}

void leap::ConsoleLogger::SetEnabled(bool enable)
{
    if (m_Enabled == enable) return;

    if (enable)
    {
        Debug::pOnEvent->AddListener(this);
    }
    else
    {
        Debug::pOnEvent->RemoveListener(this);
    }
    m_Enabled = enable;
}

void leap::ConsoleLogger::Notify(const Debug::LogInfo& data)
{
    switch (data.Type)
    {
    case Debug::Type::Message:
        std::cout << BOLDWHITE;
        break;
    case Debug::Type::Warning:
        std::cout << BOLDYELLOW;
        break;
    case Debug::Type::Error:
        std::cout << BOLDRED;
        break;
    }

    std::cout << data.Time << ' ' << data.Message << '\n';
    std::cout << "** " << data.Location.file_name() << " line " << data.Location.line() << ":" << data.Location.column() << '\n' << RESET << '\n';
}
