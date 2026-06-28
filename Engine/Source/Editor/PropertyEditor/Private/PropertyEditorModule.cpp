
#include "Modules/IModuleInterface.hpp"
#include <iostream>

class PropertyEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        std::cout << "PropertyEditorModule started\n";
    }

    virtual void ShutdownModule() override
    {
        std::cout << "PropertyEditorModule shutdown\n";
    }
};

IMPLEMENT_MODULE(PropertyEditorModule, WindEffects_PropertyEditor)
