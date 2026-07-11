#pragma once

#include <string>
#include <vector>
#include <volk.h>
#include <functional>

namespace WindEffects::Editor::UI {

struct ContentItem {
    std::string id;
    std::string name;
    std::string type;
    std::string path;
    std::string iconName;
    VkDescriptorSet iconTexture = VK_NULL_HANDLE;
    bool isFolder = false;
    bool isFavorite = false;
    bool isDirty = false;
    bool thumbnailRequested = false;
    void* userData = nullptr;
};

enum class ContentViewMode {
    LargeIcons,
    MediumIcons,
    SmallIcons,
    Tiles,
    List,
    Details,
    Columns = Details,
    Grid = LargeIcons
};

class ContentBrowserModel {
public:
    std::vector<ContentItem> items;
    std::vector<std::string> selectedIds;
    std::string filterText;
    std::string currentFolder = "/Game";
    ContentViewMode viewMode = ContentViewMode::LargeIcons;

    size_t assetCount = 0;
    size_t folderCount = 0;
    size_t memoryUsageBytes = 0;

    std::function<void()> onModelChanged;

    void NotifyChanged() {
        if (onModelChanged) {
            onModelChanged();
        }
    }
};

} // namespace WindEffects::Editor::UI
