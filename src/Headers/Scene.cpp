#include "Scene.h"

namespace app {
	void Scene::loadGameObjects(GameObject::Map& gameObjects, EngineDevice& engineDevice)
	{
		gameObjects.clear();
		std::shared_ptr<Model> model = Model::createModelFromFile(engineDevice, "models/smooth_vase.obj");
		auto flat_vase = std::make_unique<GameObject>(GameObject::createGameObject());
		flat_vase->model = model;
		flat_vase->transform.translation = { 0.0f, -10.0f, 0.0f };
		flat_vase->transform.scale = { 10.0f,10.0f, 10.0f };
		flat_vase->rigidbody = std::make_unique<app::RigidbodyComponent>(flat_vase->transform, 1.0f, true);
		flat_vase->texture = std::make_shared<Texture>(engineDevice, "../Models/bricks.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		
		gameObjects.emplace(flat_vase->getId(), std::move(flat_vase));

		model = Model::createModelFromFile(engineDevice, "models/quad.obj");
		auto floor = std::make_unique<GameObject>(GameObject::createGameObject());
		floor->model = model;
		floor->transform.translation = { 0.f, .5f, 0.f };
		floor->transform.scale = { 100.f, 100.f, 100.f };
		floor->texture = std::make_shared<Texture>(engineDevice, "../Models/chess.jpg", VK_SAMPLER_ADDRESS_MODE_REPEAT);
		floor->uvScale = 20.f;
		gameObjects.emplace(floor->getId(), std::move(floor));

		//auto sphere = Sphere::createSphere(engineDevice, 0.5f, true);
		//gameObjects.emplace(sphere.getId(), std::move(sphere));

		//Sphere sphere1 = Sphere::createSphere(2.0f, glm::vec3(1.0f, 0.0f, 0.0f)); // Red sphere with radius 2
		//gameObjects.emplace(sphere1.getId(), std::move(sphere1));

		std::vector<glm::vec3> lightColors{
		  {1.f, 1.f, 1.f},
		  {1.f, 1.f, 1.f},
		  {1.f, 1.f, 1.f},
		  {1.f, 1.f, 1.f},
		  {1.f, 1.f, 1.f},
		  {1.f, 1.f, 1.f}  //
		};

		for (int i = 0; i < lightColors.size(); i++) {
			auto pointLight = std::make_unique<GameObject>(GameObject::makePointLight(100.0f));
			pointLight->color = lightColors[i];

			auto rotateLight = glm::rotate(
				glm::mat4(1.f),
				(i * glm::two_pi<float>()) / lightColors.size(),
				{ 0.f, -1.f, 0.f });

			pointLight->transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
			gameObjects.emplace(pointLight->getId(), std::move(pointLight));
		}
	}
	void Scene::loadSolarSystem(GameObject::Map& gameObjects, EngineDevice& engineDevice)
	{
		gameObjects.clear();
		// 1. Configurações Globais
		float sunRadius = 20.0f;        // Sol um pouco maior para destaque
		float SIZE_SCALE = 2.0f;        // Escala global de tamanho
		float DIST_SCALE = 100.0f;       // Escala de distância entre órbitas
		float baseSpeed = glm::two_pi<float>() / 10.f; // 10 segundos por volta (0.628)

		// ========== SOL ==========
		auto sunMesh = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius, false));
		sunMesh->transform.translation = { 0.0f, 0.0f, 0.0f };
		sunMesh->color = { 1.0f, 0.9f, 0.3f };
		sunMesh->texture = std::make_shared<Texture>(engineDevice, "../Models/sun.jpg");
		baseSpeed / 25.0f;
		gameObjects.emplace(sunMesh->getId(), std::move(sunMesh));

		// Luz Centralizada
		auto sunLight = std::make_unique<GameObject>(GameObject::makePointLight(500.0f, 80.0f));
		sunLight->color = { 1.0f, 1.0f, 1.0f };
		sunLight->transform.translation = { 0.0f, 0.0f, 0.0f };
		gameObjects.emplace(sunLight->getId(), std::move(sunLight));

		// ========== PLANETAS ==========
		// Estrutura: Distância, Velocidade Órbita, Velocidade Rotação Própria

