#pragma once

// glm
#include <glm/glm.hpp>

namespace app {

    // Forward Declaration (Avisa o compilador que essa struct existe sem puxar o GameObject.h inteiro)
    struct TransformComponent;

    class RigidbodyComponent {
    public:
        // O construtor OBRIGA a passar o transform que este componente vai controlar
        RigidbodyComponent(TransformComponent& ownerTransform, float mass = 1.0f, bool useGravity = true);
        ~RigidbodyComponent() = default;

        // Desabilita cópias para proteger a referência do Transform
        RigidbodyComponent(const RigidbodyComponent&) = delete;
        RigidbodyComponent& operator=(const RigidbodyComponent&) = delete;
        RigidbodyComponent(RigidbodyComponent&&) = default;
        RigidbodyComponent& operator=(RigidbodyComponent&&) = default;

        // Executa a física do frame e altera a posição do transform diretamente
        void update(float deltaTime);

        // Adiciona uma força instantânea (pulo, empurrão, explosão)
        void addForce(const glm::vec3& force);

        // Getters e Setters
        void setVelocity(const glm::vec3& newVelocity) { velocity = newVelocity; }
        glm::vec3 getVelocity() const { return velocity; }

        void setMass(float newMass) { mass = newMass > 0.0f ? newMass : 0.001f; }
        float getMass() const { return mass; }

        bool useGravity;

    private:
        TransformComponent& transform; // Referência direta ao transform do GameObject dono

        glm::vec3 velocity{ 0.0f };      // Velocidade (m/s)
        glm::vec3 acceleration{ 0.0f };  // Aceleração acumulada no frame
        float mass;                    // Massa (kg)

        // Força da gravidade padrão (Y negativo puxa para baixo)
        const glm::vec3 gravityConst{ 0.0f, 1.81f, 0.0f };
    };

} // namespace app