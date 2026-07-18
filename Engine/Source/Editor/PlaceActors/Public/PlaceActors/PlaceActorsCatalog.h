#pragma once

#include "PlaceActors/Export.h"

#include "PlaceActors/PlaceActorsTypes.h"
#include <string>
#include <vector>

namespace we::programs::editor {

class PLACEACTORS_API PlaceActorsCatalog {
public:
    static PlaceActorsCatalog& Get();

    void Refresh();

    [[nodiscard]] const std::vector<PlaceActorsCategoryData>& GetCategories() const { return m_Categories; }
    [[nodiscard]] std::vector<PlaceActorsItemData> GetAllItems() const;
    [[nodiscard]] std::vector<std::string> GetCategoryFilterLabels() const;
    [[nodiscard]] const PlaceActorsItemData* FindItem(const std::string& toolId) const;

private:
    PlaceActorsCatalog() = default;

    void BuildFromRegistry();

    std::vector<PlaceActorsCategoryData> m_Categories;
    bool m_Built = false;
};

} // namespace we::programs::editor
