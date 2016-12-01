#pragma once

#include "FlowControl/Result.h"
#include "FlowControl/Manipulate.h"
#include "FlowControl/Helpers/Resolver.h"
#include "FlowControl/Manipulators/HeroManipulator.h"
#include "FlowControl/IRandomGenerator.h"
#include "FlowControl/ActionParameterWrapper.h"

namespace FlowControl
{
	namespace Helpers
	{
		class EndTurnHelper
		{
		public:
			EndTurnHelper(state::State & state, FlowContext & flow_context)
				: state_(state), flow_context_(flow_context)
			{

			}

			Result Go()
			{
				Result rc = kResultNotDetermined;

				if (state_.turn == 89) return kResultDraw;
				++state_.turn;

				EndTurn();
				if ((rc = Resolver(state_, flow_context_).Resolve()) != kResultNotDetermined) return rc;

				state_.ChangePlayer();

				StartTurn();
				if ((rc = Resolver(state_, flow_context_).Resolve()) != kResultNotDetermined) return rc;

				DrawCard();
				if ((rc = Resolver(state_, flow_context_).Resolve()) != kResultNotDetermined) return rc;

				return kResultNotDetermined;
			}

		private:
			void EndTurn()
			{
				state_.event_mgr.TriggerEvent<state::Events::EventTypes::OnTurnEnd>();
			}

			void StartTurn()
			{
				state_.GetCurrentPlayer().resource_.IncreaseTotal();
				state_.GetCurrentPlayer().resource_.Refill();
				// TODO: overload

				state_.event_mgr.TriggerEvent<state::Events::EventTypes::OnTurnStart>();
			}

			void DrawCard()
			{
				Manipulate(state_, flow_context_).CurrentHero().DrawCard();
			}

		private:
			state::State & state_;
			FlowContext & flow_context_;
		};
	}
}