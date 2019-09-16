//==============================================================================
//
// class PartBehavior
//
//
// <CLASS>
// <NAME>PartBehavior</NAME>
// <SUMMARY>
// PartBehavior is a part for decision-making through a Behavior Tree structure.
// </SUMMARY>
// </CLASS>
//
// (c)2004-2005 Big Huge Games, Inc.
//==============================================================================
#ifndef _PARTBEHAVIOR_H_
#define _PARTBEHAVIOR_H_

#include "Part.h"
#include "behavior.h"
#include "BehaviorTreeMgr.h" //For the index
#include "LeashMemory.h"
#include "TalentIndex.h"
#include "BehaviorMgr.h"
#include "BehaviorOverrideMgr.h"
#include <LuaLib\LuaScriptIndex.h>

namespace BHG {

  class MotionTreeEvent;
  class MotionTreeInstance;


class PartBehaviorBlueprint : public PartBlueprint {
public:
  inline          PartBehaviorBlueprint (void) : m_variable_overrides(MEM_PARTS), m_allowed_activities(MEM_PARTS), m_buffs(MEM_PARTS) { }
  inline virtual ~PartBehaviorBlueprint (void) { }

  virtual void stream_data (MemoryDataWalk* stream) 
  { 
    MEM_SCOPE("PartBehaviorBlueprintData");

    external_stream(stream, m_behavior_tree_index);
    external_stream(stream, m_variable_overrides);
    external_stream(stream, m_allowed_activities);
    external_stream(stream, m_max_pursuit_dist_sq);
    external_stream(stream, m_buffs);
    external_stream(stream, m_activity_ownership_preference);
    external_stream(stream, m_activity_starts_on_space_load);
  }
  virtual void post_load_fixup (void)
  {
    m_behavior_tree_index.post_load_fixup();
    array_post_load_fixup(m_buffs);
  }


  BehaviorTreeNonRefIndex m_behavior_tree_index;
  SimpleArray<BehaviorOverrideKey> m_variable_overrides;
  IntArray m_allowed_activities;
  int       m_max_pursuit_dist_sq;    
  BuffNonRefIndexArray m_buffs;
  byte m_activity_ownership_preference;
  byte m_activity_starts_on_space_load;
};


class PartBehavior : public Part 
{

public:
  inline         PartBehavior  (void) : m_last_behaviors(MEM_PARTS), m_current_behaviors(MEM_PARTS), m_restored_current_behaviors(MEM_PARTS), m_allowed_activities(MEM_PARTS), m_variable_overrides(MEM_PARTS), m_valid_transition_talents(MEM_PARTS) { wipe_data(); }
	inline virtual ~PartBehavior (void) { dealloc_memory(); }

	//COMMON INIT FUNCTIONS
  virtual void init       (const dword part_index, const ObjectHandle o, const PartEnum part_type);
  virtual void derived_init (void);
  virtual void clone_part_data (const Part *type_part);
  virtual void copy_data_from_blueprint (const PartBlueprint* part_blueprint);
  virtual void post_setup (void);
  virtual void post_restore (void);
	void         close      (void);

  virtual void  stop_processing (void)               const;


	//PUBLIC ENUM AND STRUCT DEFINITIONS
  enum ActivityOwnershipPreference {
    ALL = 0,
    ONLY_OWNED = 1,
    PREFER_OWNED = 2
  };


	//PUBLIC INTERFACE
  virtual void process   (const dword& dt);

#if ALLOW_PARTS_DEBUG
  String       get_last_behavior          (void) const     { return m_last_behavior_name; }
  unsigned int get_times_in_last_behavior (void) const     { return m_last_behavior_count; }
#endif

  bool         get_can_process (void) const                 { return m_flags.has_index(ALLOW_BEHAVIOR_PROCESSING); }
  void         set_can_process (bool flag = true)           { m_flags.set_index(ALLOW_BEHAVIOR_PROCESSING, flag); }
  void         set_next_behavior_update   (int time_in_ms)  { Part::request_process(time_in_ms); }

  bool         get_activity_starts_on_space_load (void) const                 { return m_flags.has_index(ACTIVITIES_START_ON_SPACE_LOAD); }
  void         set_activity_starts_on_space_load (bool flag = true)           { m_flags.set_index(ACTIVITIES_START_ON_SPACE_LOAD, flag); }

  inline bool  requires_post_setup (void) const { return m_flags.has_index(REQUIRES_POST_SETUP); }
  inline void  set_requires_post_setup (bool flag = true) { m_flags.set_index(REQUIRES_POST_SETUP, flag); }

