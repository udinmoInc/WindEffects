#pragma once

#include <string>
#include <vector>
#include <volk.h>
#include <functional>

namespace HouseEngine::UI {

struct ContentItem {
    std::string id;
    std::string name;
    std::string type; // "folder", "mesh", "material", "texture", etc.
    std::string path; // Full asset path
    std::string iconName; // Fallback font icon name
    VkDescriptorSet iconTexture = VK_NULL_HANDLE; // Crisp SVG texture
    bool isFolder = false;
    bool isFavorite = false;
    bool thumbnailRequested = false;
    void* userData = nullptr;
};

enum class ContentViewMode {
    Grid,
    List
};

class ContentBrowserModel {
public:
    std::vector<ContentItem> items;
    std::vector<std::string> selectedIds;
    std::string filterText;
    ContentViewMode viewMode = ContentViewMode::Grid;

    // Optional event delegates to notify the View when model changes
    std::function<void()> onModelChanged;

    void NotifyChanged() {
        if (onModelChanged) {
            onModelChanged();
        }
    }
};

} // namespace HouseEngine::UI