		// MERCÚRIO (O mais rápido em órbita)
		float mercDist = 12.0f * DIST_SCALE;
		auto mercury = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.004f * SIZE_SCALE * 10, false));
		mercury->transform.translation = { mercDist, 0.0f, 0.0f };
		mercury->orbitRadius = mercDist;
		mercury->orbitSpeed = 4.7f;
		mercury->selfRotationSpeed = baseSpeed / 58.6f;
		mercury->texture = std::make_shared<Texture>(engineDevice, "../Models/mercury.jpg");
		gameObjects.emplace(mercury->getId(), std::move(mercury));

		// VÊNUS (Rotação lenta)
		float venDist = 22.0f * DIST_SCALE;
		auto venus = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.009f * SIZE_SCALE * 10, false));
		venus->transform.translation = { venDist, 0.0f, 0.0f };
		venus->orbitRadius = venDist;
		venus->orbitSpeed = 3.5f;
		venus->selfRotationSpeed = baseSpeed / 243.0f;
		venus->texture = std::make_shared<Texture>(engineDevice, "../Models/venus.jpg");
		gameObjects.emplace(venus->getId(), std::move(venus));

		// TERRA
		float earthDist = 32.0f * DIST_SCALE;
		auto earth = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.01f * SIZE_SCALE * 10, false));
		earth->transform.translation = { earthDist, 0.0f, 0.0f };
		earth->orbitRadius = earthDist;
		earth->orbitSpeed = 2.4f;
		earth->selfRotationSpeed = baseSpeed;
		earth->texture = std::make_shared<Texture>(engineDevice, "../Models/earth.jpg");
		gameObjects.emplace(earth->getId(), std::move(earth));

		// MARTE
		float marsDist = 42.0f * DIST_SCALE;
		auto mars = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.005f * SIZE_SCALE * 10, false));
		mars->transform.translation = { marsDist, 0.0f, 0.0f };
		mars->orbitRadius = marsDist;
		mars->orbitSpeed = 2.0f;
		mars->selfRotationSpeed = baseSpeed / 1.03f;
		mars->texture = std::make_shared<Texture>(engineDevice, "../Models/mars.jpg");
		gameObjects.emplace(mars->getId(), std::move(mars));

		// JÚPITER (Gira muito rápido no próprio eixo)
		float jupDist = 60.0f * DIST_SCALE;
		auto jupiter = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.06f * SIZE_SCALE * 10, false));
		jupiter->transform.translation = { jupDist, 0.0f, 0.0f };
		jupiter->orbitRadius = jupDist;
		jupiter->orbitSpeed = 1.3f;
		jupiter->selfRotationSpeed = baseSpeed / 0.41f;
		jupiter->texture = std::make_shared<Texture>(engineDevice, "../Models/jupiter.jpg");
		gameObjects.emplace(jupiter->getId(), std::move(jupiter));

		// SATURNO
		float satDist = 80.0f * DIST_SCALE;
		auto saturn = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.05f * SIZE_SCALE * 10, false));
		saturn->transform.translation = { satDist, 0.0f, 0.0f };
		saturn->orbitRadius = satDist;
		saturn->orbitSpeed = 0.9f;
		saturn->selfRotationSpeed = baseSpeed / 0.45f;
		saturn->texture = std::make_shared<Texture>(engineDevice, "../Models/saturn.jpg");
		gameObjects.emplace(saturn->getId(), std::move(saturn));

		// URANO
		float uraDist = 100.0f * DIST_SCALE;
		auto uranus = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.02f * SIZE_SCALE * 10, false));
		uranus->transform.translation = { uraDist, 0.0f, 0.0f };
		uranus->orbitRadius = uraDist;
		uranus->orbitSpeed = 0.6f;
		uranus->selfRotationSpeed = baseSpeed / 0.72f;
		uranus->texture = std::make_shared<Texture>(engineDevice, "../Models/uranus.jpg");
		gameObjects.emplace(uranus->getId(), std::move(uranus));

		// NETUNO
		float nepDist = 120.0f * DIST_SCALE;
		auto neptune = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.02f * SIZE_SCALE * 10, false));
		neptune->transform.translation = { nepDist, 0.0f, 0.0f };
		neptune->orbitRadius = nepDist;
		neptune->orbitSpeed = 0.5f;
		neptune->selfRotationSpeed = baseSpeed / 0.67f;
		neptune->texture = std::make_shared<Texture>(engineDevice, "../Models/neptune.jpg");
		gameObjects.emplace(neptune->getId(), std::move(neptune));
	}
}