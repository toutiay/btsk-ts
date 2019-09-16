#ifndef _BEHAVIORTREE_H_
#define _BEHAVIORTREE_H_

#include <memory>
#include <vector>

#include "..\ntree.h"
#include "Behavior.h"
#include "..\LuaWrapper.h"

class BehaviorTree
{
public:
  typedef NTreeNode<std::weak_ptr<Behavior>> BehaviorTreeNode;

  inline         BehaviorTree () { m_tree_root.data.reset(); }
  inline virtual ~BehaviorTree() { }

  void add_behavior_as_child_of(const std::string& behavior_name, const std::string& parent_behavior);
  void process();

private:
  void   run_tree(BehaviorTreeNode* node);
  void   test_should_run();

private:  //PRIVATE DATA
  BehaviorTreeNode m_tree_root;
}; 

#endif // _BEHAVIORTREE_H_
