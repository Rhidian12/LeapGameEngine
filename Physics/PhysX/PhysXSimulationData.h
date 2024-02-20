#pragma once

#include "../Data/SimulationEventData.h"

#include <Delegate.h>

namespace physx
{
	class PxShape;
}

namespace leap::physics
{
	struct SimulationPair final
	{
		const physx::PxShape* pShape0{};
		const physx::PxShape* pShape1{};
	};

	struct SimulationEvent final
	{
		SimulationEventType type{};
		const physx::PxShape* pShape0{};
		const physx::PxShape* pShape1{};
	};

	DECLARE_DELEGATE_OneParam(OnSimulationEvent, const SimulationEvent&);
}