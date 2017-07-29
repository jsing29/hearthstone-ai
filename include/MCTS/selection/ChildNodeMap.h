#pragma once

#include <unordered_map>
#include <memory>
#include "MCTS/selection/EdgeAddon.h"

namespace mcts
{
	namespace selection
	{
		class TreeNode;

		class ChildType
		{
		public:
			ChildType() : edge_addon_(), is_redirect_node_(false), node_() {}

			ChildType(ChildType const&) = delete;
			ChildType & operator=(ChildType const&) = delete;

			EdgeAddon & GetEdgeAddon() { return edge_addon_; }
			EdgeAddon const& GetEdgeAddon() const { return edge_addon_; }
			
			void SetNode(std::unique_ptr<TreeNode> node) {
				assert(!node_);
				assert(!is_redirect_node_);
				node_ = std::move(node);
			}

			void SetAsRedirectNode() {
				assert(!node_);
				is_redirect_node_ = true;
			}

			bool IsRedirectNode() const { return is_redirect_node_; }
			TreeNode * GetNode() const {
				assert(!is_redirect_node_);
				return node_.get();
			}

		private:
			EdgeAddon edge_addon_;
			bool is_redirect_node_;
			std::unique_ptr<TreeNode> node_;
		};

		// Thread safety:
		//    Multiple threads can call Get() concurrently.
		//    Other methods are not thread-safe.
		class ChildNodeMap
		{
		public:
			// Hash table is used here, since
			//   1. we don't know the total choices in advance
			//   2. the key is 'choice', which might be card id for choose-one action
			using ChildMapType = std::unordered_map<int, ChildType>;

			ChildNodeMap() : map_() {}

			ChildType * Get(int choice) {
				auto it = map_.find(choice);
				if (it == map_.end()) return nullptr;
				return &(it->second);
			}

			ChildType const* Get(int choice) const {
				auto it = map_.find(choice);
				if (it == map_.end()) return nullptr;
				return &(it->second);
			}

			ChildType* Create(int choice) {
				assert(map_.find(choice) == map_.end());
				return &map_[choice];
			}

			template <typename Functor>
			void ForEach(Functor&& functor) const {
				for (auto const& kv : map_) {
					if (!functor(kv.first, kv.second)) return;
				}
			}

		private:
			ChildMapType map_;
		};
	}
}