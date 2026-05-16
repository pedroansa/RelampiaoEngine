#pragma once
#include <glm/glm.hpp>

namespace app {

    struct TransformComponent;

    class RigidbodyComponent {
    public:
        RigidbodyComponent(TransformComponent& ownerTransform, float mass = 1.0f, bool useGravity = true);
        ~RigidbodyComponent() = default;

        RigidbodyComponent(const RigidbodyComponent&) = delete;
        RigidbodyComponent& operator=(const RigidbodyComponent&) = delete;
        RigidbodyComponent(RigidbodyComponent&&) = default;
        RigidbodyComponent& operator=(RigidbodyComponent&&) = default;

        void update(float deltaTime);

        // Agora a força precisa saber ONDE bateu no objeto (em coordenadas do mundo)
        void addForceAtPosition(const glm::vec3& force, const glm::vec3& worldPoint);

        // Mantemos o addForce antigo para empurrões puramente centrais (sem girar)
        void addForce(const glm::vec3& force);

        void setVelocity(const glm::vec3& newVelocity) { velocity = newVelocity; }
        glm::vec3 getVelocity() const { return velocity; }

        void setAngularVelocity(const glm::vec3& newAngVel) { angularVelocity = newAngVel; }
        glm::vec3 getAngularVelocity() const { return angularVelocity; }

        void setMass(float newMass);
        float getMass() const { return mass; }

        bool useGravity;

    private:
        TransformComponent& transform;

        // Física Linear (Movimento)
        glm::vec3 velocity{ 0.0f };
        glm::vec3 acceleration{ 0.0f };
        float mass;

        // Física Angular (Rotação)
        glm::vec3 angularVelocity{ 0.0f }; // Velocidade de rotação em rad/s nos eixos X, Y, Z
        glm::vec3 torque{ 0.0f };          // Acumulador de forças de rotação do frame
        float inertiaMoment;               // Resistência rotacional (escalar simplificado)

        const glm::vec3 gravityConst{ 0.0f, 0.1f, 0.0f };
    };

} // namespace app