
#include <chrono>
#include <iostream>
#include <sstream>

#include "FlowControl/FlowController-impl.h"
#include "Cards/PreIndexedCards.h"
#include "MCTS/TestStateBuilder.h"
#include "UI/AIController.h"
#include "UI/CompetitionGuide.h"

static void Initialize()
{
	std::cout << "Reading json file...";
	if (!Cards::Database::GetInstance().Initialize("cards.json")) assert(false);
	Cards::PreIndexedCards::GetInstance().Initialize();
	std::cout << " Done." << std::endl;
}

class InteractiveShellHandler
{
public:
	InteractiveShellHandler(int tree_samples, std::mt19937 & rand) :
		controller_(tree_samples, rand), node_(nullptr), threads_(1)
	{}

	InteractiveShellHandler(InteractiveShellHandler const&) = delete;
	InteractiveShellHandler & operator=(InteractiveShellHandler const&) = delete;

	void DoCommand(std::istream & is, std::ostream & s)
	{
		std::string cmd;
		is >> cmd;
		if (cmd == "h" || cmd == "help") {
			s << "Commands: " << std::endl
				<< "h or help: show this message" << std::endl
				<< "threads: set number of threads" << std::endl
				<< "s or start (secs): to run for a specified seconds" << std::endl
				<< "root (1 or 2): set node to root node of player 1 or 2" << std::endl
				<< "info: show info for selected node" << std::endl
				<< "node (addr): set node to specified address." << std::endl;
		}
		else if (cmd == "threads") {
			is >> threads_;
			s << "Set thread count to " << threads_ << std::endl;
		}
		else if (cmd == "s" || cmd == "start") {
			auto start_board_getter = [](int seed) -> state::State {
				return TestStateBuilder().GetState(seed);
			};

			int secs = 0;
			is >> secs;
				
			auto seed = std::random_device()();

			s << "Running for " << secs << " seconds with " << threads_ << " threads "
				<< "(seed = " << seed << ")" << std::endl;

			auto start = std::chrono::steady_clock::now();
			std::chrono::steady_clock::time_point run_until =
				std::chrono::steady_clock::now() +
				std::chrono::seconds(secs);

			long long last_show_rest_sec = -1;
			auto continue_checker = [&]() {
				auto now = std::chrono::steady_clock::now();
				if (now > run_until) return false;

				auto rest_sec = std::chrono::duration_cast<std::chrono::seconds>(run_until - now).count();
				if (rest_sec != last_show_rest_sec) {
					s << "Rest seconds: " << rest_sec << std::endl;
					last_show_rest_sec = rest_sec;
				}
				return true;
			};

			auto start_i = controller_.GetStatistic().GetSuccededIterates();
			controller_.Run(continue_checker, threads_, seed, start_board_getter);
			auto end_i = controller_.GetStatistic().GetSuccededIterates();

			s << std::endl;
			s << "Done iterations: " << (end_i - start_i) << std::endl;
			s << "====== Statistics =====" << std::endl;
			controller_.GetStatistic().GetDebugMessage();

			auto now = std::chrono::steady_clock::now();
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
			auto speed = (double)(end_i - start_i) / ms * 1000;
			s << "Iterations per second: " << speed << std::endl;
			s << std::endl;
		}
		else if (cmd == "root") {
			DoRoot(is, s);
		}
		else if (cmd == "info") {
			DoInfo(s);
		}
		else if (cmd == "node") {
			uint64_t v = 0;
			is >> std::hex >> v >> std::dec;
			node_ = (mcts::builder::TreeBuilder::TreeNode *)(v);
			s << "Node set to: " << node_ << std::endl;
		}
		else {
			s << "Unknown command. Type 'h' or 'help' for usage help." << std::endl;
		}
	}

private:
	void DoRoot(std::istream & is, std::ostream & s)
	{
		int v = 0;
		is >> v;

		state::PlayerIdentifier side;
		if (v == 1) side = state::PlayerIdentifier::First();
		else if (v == 2) side = state::PlayerIdentifier::Second();
		else {
			s << "invalid input" << std::endl;
			return;
		}
		node_ = controller_.GetRootNode(side);
		s << "Current node set to: " << node_ << std::endl;
	}

