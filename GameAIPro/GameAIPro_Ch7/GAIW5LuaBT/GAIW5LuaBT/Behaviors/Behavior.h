#ifndef _BEHAVIOR_H_
#define _BEHAVIOR_H_

#include <string>
#include <memory>
#include "../ntree.h"

class Behavior
{

public:
	Behavior  (std::string name) : m_behavior_name(name) { }

  bool operator()(const std::string& name) { return m_behavior_name == name; }
  bool operator==(const std::string& name) { return m_behavior_name == name; }

  bool              should_run  ();
  void              run_behavior();

private:
  enum BehaviorFunctions {
    BEHAVIOR = 0,
    PRECONDITION,
    NUM_BEHAVIOR_FUNCTIONS
  };

  void   call_behavior_function () const;

public:  //PUBLIC DATA
  static const char m_behavior_function_name[];
  static const char m_behavior_precondition_name[];

private:
  std::string      m_behavior_name;

};


template <>
inline NTreeNode<std::weak_ptr<Behavior>>* NTreeNode<std::weak_ptr<Behavior>>::find_node (const std::weak_ptr<Behavior>& value)
{
  if (data.lock() == value.lock()) {
    return this;
  } else {
    if (sibling) {
      NTreeNode<std::weak_ptr<Behavior>>* sibling_value = sibling->find_node(value);
      if (sibling_value) { 
        return sibling_value;
      }
    }
    if (child) {
      NTreeNode<std::weak_ptr<Behavior>>* child_value = child->find_node(value);
      if (child_value) {
        return child_value;
      }
    }

    return NULL;
  }
}

#endif // _BEHAVIOR_H_
