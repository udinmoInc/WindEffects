
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class ContentBrowserModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "ContentBrowserModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "ContentBrowserModule shutdown\n";
    }
};

IMPLEMENT_MODULE(ContentBrowserModule, WindEffects_ContentBrowser)
