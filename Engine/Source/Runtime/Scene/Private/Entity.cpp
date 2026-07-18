#include "Scene/Entity.h"

#include "Core/Math/GlmInterop.h"

#include <glm/gtc/matrix_transform.hpp>

namespace we::runtime::scene {

we::math::Mat4 Entity::GetModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, we::math::ToGlm(Position));
    model = glm::rotate(model, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, we::math::ToGlm(Scale));
    return we::math::FromGlm(model);
}

} // namespace we::runtime::scene
