#include "PhysicsSync.h"

#include "../Components/Transform/Transform.h"
#include "../Components/Physics/Collider.h"

#include "../SceneGraph/GameObject.h"

#include <Data/CollisionData.h>

void leap::PhysicsSync::SetTransform(void* pOwner, const glm::vec3& position, const glm::quat& rotation)
{
	Transform* pTransform{ reinterpret_cast<GameObject*>(pOwner)->GetTransform() };

	pTransform->SetWorldPosition(position);
	pTransform->SetWorldRotation(rotation);
}

std::pair<const glm::vec3&, const glm::quat&> leap::PhysicsSync::GetTransform(void* pOwner)
{
	Transform* pTransform{ reinterpret_cast<GameObject*>(pOwner)->GetTransform() };

	return std::make_pair<const glm::vec3&, const glm::quat&>(pTransform->GetWorldPosition(), pTransform->GetWorldRotation());
}

void leap::PhysicsSync::OnCollisionEnter(const physics::CollisionData& collision)
{
	const auto colliders{ GetColliders(collision) };

	colliders.pFirst->NotifyCollisionEnter(colliders.pSecond);
	colliders.pSecond->NotifyCollisionEnter(colliders.pFirst);
}

void leap::PhysicsSync::OnCollisionStay(const physics::CollisionData& collision)
{
	const auto colliders{ GetColliders(collision) };

	colliders.pFirst->NotifyCollisionStay(colliders.pSecond);
	colliders.pSecond->NotifyCollisionStay(colliders.pFirst);
}

void leap::PhysicsSync::OnCollisionExit(const physics::CollisionData& collision)
{
	const auto colliders{ GetColliders(collision) };

	colliders.pFirst->NotifyCollisionExit(colliders.pSecond);
	colliders.pSecond->NotifyCollisionExit(colliders.pFirst);
}

void leap::PhysicsSync::OnTriggerEnter(const physics::CollisionData& collision)
{
	const auto colliders{ GetColliders(collision) };

	colliders.pFirst->NotifyTriggerEnter(colliders.pSecond);
	colliders.pSecond->NotifyTriggerEnter(colliders.pFirst);
}

void leap::PhysicsSync::OnTriggerStay(const physics::CollisionData& collision)
{
	const auto colliders{ GetColliders(collision) };

	colliders.pFirst->NotifyTriggerStay(colliders.pSecond);
	colliders.pSecond->NotifyTriggerStay(colliders.pFirst);
}

void leap::PhysicsSync::OnTriggerExit(const physics::CollisionData& collision)
{
	const auto colliders{ GetColliders(collision) };

	colliders.pFirst->NotifyTriggerExit(colliders.pSecond);
	colliders.pSecond->NotifyTriggerExit(colliders.pFirst);
}

leap::PhysicsSync::ColliderPair leap::PhysicsSync::GetColliders(const physics::CollisionData& collision)
{
	Collider* pFirstCollider{ reinterpret_cast<Collider*>(collision.pFirst) };
	Collider* pSecondCollider{ reinterpret_cast<Collider*>(collision.pSecond) };

	return { pFirstCollider, pSecondCollider };
}
