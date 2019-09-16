#ifndef _BEHAVIORMGR_H_
#define _BEHAVIORMGR_H_

#include <hash_map>
#include <memory>
#include <string>
#include "Behavior.h"

class BehaviorMgr
{
  typedef std::shared_ptr<Behavior> BehaviorPtr;
	typedef std::hash_map<std::string, BehaviorPtr> BehaviorMap;
  
public:
  static BehaviorMgr* instance() { if (!m_behavior_mgr) { m_behavior_mgr = new BehaviorMgr(); } return m_behavior_mgr; }

  void add_behavior (const std::string &behavior_filename);
  std::weak_ptr<Behavior> get_behavior (const std::string &behavior_name);

private:
  inline          BehaviorMgr () { }
  inline virtual ~BehaviorMgr () { }

private:
	BehaviorMap m_behavior_map;
  static BehaviorMgr *m_behavior_mgr;

};

#endif //_BEHAVIORMGR_H_