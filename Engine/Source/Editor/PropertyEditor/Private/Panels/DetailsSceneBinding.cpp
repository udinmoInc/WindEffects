#include "PropertyEditor/DetailsSceneBinding.h"

#include "Environment/EnvironmentSystem.h"
#include "Environment/EnvironmentTypes.h"
#include "PropertyEditor/PropertyEditorTypes.h"
#include "Reflection/TypeId.h"
#include "Scene/Entity.h"
#include "KindUI/Core/WindIcon.h"

namespace we::editor::property {
namespace {

using we::runtime::kindui::kWindIconNone;
using we::runtime::kindui::WindIconRef;
using we::runtime::reflection::MakeTypeId;
using we::runtime::scene::Entity;
using we::runtime::scene::EntityType;
using we::runtime::world::environment::EnvironmentActorKind;
using we::runtime::world::environment::EnvironmentSystem;

namespace WindIcons = we::runtime::kindui::WindIcons;

WindIconRef IconForEntity(const Entity& entity) {
    switch (entity.Type) {
    case EntityType::DirectionalLight:
    case EntityType::SkyLight:
        return WindIcons::Sun16;
    case EntityType::SkyAtmosphere:
        return WindIcons::WorldGlobe16;
    case EntityType::VolumetricClouds:
    case EntityType::HeightFog:
        return WindIcons::Cloud16;
    case EntityType::Landscape:
        return WindIcons::Grid3x316;
    case EntityType::EmptyActor:
        return WindIcons::FolderClosed16;
    default:
        return kWindIconNone;
    }
}

void ResetCategoryFilter(IDetailsView& details) {
    details.SetActiveCategory("");
}

} // namespace

void PopulateDetailsFromSceneEntity(IDetailsView& details, Entity* entity) {
    if (!entity) {
        details.Clear();
        return;
    }

    ResetCategoryFilter(details);

    EnvironmentSystem& system = EnvironmentSystem::Get();
    system.SyncFromScene();

    std::vector<ObjectBinding> bindings;
    bindings.push_back({ MakeTypeId("we::runtime::scene::Entity"), entity });

    switch (system.GetActorKind(entity->Id)) {
    case EnvironmentActorKind::DirectionalLight:
        bindings.push_back({
            MakeTypeId("we::runtime::world::environment::EnvironmentDirectionalLight"),
            &system.GetSun() });
        break;
    case EnvironmentActorKind::SkyLight:
        bindings.push_back({
            MakeTypeId("we::runtime::world::environment::EnvironmentSkyLight"),
            &system.GetSkyLight() });
        break;
    case EnvironmentActorKind::SkyAtmosphere:
        bindings.push_back({
            MakeTypeId("we::runtime::world::environment::EnvironmentSkyAtmosphere"),
            &system.GetSkyAtmosphere() });
        break;
    case EnvironmentActorKind::HeightFog:
        bindings.push_back({
            MakeTypeId("we::runtime::world::environment::EnvironmentHeightFog"),
            &system.GetHeightFog() });
        break;
    case EnvironmentActorKind::VolumetricClouds:
        bindings.push_back({
            MakeTypeId("we::runtime::world::environment::EnvironmentVolumetricClouds"),
            &system.GetVolumetricClouds() });
        break;
    case EnvironmentActorKind::ExposureController:
        bindings.push_back({
            MakeTypeId("we::runtime::world::environment::EnvironmentExposureController"),
            &system.GetExposureController() });
        break;
    default:
        break;
    }

    details.SetBindings(bindings);
    details.SetObjectTitle(entity->Name);
    details.SetObjectIcon(IconForEntity(*entity));
}

} // namespace we::editor::property