	void DoInfo(std::ostream & s)
	{
		if (!node_) {
			s << "Should set node first." << std::endl;
			return;
		}

		s << "Action type: " << GetActionType(node_->GetActionType()) << std::endl;

		if (node_->GetActionType() == mcts::ActionType::kMainAction) {
			s << "Playable hand cards:";
			auto * board_view = node_->GetAddon().consistency_checker.GetBoard();
			node_->GetAddon().action_analyzer.ForEachPlayableCard([&](size_t idx) {
				s << " [" << idx << "] ";
				if (board_view) {
					s << board_view->GetSelfHand()[idx].card_id;
				}
				return true;
			});
			s << std::endl;
		}

		s << "Children:" << std::endl;
		node_->ForEachChild([&](int choice, mcts::selection::ChildType const& child) {
			auto const* edge_addon = node_->GetEdgeAddon(choice);

			if (child.IsRedirectNode()) {
				s << "  " << choice << ": [REDIRECT]" << std::endl;
			}
			else {
				s << "  " << choice << ": [NORMAL] "
					<< child.GetNode() << std::endl;
			}

			if (node_->GetActionType() == mcts::ActionType::kMainAction) {
				auto op = node_->GetAddon().action_analyzer.GetMainOpType(choice);
				s << "    Main Action Op: " << GetOpType(op) << std::endl;
			}

			if (edge_addon) {
				s << "    Chosen time: " << edge_addon->GetChosenTimes() << std::endl;

				double credit_percentage = (double)edge_addon->GetCredit() / edge_addon->GetTotal() * 100;
				s << "    Credit: " << edge_addon->GetCredit() << " / " << edge_addon->GetTotal()
					<< " (" << credit_percentage << "%)"
					<< std::endl;
			}
			return true;
		});
		s << "BoardNodeMap:" << std::endl;
		node_->GetAddon().board_node_map.ForEach([&](mcts::board::BoardView board_view, mcts::builder::TreeBuilder::TreeNode* node) {
			uint64_t total_chosen_time = 0;
			node->ForEachChild([&](int choice, mcts::selection::ChildType const& child) {
				total_chosen_time += child.GetEdgeAddon().GetChosenTimes();
				return true;
			});
			s << "  " << node << " Chosen time: " << total_chosen_time
				<< std::endl;
			PrintBoardView(board_view, "    ", s);
			return true;
		});
	}

	std::string GetActionType(mcts::ActionType type) {
		using mcts::ActionType;
		switch (type.GetType()) {
		case ActionType::kInvalid: return "kInvalid";
		case ActionType::kMainAction: return "kMainAction";
		case ActionType::kRandom: return "kRandom";
		case ActionType::kChooseHandCard: return "kChooseHandCard";
		case ActionType::kChooseAttacker: return "kChooseAttacker";
		case ActionType::kChooseDefender: return "kChooseDefender";
		case ActionType::kChooseMinionPutLocation: return "kChooseMinionPutLocation";
		case ActionType::kChooseTarget: return "kChooseTarget";
		case ActionType::kChooseOne: return "kChooseOne";
		default: return "Unknown!!!";
		}
	}

	std::string GetOpType(mcts::board::BoardActionAnalyzer::OpType op) {
		using mcts::board::BoardActionAnalyzer;
		switch (op) {
		case BoardActionAnalyzer::kInvalid: return "kInvalid";
		case BoardActionAnalyzer::kPlayCard: return "kPlayCard";
		case BoardActionAnalyzer::kAttack: return "kAttack";
		case BoardActionAnalyzer::kHeroPower: return "kHeroPower";
		case BoardActionAnalyzer::kEndTurn: return "kEndTurn";
		default: return "Unknown!!!";
		}
	}

	void PrintBoardView(mcts::board::BoardView const& view, std::string const& line_prefix, std::ostream & s) {
		s << line_prefix << "Side: " << GetSideStr(view.GetSide())
			<< std::endl;

		s << line_prefix << "Self resource: "
			<< view.GetSelfCrystal().current << " / " << view.GetSelfCrystal().total
			<< std::endl;

		s << line_prefix << "Self Hero: "
			<< view.GetSelfHero().hp << " / " << view.GetSelfHero().max_hp
			<< " armor: " << view.GetSelfHero().armor
			<< std::endl;

		s << line_prefix << "Self hand cards: ";
		for (auto const& card : view.GetSelfHand()) {
			s << card.card_id << " ";
		}
		s << std::endl;

		s << line_prefix << "Self Minions: ";
		for (auto const& minion : view.GetSelfMinions()) {
			s << minion.hp << "/" << minion.max_hp << " ";
		}
		s << std::endl;

		s << line_prefix << "Opponent resource: "
			<< view.GetOpponentCrystal().current << " / " << view.GetOpponentCrystal().total
			<< std::endl;

		s << line_prefix << "Opponent Hero: "
			<< view.GetOpponentHero().hp << " / " << view.GetOpponentHero().max_hp
			<< " armor: " << view.GetOpponentHero().armor
			<< std::endl;

		s << line_prefix << "Opponent Minions: ";
		for (auto const& minion : view.GetOpponentMinions()) {
			s << minion.hp << "/" << minion.max_hp << " ";
		}
		s << std::endl;
	}

	std::string GetSideStr(state::PlayerSide side) {
		if (side == state::kPlayerFirst) return "kPlayerFirst";
		if (side == state::kPlayerSecond) return "kPlayerSecond";
		if (side == state::kPlayerInvalid) return "kPlayerInvalid";
		return "Unknown!!!";
	}

private:
	ui::AIController controller_;
	mcts::builder::TreeBuilder::TreeNode const* node_;
	int threads_;
};

void TestAI()
{
	srand(0);

	int tree_samples = 100;
	std::mt19937 rand;

	InteractiveShellHandler handler(tree_samples, rand);

	while (std::cin) {
		std::string cmdline;
		std::cout << "Command: ";
		std::getline(std::cin, cmdline);

		if (cmdline == "q" || cmdline == "quit") {
			std::cout << "Good bye!" << std::endl;
			break;
		}

		std::istringstream iss(cmdline);
		handler.DoCommand(iss, std::cout);
	}
}

int main(int argc, char *argv[])
{
	Initialize();
	TestAI();
	return 0;
}
