#include <iostream>

#include "LuaWrapper.h"
#include "Behaviors\Behavior.h"
#include "Behaviors\BehaviorMgr.h"
#include "Behaviors\behaviortree.h"

int main()
{
  LuaWrapper *LUA = LuaWrapper::instance();
  LUA->load_all_scripts();

  //LUA->test_load();

  BehaviorTree test_tree;
  test_tree.add_behavior_as_child_of("root", "");
  test_tree.add_behavior_as_child_of("skipped_behavior", "root");
  test_tree.add_behavior_as_child_of("test_behavior", "root");
  test_tree.add_behavior_as_child_of("child_behavior", "test_behavior");

  test_tree.process();

	return 0;
}