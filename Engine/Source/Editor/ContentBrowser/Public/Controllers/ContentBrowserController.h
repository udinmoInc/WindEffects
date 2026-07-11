#pragma once

#include "Models/ContentBrowserModel.h"
#include <memory>
#include <algorithm>

namespace WindEffects::Editor::UI {

class ContentBrowserController {
public:
    ContentBrowserController(std::shared_ptr<ContentBrowserModel> model) : m_Model(model) {}

    void AddItem(const ContentItem& item) {
        if (!m_Model) return;
        m_Model->items.push_back(item);
        m_Model->NotifyChanged();
    }

    void RemoveItem(const std::string& id) {
        if (!m_Model) return;
        auto& items = m_Model->items;
        items.erase(std::remove_if(items.begin(), items.end(), 
            [&](const ContentItem& item) { return item.id == id; }), items.end());
        
        RemoveFromSelection(id);
        m_Model->NotifyChanged();
    }

    void Clear() {
        if (!m_Model) return;
        m_Model->items.clear();
        m_Model->selectedIds.clear();
        m_Model->NotifyChanged();
    }

    void SetSelectedId(const std::string& id) {
        if (!m_Model) return;
        m_Model->selectedIds = {id};
        m_Model->NotifyChanged();
    }

    void AddToSelection(const std::string& id) {
        if (!m_Model) return;
        if (std::find(m_Model->selectedIds.begin(), m_Model->selectedIds.end(), id) == m_Model->selectedIds.end()) {
            m_Model->selectedIds.push_back(id);
            m_Model->NotifyChanged();
        }
    }

    void RemoveFromSelection(const std::string& id) {
        if (!m_Model) return;
        auto& sel = m_Model->selectedIds;
        sel.erase(std::remove(sel.begin(), sel.end(), id), sel.end());
        m_Model->NotifyChanged();
    }

    void ClearSelection() {
        if (!m_Model) return;
        m_Model->selectedIds.clear();
        m_Model->NotifyChanged();
    }

    void SetFilterText(const std::string& filter) {
        if (!m_Model) return;
        m_Model->filterText = filter;
        m_Model->NotifyChanged();
    }

    void SetViewMode(ContentViewMode mode) {
        if (!m_Model) return;
        m_Model->viewMode = mode;
        m_Model->NotifyChanged();
    }

    void UpdateItemIcon(const std::string& id, VkDescriptorSet textureId) {
        if (!m_Model) return;
        bool changed = false;
        for (auto& item : m_Model->items) {
            if (item.id == id) {
                item.iconTexture = textureId;
                changed = true;
                break;
            }
        }
        if (changed) {
            m_Model->NotifyChanged();
        }
    }

private:
    std::shared_ptr<ContentBrowserModel> m_Model;
};

} // namespace we::editor::contentbrowser::UI
