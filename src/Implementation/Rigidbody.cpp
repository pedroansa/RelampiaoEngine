#include "Rigidbody.h"
#include "GameObject.h" // Necessário aqui para o compilador entender o tamanho do TransformComponent

namespace app {

    RigidbodyComponent::RigidbodyComponent(TransformComponent& ownerTransform, float mass, bool useGravity)
        : transform{ ownerTransform }, mass{ mass > 0.0f ? mass : 0.001f }, useGravity{ useGravity } {}

    void RigidbodyComponent::addForce(const glm::vec3& force) {
        // F = m * a  ->  a = F / m
        acceleration += force / mass;
    }

    void RigidbodyComponent::update(float deltaTime) {
        // 1. Se usar gravidade, acumula na velocidade
        if (useGravity) {
            velocity += gravityConst * deltaTime;
        }

        // 2. Acumula as outras forças/acelerações na velocidade (v = v0 + a * t)
        velocity += acceleration * deltaTime;

        // 3. Move a posição real do GameObject através da nossa referência (s = s0 + v * t)
        transform.translation += velocity * deltaTime;

        // 4. Reseta a aceleração para o próximo frame
        acceleration = glm::vec3(0.0f);
    }

} // namespace app