  bool uses_activities (void) const { return (m_allowed_activities.len() == 0) ? (false) : (true); }
  bool is_allowed_activity (const int activity_type) const;

  byte   get_requested_priority (void) const   { return m_requested_priority; }

  bool is_talent_available (const TalentKey& talent, ObjectHandle target_object) const;
  const String why_is_talent_available (const TalentKey& talent, ObjectHandle target_object) const;
  void activate_motion_for_talent (const TalentKey& talent);

  bool forget_about_attack_weight_and_cooldown (TalentNonRefIndex talent_index, ObjectHandle target);
  void release_attack_weight_and_cooldown (void);

  void on_event (const MotionTreeEvent& event);

  void  set_behavior_tree (const BehaviorTreeNonRefIndex new_tree, const SimpleArray<BehaviorOverrideKey>& variable_overrides, const IntArray& allowed_activities);
  const BehaviorTreeNonRefIndex get_behavior_tree_index (void) const { return m_behavior_tree_index; }

  const int get_activity_ownership_preference (void) const { return m_activity_ownership_preference; }

  void clear_behavior_overrides (void);

  // LEASHING
  inline bool is_outside_leashing_range (const Point& point) const { return m_leash_memory.is_outside_leashing_range(point); }

  inline void update_leashing_home_location (void) { m_leash_memory.update_home_location(o); }  
  inline void set_leashing_home_object    (ObjectHandle new_home_object) { m_leash_memory.set_home_object(o, new_home_object); }
  inline void clear_leashing_home_object  (void) { m_leash_memory.clear_home_object(o); }

  inline void set_leashing_max_pursuit_distance (int dist_in_centimeters_squared) { m_leash_memory.set_max_pursuit_distance(dist_in_centimeters_squared); }

  virtual void  internal_save_data    (MemoryDataWalk* stream, BundleSaveDataArray* out_required_bundles) const;
  virtual void  internal_restore_data (MemoryDataWalk* stream);


private:
	//PRIVATE ENUM AND STRUCT DEFINITIONS
  enum {
    ALLOW_BEHAVIOR_PROCESSING,
    ACTIVITIES_START_ON_SPACE_LOAD,
    REQUIRES_POST_SETUP,
    NUM_FLAGS
  };

  //PRIVATE FUNCTIONS  
  void init_behavior_tree (bool only_update_overrides = false);
  void setup_motion_tree_event_listener (void);
  void use_overrides (bool onoff);

  void apply_attack_weight_and_cooldown (TalentIndex &talent_index);
    
  void set_valid_transition_talents (int stack_item_unique_id);

  bool add_motion_for_talent (TalentNonRefIndex talent_index);
  
  bool is_valid_transition_talent (TalentNonRefIndex talent_index) const;
  bool is_current_unique_id_accurate (const MotionTreeInstance& motion_tree_instance) const;
  void get_prep_zone_index_and_gate_index_from_talent_index (TalentNonRefIndex talent_index, int* gate_index_out, int* prep_zone_index_out) const;

	//COMMON PRIVATE INIT FUNCTIONS
	void wipe_data      (void);
	void dealloc_memory (void);

public:  //PUBLIC DATA

private:  //PRIVATE DATA
  BehaviorTreeIndex       m_behavior_tree_index;
  BehaviorTree*           m_behavior_tree;

  IntArray m_allowed_activities;
  LeashMemory m_leash_memory;

  SimpleArray<BehaviorOverrideKey> m_variable_overrides;
  const SimpleArray<BehaviorOverrideKey>* m_schedule_variable_overrides;

  TalentNonRefIndexArray m_valid_transition_talents;

  EventListener<MotionTreeEvent> m_motion_tree_listener;

  mutable DynamicBitArray m_last_behaviors;
  mutable DynamicBitArray m_current_behaviors;
  BehaviorKeyArray m_restored_current_behaviors;  // need to store this until the post_restore step

  ObjectHandle m_cur_attack_weight_target;
  TalentIndex m_cur_attack_talent_index;

  static dword recursion_calls;

  int m_current_unique_id;
  byte m_activity_ownership_preference;
  byte m_requested_priority;
  BitMask8 m_flags;

  friend class BehaviorDebugWin;

#if ALLOW_PARTS_DEBUG
  String  m_last_behavior_name;
  unsigned int m_last_behavior_count;
#endif

}; // End PartBehavior


//==============================================================================
// INLINES
//==============================================================================


} //namespace BHG
#endif // _PARTBEHAVIOR_H_



