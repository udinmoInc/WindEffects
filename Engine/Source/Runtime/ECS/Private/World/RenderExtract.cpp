#include "ECS/RenderExtract.h"

#include "ECS/Components/CoreComponents.h"
#include "ECS/World.h"

#include <cstring>

namespace we::runtime::ecs {

namespace {

void CopyMatrix(float dst[16], const glm::mat4& m) {
    std::memcpy(dst, &m[0][0], sizeof(float) * 16);
}

void CopyVec3(float dst[3], const glm::vec3& v) {
    dst[0] = v.x;
    dst[1] = v.y;
    dst[2] = v.z;
}

void CopyVec4(float dst[4], const glm::vec4& v) {
    dst[0] = v.x;
    dst[1] = v.y;
    dst[2] = v.z;
    dst[3] = v.w;
}

} // namespace

void ExtractRenderFrame(const World& world, ExtractedFrameData& out) {
    out.Clear();

    // Opaque meshes
    const_cast<World&>(world).QueryAll<TransformComponent, StaticMeshComponent, MaterialComponent, VisibilityComponent>()
        .Each([&](Entity e,
                  TransformComponent& t,
                  StaticMeshComponent& mesh,
                  MaterialComponent& mat,
                  VisibilityComponent& vis) {
            if (!vis.visible || !world.IsEntityEnabled(e)) {
                return;
            }
            ExtractedMeshDraw draw{};
            draw.entityId = e.id;
            CopyMatrix(draw.worldMatrix, t.worldMatrix);
            draw.meshAssetId = mesh.meshAssetId;
            draw.materialAssetId = mat.materialAssetId;
            CopyVec4(draw.color, mat.color);
            draw.castShadows = vis.castShadows;
            draw.receiveShadows = vis.receiveShadows;
            out.meshes.push_back(draw);
        });

    // Directional lights
    const_cast<World&>(world).QueryAll<DirectionalLightComponent, TransformComponent>()
        .Each([&](Entity e, DirectionalLightComponent& light, TransformComponent&) {
            if (!world.IsEntityEnabled(e)) {
                return;
            }
            ExtractedDirectionalLight item{};
            item.entityId = e.id;
            CopyVec3(item.direction, light.direction);
            CopyVec3(item.color, light.color);
            item.intensity = light.intensity;
            item.castShadows = light.castShadows;
            out.directionalLights.push_back(item);
        });

    // Point lights
    const_cast<World&>(world).QueryAll<PointLightComponent, TransformComponent>()
        .Each([&](Entity e, PointLightComponent& light, TransformComponent& t) {
            if (!world.IsEntityEnabled(e)) {
                return;
            }
            ExtractedPointLight item{};
            item.entityId = e.id;
            CopyVec3(item.position, t.localPosition);
            // Prefer world translation when available.
            item.position[0] = t.worldMatrix[3][0];
            item.position[1] = t.worldMatrix[3][1];
            item.position[2] = t.worldMatrix[3][2];
            CopyVec3(item.color, light.color);
            item.intensity = light.intensity;
            item.range = light.range;
            out.pointLights.push_back(item);
        });

    // Spot lights
    const_cast<World&>(world).QueryAll<SpotLightComponent, TransformComponent>()
        .Each([&](Entity e, SpotLightComponent& light, TransformComponent& t) {
            if (!world.IsEntityEnabled(e)) {
                return;
            }
            ExtractedSpotLight item{};
            item.entityId = e.id;
            item.position[0] = t.worldMatrix[3][0];
            item.position[1] = t.worldMatrix[3][1];
            item.position[2] = t.worldMatrix[3][2];
            CopyVec3(item.direction, light.direction);
            CopyVec3(item.color, light.color);
            item.intensity = light.intensity;
            item.range = light.range;
            item.innerConeDegrees = light.innerConeDegrees;
            item.outerConeDegrees = light.outerConeDegrees;
            out.spotLights.push_back(item);
        });

    // Cameras
    const_cast<World&>(world).QueryAll<CameraComponent, TransformComponent>()
        .Each([&](Entity e, CameraComponent& cam, TransformComponent& t) {
            if (!world.IsEntityEnabled(e)) {
                return;
            }
            ExtractedCamera item{};
            item.entityId = e.id;
            CopyMatrix(item.worldMatrix, t.worldMatrix);
            item.fieldOfViewDegrees = cam.fieldOfViewDegrees;
            item.nearPlane = cam.nearPlane;
            item.farPlane = cam.farPlane;
            item.primary = cam.primary;
            out.cameras.push_back(item);
        });

    // Sky atmosphere
    const_cast<World&>(world).QueryAll<SkyAtmosphereComponent>()
        .Each([&](Entity e, SkyAtmosphereComponent& sky) {
            ExtractedSkyAtmosphere item{};
            item.entityId = e.id;
            item.enabled = sky.enabled;
            item.mieScale = sky.mieScale;
            item.rayleighScale = sky.rayleighScale;
            out.skyAtmospheres.push_back(item);
        });

    // Volumetric clouds
    const_cast<World&>(world).QueryAll<VolumetricCloudComponent>()
        .Each([&](Entity e, VolumetricCloudComponent& clouds) {
            ExtractedVolumetricCloud item{};
            item.entityId = e.id;
            item.enabled = clouds.enabled;
            item.coverage = clouds.coverage;
            item.density = clouds.density;
            item.bottomAltitude = clouds.bottomAltitude;
            item.topAltitude = clouds.topAltitude;
            out.volumetricClouds.push_back(item);
        });

    // Terrain
    const_cast<World&>(world).QueryAll<TerrainComponent, TransformComponent>()
        .Each([&](Entity e, TerrainComponent& terrain, TransformComponent& t) {
            ExtractedTerrain item{};
            item.entityId = e.id;
            item.enabled = terrain.enabled;
            item.landscapeId = terrain.landscapeId;
            item.resolutionX = terrain.resolutionX;
            item.resolutionY = terrain.resolutionY;
            CopyMatrix(item.worldMatrix, t.worldMatrix);
            out.terrains.push_back(item);
        });

    // Water
    const_cast<World&>(world).QueryAll<WaterComponent, TransformComponent>()
        .Each([&](Entity e, WaterComponent& water, TransformComponent&) {
            ExtractedWater item{};
            item.entityId = e.id;
            item.enabled = water.enabled;
            item.level = water.level;
            out.waters.push_back(item);
        });
}

} // namespace we::runtime::ecs
