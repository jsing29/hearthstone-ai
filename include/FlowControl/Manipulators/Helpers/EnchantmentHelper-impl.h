#pragma once

#include <assert.h>
#include "FlowControl/Manipulate.h"
#include "FlowControl/Manipulators/Helpers/EnchantmentHelper.h"
#include "Cards/Database.h"

namespace FlowControl
{
	namespace Manipulators
	{
		namespace Helpers
		{
			inline void EnchantmentHelper::UpdateCharacter(state::Cards::EnchantableStates const& new_states)
			{
				state::Cards::EnchantableStates const& current_states = card_.GetRawData().enchantable_states;
				state::Cards::EnchantmentAuxData & data = card_.GetMutableEnchantmentAuxDataGetter().Get();

				auto card_manipulator = Manipulate(state_, flow_context_).Character(card_ref_);

				static_assert(state::Cards::EnchantableStates::kFieldChangeId == 4, "enchantable fields changed");

				if (new_states.attack != current_states.attack) {
					card_manipulator.Attack(new_states.attack);
					assert(card_.GetAttack() == new_states.attack);
				}
			}

			inline void EnchantmentHelper::UpdateMinion()
			{
				state::Cards::EnchantmentAuxData & data = card_.GetMutableEnchantmentAuxDataGetter().Get();
				state::Cards::EnchantableStates const& current_states = card_.GetRawData().enchantable_states;

				state::Cards::EnchantableStates new_states = data.origin_states;
				data.enchantments.ApplyAll(new_states);

				UpdateCharacter(new_states);

				auto manipulator = Manipulate(state_, flow_context_).Minion(card_ref_);

				if (new_states.player != current_states.player) {
					switch (card_.GetCardType()) {
					case state::kCardTypeMinion:
						ChangeMinionPlayer(new_states.player);
						break;
					default:
						throw std::exception("unsupported enchantment type");
					}
				}

				if (new_states.cost != current_states.cost) {
					manipulator.Cost(new_states.cost);
					assert(card_.GetCost() == new_states.cost);
				}
				if (new_states.max_hp != current_states.max_hp) {
					manipulator.MaxHP(new_states.max_hp);
					assert(card_.GetMaxHP() == new_states.max_hp);
				}

				if (new_states.taunt != current_states.taunt) {
					manipulator.Taunt(new_states.taunt);
					assert(card_.GetRawData().enchantable_states.taunt == new_states.taunt);
				}

				if (new_states.shielded != current_states.shielded) {
					manipulator.Shield(new_states.shielded);
					assert(card_.GetRawData().enchantable_states.shielded == new_states.shielded);
				}

				if (new_states.charge != current_states.charge) {
					manipulator.Charge(new_states.charge);
					assert(card_.GetRawData().enchantable_states.charge == new_states.charge);
				}

				assert(card_.GetRawData().enchantable_states == new_states);

				data.need_update = false;
			}

			inline void EnchantmentHelper::UpdateHero()
			{
				state::Cards::EnchantmentAuxData & data = card_.GetMutableEnchantmentAuxDataGetter().Get();
				state::Cards::EnchantableStates const& current_states = card_.GetRawData().enchantable_states;

				state::Cards::EnchantableStates new_states = data.origin_states;
				data.enchantments.ApplyAll(new_states);

				UpdateCharacter(new_states);

				assert(card_.GetRawData().enchantable_states == new_states);

				data.need_update = false;
			}

			inline void EnchantmentHelper::ChangeMinionPlayer(state::PlayerIdentifier player)
			{
				assert(card_.GetZone() == state::kCardZonePlay);

				if (state_.board.Get(player).minions_.Full()) {
					Manipulate(state_, flow_context_).Minion(card_ref_).Zone().ChangeTo<state::kCardZoneGraveyard>(player);
					assert(card_.GetZone() == state::kCardZoneGraveyard);
					assert(card_.GetPlayerIdentifier() == player);
				}
				else {
					int location = (int)state_.board.Get(player).minions_.Size();

					Manipulate(state_, flow_context_).Minion(card_ref_).Zone().ChangeTo<state::kCardZonePlay>(player, location);
					assert(card_.GetZone() == state::kCardZonePlay);
					assert(card_.GetPlayerIdentifier() == player);
				}
			}
		}
	}
}