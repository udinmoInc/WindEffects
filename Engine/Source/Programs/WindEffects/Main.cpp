#include "EditorApplication.hpp"
#include "Core/Logger.hpp"
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    HouseEngine::Logger::Init();
    
    try {
        HE_INFO("WindEffects Engine bootstrapping...");
        HouseEngine::EditorApplication app;
        app.Initialize();
        
        // Let the legacy editor run its own loop for now
        // Eventually we'll call app.Run() once the editor is fully refactored
        // But since we didn't remove the loop in Editor, we have to either remove it or just call it here.
        // Wait, the plan says: "Refactor Main.cpp to initialize the application and be under 50 lines."
        // We can just call app.Run() if we extract the loop!
        app.Run();
    } catch (const std::exception& e) {
        HouseEngine::Logger::ReportError(
            "Engine Crash",
            std::string("A fatal exception occurred: ") + e.what(),
            true
        );
        return -1;
    }
    
    HouseEngine::Logger::Shutdown();
    return 0;
}
