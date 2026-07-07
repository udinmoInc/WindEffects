#include "Scene/Entity.h"

#if WE_HAS_GLM
#include <glm/gtc/matrix_transform.hpp>
#endif

namespace we::runtime::scene {

#if WE_HAS_GLM

glm::mat4 Entity::GetModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, Position);
    model = glm::rotate(model, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, Scale);
    return model;
}

#else // !WE_HAS_GLM

glm::mat4 Entity::GetModelMatrix() const {
    glm::mat4 result{};
    // Identity matrix stub
    result.data[0] = 1.0f;
    result.data[5] = 1.0f;
    result.data[10] = 1.0f;
    result.data[15] = 1.0f;
    return result;
}

#endif // WE_HAS_GLM

} // namespace we::runtime::scene
