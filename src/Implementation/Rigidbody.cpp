#include "RigidBody.h"
#include "GameObject.h" // Garanta que aponta para o seu TransformComponent

namespace app {

    RigidbodyComponent::RigidbodyComponent(TransformComponent& ownerTransform, float mass, bool useGravity)
        : transform(ownerTransform), useGravity(useGravity) {
        setMass(mass);
    }

    void RigidbodyComponent::setMass(float newMass) {
        mass = newMass > 0.0f ? newMass : 0.001f;

        // Simplificação: Consideramos o objeto uma esfera sólida para o cálculo de inércia.
        // Formula aproximada: I = 2/5 * m * r^2. Vamos assumir um raio médio de 1.0f aqui.
        inertiaMoment = 0.4f * mass * 1.0f;
    }

    void RigidbodyComponent::addForce(const glm::vec3& force) {
        // Força aplicada exatamente no centro de massa: gera apenas translação
        acceleration += force / mass;
    }

    void RigidbodyComponent::addForceAtPosition(const glm::vec3& force, const glm::vec3& worldPoint) {
        // 1. Efeito Linear (Move o objeto)
        acceleration += force / mass;

        // 2. Efeito Angular (Gira o objeto)
        // Vetor r vai do centro do objeto até onde a força foi aplicada
        glm::vec3 r = worldPoint - transform.translation;

        // Torque = r x F (Produto vetorial gera um eixo perpendicular ao empurrão)
        torque += glm::cross(r, force);
    }

    void RigidbodyComponent::update(float deltaTime) {
        if (deltaTime <= 0.0f) return;

        // --- 1. Atualização Linear (Posição) ---
        if (useGravity) {
            acceleration += gravityConst;
        }
        velocity += acceleration * deltaTime;
        transform.translation += velocity * deltaTime;
        acceleration = glm::vec3(0.0f); // Reseta para o próximo frame

        // --- 2. Atualização Angular (Rotação) ---
        // Aceleração Angular = Torque / Momento de Inércia
        glm::vec3 angularAcceleration = torque / inertiaMoment;

        angularVelocity += angularAcceleration * deltaTime;

        // Aplica a rotação acumulada diretamente nos ângulos de Euler do seu Transform
        transform.rotation += angularVelocity * deltaTime;

        torque = glm::vec3(0.0f); // Reseta o torque para o próximo frame
    }

} // namespace app