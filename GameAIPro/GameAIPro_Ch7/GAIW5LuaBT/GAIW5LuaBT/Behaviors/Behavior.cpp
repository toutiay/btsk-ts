#include "Behavior.h"

#include "..\LuaWrapper.h"

const char Behavior::m_behavior_function_name[]     = "behavior";
const char Behavior::m_behavior_precondition_name[] = "precondition";


void Behavior::call_behavior_function () const
{
  LuaWrapper* wrapper = LuaWrapper::instance();
  wrapper->call_function(m_behavior_name.c_str(), Behavior::m_behavior_function_name);
}

bool Behavior::should_run ()
{
  LuaWrapper* wrapper = LuaWrapper::instance();
  return wrapper->call_function_with_bool_return(m_behavior_name.c_str(), Behavior::m_behavior_precondition_name);
}

void Behavior::run_behavior ()
{
  call_behavior_function();
}
