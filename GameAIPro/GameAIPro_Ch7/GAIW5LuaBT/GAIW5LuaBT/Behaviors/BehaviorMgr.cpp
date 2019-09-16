#include "BehaviorMgr.h"

BehaviorMgr* BehaviorMgr::m_behavior_mgr = NULL;

void BehaviorMgr::add_behavior (const std::string &behavior_filename) 
{
  std::string behavior_name = behavior_filename.substr(0, behavior_filename.length() - 4);
  m_behavior_map[behavior_name] = BehaviorPtr(new Behavior(behavior_name));
}

std::weak_ptr<Behavior> BehaviorMgr::get_behavior (const std::string &behavior_name)
{
  return std::weak_ptr<Behavior>(m_behavior_map[behavior_name]);
}

