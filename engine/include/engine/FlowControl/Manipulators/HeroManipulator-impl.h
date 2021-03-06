#pragma once

#include "engine/FlowControl/Manipulators/HeroManipulator.h"
#include "Cards/CardDispatcher.h"

namespace engine {
	namespace FlowControl
	{
		namespace Manipulators
		{
			inline void HeroManipulator::GainArmor(int amount)
			{
				GetCard().SetArmor(GetCard().GetArmor() + amount);
			}

			inline state::CardRef HeroManipulator::Transform(Cards::CardId id)
			{
				assert(GetCard().GetCardType() == state::kCardTypeHero);
				assert(GetCard().GetZone() == state::kCardZonePlay);

				state::CardRef new_card_ref = BoardManipulator(state_, flow_context_).AddCardById(id, GetCard().GetPlayerIdentifier());
				state_.GetZoneChanger<state::kCardZonePlay, state::kCardTypeHero>(card_ref_)
					.ReplaceBy(new_card_ref);

				return new_card_ref;
			}
		}
	}
}