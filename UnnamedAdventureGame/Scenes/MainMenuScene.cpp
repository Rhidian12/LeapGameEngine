#include "MainMenuScene.h"

#include "SceneGraph/Scene.h"
#include "Components/RenderComponents/CameraComponent.h"
#include "Camera.h"

#include "Components/Transform/Transform.h"

#include "ServiceLocator/ServiceLocator.h"
#include "Interfaces/IRenderer.h"

#include "GameContext/GameContext.h"
#include "GameContext/Window.h"

#include <Components/RenderComponents/MeshRenderer.h>
#include "../Components/Transformator.h"
#include "../Components/InfoUI.h"

#include "Shaders/Pos3D.h"
#include "Shaders/PosNorm3D.h"

#include "Interfaces/IMaterial.h"
#include "Components/RenderComponents/DirectionalLightComponent.h"
#include <Shaders/PosNormTex3D.h>

#include <Data/CustomMesh.h>

#include "Components/RenderComponents/UIComponents/Image.h"
#include "../Components/FreeCamMovement.h"
#include "../Components/WindowManager.h"
#include "Components/RenderComponents/UIComponents/CanvasComponent.h"
#include "Components/RenderComponents/UIComponents/RectTransform.h"
#include "Components/RenderComponents/UIComponents/Button.h"
#include "Components/RenderComponents/UIComponents/CanvasActions.h"

#include "Debug.h"
#include <Interfaces/IAudioSystem.h>
#include <Interfaces/IAudioClip.h>

#include <Components/Audio/AudioSource.h>
#include <Components/Audio/AudioListener.h>

#include "../Components/AudioTester.h"

#include "Shaders/Heightmap.h"
#include <Components/RenderComponents/TerrainComponent.h>

#include <Data/Vertex.h>
#include "../Components/SineWaveTerrain.h"

#include <Components/Physics/BoxCollider.h>
#include <Components/Physics/Rigidbody.h>
#include <Components/Physics/SphereCollider.h>
#include <Components/Physics/CapsuleCollider.h>

#include <Interfaces/IPhysicsMaterial.h>
#include <Interfaces/IPhysics.h>

#include "../Components/PrintCollision.h"
#include "../Components/ApplyForces.h"
#include "../Components/ColliderScaler.h"
#include "../Components/PrintVelocity.h"

void unag::MainMenuScene::Load(leap::Scene& scene)
{
	leap::GameObject* pDirLight{ scene.CreateGameObject("Directional Light") };
	pDirLight->AddComponent<leap::DirectionalLightComponent>();
	pDirLight->GetTransform()->SetWorldRotation(60.0f, 0.0f, 0.0f);

	leap::GameObject* pCameraObj{ scene.CreateGameObject("Main Camera") };
	const leap::CameraComponent* pMainCamera{ pCameraObj->AddComponent<leap::CameraComponent>() };
	pMainCamera->SetAsActiveCamera(true);
	//pCameraObj->AddComponent<FreeCamMovement>();
	pCameraObj->AddComponent<leap::AudioListener>();
	pCameraObj->GetTransform()->SetLocalPosition(-10.0f, 5.0f, 0.0f);
	pCameraObj->GetTransform()->SetLocalRotation(0.0f, 90.0f, 0.0f);

	auto canvas{ scene.CreateGameObject("Canvas") };
	leap::CanvasComponent* pCanvas{ canvas->AddComponent<leap::CanvasComponent>() };
	pCanvas->SetReferenceResolution({ 1920,1080 });
	pCanvas->SetMatchMode(leap::CanvasComponent::MatchMode::MatchHeight);
	canvas->AddComponent<leap::CanvasActions>();

	const auto pTexturedMaterial{ leap::ServiceLocator::GetRenderer().CloneMaterial("Default", "Texture") };
	pTexturedMaterial->SetTexture("gDiffuseMap", leap::ServiceLocator::GetRenderer().CreateTexture("Data/debug.png"));

	const auto info{ scene.CreateGameObject("Info") };
	info->AddComponent<InfoUI>();

	const auto windowControls{ scene.CreateGameObject("Window") };
	windowControls->AddComponent<WindowManager>();

	/*auto pSlideMaterial{ leap::ServiceLocator::GetPhysics().CreateMaterial() };
	pSlideMaterial->SetStaticFriction(0.1f);
	pSlideMaterial->SetDynamicFriction(0.1f);*/
	auto pBounceMaterial{ leap::ServiceLocator::GetPhysics().CreateMaterial() };
	pBounceMaterial->SetBounciness(1.0f);

	auto pBox{ scene.CreateGameObject("Sphere") };
	auto pBoxColl{ pBox->AddComponent<leap::SphereCollider>() };
	pBoxColl->SetMaterial(pBounceMaterial);
	auto pBoxRb{ pBox->AddComponent<leap::Rigidbody>() };
	//pBox->AddComponent<PrintVelocity>();
	pBoxRb->SetVelocity(0.0f, 0.0f, 3.0f);
	pBoxRb->SetAngularVelocity(0.0f, 100.0f, 0.0f);
	pBoxRb->SetConstraint(leap::physics::Rigidbody::Constraint::Flag::RotationX, true);
	auto pBoxMesh{ pBox->AddComponent<leap::MeshRendererComponent>() };
	pBoxMesh->LoadMesh("Data/Engine/Models/sphere.obj");
	pBoxMesh->SetMaterial(leap::ServiceLocator::GetRenderer().CloneMaterial("Default", "Texture"));
	pBox->GetTransform()->Translate(0.0f, 2.0f, 0.0f);
	pBox->GetTransform()->Rotate(0.0f, 0.0f, 0.0f);
	//pBox->AddComponent<PrintCollision>();
	//pBox->AddComponent<ApplyForces>();
	auto pBoxTrigger{ pBox->AddComponent<leap::BoxCollider>() };
	pBoxTrigger->SetSize(3.0f);
	pBoxTrigger->SetTrigger(true);

	auto pBox4{ scene.CreateGameObject("Box") };
	pBox4->AddComponent<leap::BoxCollider>()/*->SetMaterial(pBounceMaterial)*/;
	pBox4->AddComponent<leap::Rigidbody>();
	auto pBoxMesh4{ pBox4->AddComponent<leap::MeshRendererComponent>() };
	pBoxMesh4->SetMaterial(leap::ServiceLocator::GetRenderer().CloneMaterial("Default", "Texture"));
	pBoxMesh4->LoadMesh("Data/Engine/Models/cube.obj");
	pBox4->GetTransform()->Translate(0.0f, 3.0f, 0.0f);
	pBox4->GetTransform()->Rotate(0.0f, 0.0f, 0.0f);
	pBox4->AddComponent<ColliderScaler>();
	//pBox4->AddComponent<PrintCollision>();

	auto pBox2{ scene.CreateGameObject("Ground") };
	pBox2->AddComponent<leap::BoxCollider>()/*->SetMaterial(pBounceMaterial)*/;
	auto pBoxMesh2{ pBox2->AddComponent<leap::MeshRendererComponent>() };
	pBoxMesh2->SetMaterial(leap::ServiceLocator::GetRenderer().CloneMaterial("Default", "Texture"));
	pBoxMesh2->LoadMesh("Data/Engine/Models/cube.obj");
	pBox2->GetTransform()->Translate(0.0f, -1.5f, 0.0f);
	pBox2->GetTransform()->Scale(20.0f, 1.0f, 20.0f);
	pBox2->AddComponent<PrintCollision>();
}
