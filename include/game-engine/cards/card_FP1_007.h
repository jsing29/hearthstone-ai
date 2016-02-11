#ifndef GAME_ENGINE_CARDS_CARD_FP1_007
#define GAME_ENGINE_CARDS_CARD_FP1_007

#include "game-engine/board.h"
#include "game-engine/card-id-map.h"
#include "game-engine/stages/helper.h"
#include "game-engine/board-objects/minion-manipulator.h"

namespace GameEngine {
namespace Cards {

class Card_FP1_007
{
public:
	static constexpr int card_id = CARD_ID_FP1_007;

	// Nerubian Egg

	static void Deathrattle(GameEngine::BoardObjects::MinionManipulator triggering_minion)
	{
		// summon Nerubian (AT_036t) when death at [pos]
		Card card = CardDatabase::GetInstance().GetCard(CARD_ID_AT_036t);

		auto inserter = triggering_minion.GetMinions().GetInserterBefore(triggering_minion.GetBoard(), triggering_minion.GetMinion());
#ifdef DEBUG
		if (inserter.IsEnd()) {
			std::cout << "deathrattle triggering minion is vanished!" << std::endl;
		}
#endif
		
		StageHelper::SummonMinion(card, inserter);
	}

	static void AfterSummoned(GameEngine::BoardObjects::MinionManipulator & summoned_minion)
	{
		summoned_minion.AddOnDeathTrigger(Deathrattle);
	}
};

} // namespace Cards
} // namespace GameEngine

#endif
