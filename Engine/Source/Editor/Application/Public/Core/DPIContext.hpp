#pragma once

namespace we::UI {

class DPIContext {
public:
    static float GetScale() { return s_Scale; }
    static void SetScale(float scale) { s_Scale = scale; }

    static float Scale(float value) { return value * s_Scale; }

private:
    static inline float s_Scale = 1.0f; // Default 100%
};

} // namespace we::editor::application::UI
