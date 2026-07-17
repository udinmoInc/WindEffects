#pragma once

#include "WindEffects/Runtime/UI/Export.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace WindEffects::Editor::UI {

/// Lightweight observable value. Listeners are notified on Set().
template <typename T>
class Observable {
public:
    using Listener = std::function<void(const T&)>;

    Observable() = default;
    explicit Observable(T value) : m_Value(std::move(value)) {}

    const T& Get() const { return m_Value; }
    operator const T&() const { return m_Value; }

    void Set(T value) {
        if (m_Value == value) return;
        m_Value = std::move(value);
        Notify();
    }

    Observable& operator=(T value) {
        Set(std::move(value));
        return *this;
    }

    [[nodiscard]] size_t Subscribe(Listener listener) {
        m_Listeners.push_back(std::move(listener));
        return m_Listeners.size() - 1;
    }

    void Unsubscribe(size_t index) {
        if (index < m_Listeners.size()) {
            m_Listeners[index] = nullptr;
        }
    }

private:
    void Notify() {
        for (auto& listener : m_Listeners) {
            if (listener) listener(m_Value);
        }
    }

    T m_Value{};
    std::vector<Listener> m_Listeners;
};

template <typename T>
class ObservableList {
public:
    using Listener = std::function<void()>;

    const std::vector<T>& Items() const { return m_Items; }

    void Set(std::vector<T> items) {
        m_Items = std::move(items);
        Notify();
    }

    void Clear() {
        m_Items.clear();
        Notify();
    }

    void Push(T item) {
        m_Items.push_back(std::move(item));
        Notify();
    }

    [[nodiscard]] size_t Subscribe(Listener listener) {
        m_Listeners.push_back(std::move(listener));
        return m_Listeners.size() - 1;
    }

private:
    void Notify() {
        for (auto& listener : m_Listeners) {
            if (listener) listener();
        }
    }

    std::vector<T> m_Items;
    std::vector<Listener> m_Listeners;
};

/// Bind an observable to a widget property updater; returns unsubscribe index.
template <typename T, typename WidgetT, typename Setter>
void Bind(Observable<T>& source, const std::shared_ptr<WidgetT>& widget, Setter setter) {
    if (!widget) return;
    setter(*widget, source.Get());
    source.Subscribe([weak = std::weak_ptr<WidgetT>(widget), setter](const T& value) {
        if (auto locked = weak.lock()) {
            setter(*locked, value);
            locked->InvalidatePaint();
        }
    });
}

} // namespace WindEffects::Editor::UI
