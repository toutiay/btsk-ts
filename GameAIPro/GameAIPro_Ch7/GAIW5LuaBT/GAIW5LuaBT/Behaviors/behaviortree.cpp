#include "behaviortree.h"
#include "BehaviorMgr.h"

#include <assert.h>
#include <algorithm>


void BehaviorTree::add_behavior_as_child_of (const std::string& behavior_name, const std::string& parent_behavior)
{
  std::weak_ptr<Behavior> new_behavior = BehaviorMgr::instance()->get_behavior(behavior_name);
  std::shared_ptr<Behavior> new_behavior_ptr = new_behavior.lock();
  if (new_behavior_ptr) {
    if (parent_behavior.empty()) { //Add as root if no parent
      assert(m_tree_root.count_nodes() == 1);
      m_tree_root.data = new_behavior;
    } else {
      std::weak_ptr<Behavior> parent_behavior_ptr = BehaviorMgr::instance()->get_behavior(parent_behavior);
      std::shared_ptr<Behavior> parent_behavior_locked_ptr = parent_behavior_ptr.lock();
      BehaviorTreeNode* parent_node = m_tree_root.find_node(parent_behavior_locked_ptr);
      parent_node->add_child(new_behavior);
    }
  }
}

void BehaviorTree::process()
{
  run_tree(&m_tree_root);
}

void BehaviorTree::run_tree (BehaviorTreeNode* node)
{
  if (node) {
    std::shared_ptr<Behavior> behavior_ptr = node->data.lock();
    if (behavior_ptr && behavior_ptr->should_run()) {
      behavior_ptr->run_behavior();
      run_tree(node->child);
    } else {
      run_tree(node->sibling);
    }
  }
}
