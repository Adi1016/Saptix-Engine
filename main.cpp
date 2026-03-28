#include "Engine/Engine.h"
#include "Engine/HubApp.h"
#include <iostream>

#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

int main()
{
    HubApp hub;
    if (hub.Init())
    {
        hub.Run();
    }
    hub.Shutdown();

    if (hub.DidSelectProject())
    {
        std::string targetProject = hub.GetSelectedProject();

        Engine engine;
        if (!engine.Init(targetProject))
        {
            std::cout << "Engine failed to initialize with project: " << targetProject << "\n";
            return -1;
        }

        engine.Run();
        engine.Shutdown();
    }

    return 0;
}   