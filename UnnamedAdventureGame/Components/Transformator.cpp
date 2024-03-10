#include "Transformator.h"

#include "Components/Transform/Transform.h"

#include "GameContext/GameContext.h"
#include "GameContext/Timer.h"

#include <Leap.h>

void unag::Transformator::Update()
{
	GetTransform()->Rotate(0.0f, 90.0f * leap::LeapEngine::GetGameContext().GetTimer()->GetDeltaTime(), 0.0f);
}
