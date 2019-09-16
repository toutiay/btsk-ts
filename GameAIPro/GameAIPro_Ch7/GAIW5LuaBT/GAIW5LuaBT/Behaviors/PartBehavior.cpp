//==============================================================================
void PartBehavior::stop_processing (void) const
{
  // run any on_exits()
  for (int scan = 0; scan < m_current_behaviors.get_num_bits(); scan++) {
    if (m_current_behaviors.get(scan)) {
      // found a behavior that needs to be closed out
      m_behavior_tree->run_on_exit(scan, o);
    }
  }
  m_last_behaviors.clear();
  m_current_behaviors.clear();

  Part::stop_processing();
}



//==============================================================================
void PartBehavior::set_behavior_tree (const BehaviorTreeNonRefIndex new_tree, const SimpleArray<BehaviorOverrideKey>& variable_overrides, const IntArray& allowed_activities)
{
  m_schedule_variable_overrides = &variable_overrides;
  m_allowed_activities = allowed_activities;

  clear_behavior_overrides();

  if (m_behavior_tree_index != new_tree) {
    //clean up the currently-running tree
    stop_processing();

    m_behavior_tree_index = new_tree;
    init_behavior_tree();
  } else {
    init_behavior_tree(ON("only_update_overrides"));
  }

#if ALLOW_PARTS_DEBUG
  for (int scan = 0; scan < m_schedule_variable_overrides->len(); ++scan) {
    if (!(GET_BEHAVIOR_OVERRIDE_SCRIPT_INDEX((*m_schedule_variable_overrides)[scan]).is_valid())) {
      WARN(WARN_GAME, SC("Object $NAME0 has invalid script asset for behavior dbid $NUM1", 211912820, 209853892), GAME_OBJ(o).get_display_name(), m_behavior_tree_index.get_database_id());
    }
  }
#endif

  request_process();
}


//==============================================================================
void PartBehavior::clear_behavior_overrides(void)
{
  if (m_behavior_tree)
  {
    m_behavior_tree->clear_behavior_overrides(o);
  }
}

//==============================================================================
void PartBehavior::process (const dword& dt)
{
  MEM_SCOPE("PartBehavior");
  CPU_PROFILE_MARKER_SCOPE("PartBehaviorProcess");

  PROFMON_SCOPE_CAT (PartBehavior, CATEGORY_PARTS);

  static dword process_count = 0;
  Behavior::num_lua_calls = 0;
  recursion_calls = 0;

  //Check to see if we're in the middle of an animation
  if (!get_can_process()) {
    //We're in the middle of an animation that doesn't allow behavior updates. Try again in a little bit
    Part::request_process(250);
    return;
  }      

  // Leash this Object to a HomeLocation when it's in combat.
  if (m_leash_memory.process(o)) {    
    Part::request_process(2000);

    // This NPC is returning home. Turn off behaviors until it reaches home. [9/4/2009 matt.gray]
    return;
  }

  // reset your requested priority
  m_requested_priority = 0;


  m_last_behaviors = m_current_behaviors;
  m_behavior_tree->find_active_behaviors(o, m_current_behaviors, m_last_behaviors);

  int scan;
  // run any on_exits()
  for (scan = 0; scan < m_last_behaviors.get_num_bits(); scan++) {
    if (m_last_behaviors.get(scan)) {
      if (m_current_behaviors.get(scan) == 0) {
        // found a behavior that is finished
        m_behavior_tree->run_on_exit(scan, o);
      }
    }
  }

#if ALLOW_PARTS_DEBUG
  String all_behaviors;
#endif

  int last = -1;
  // run the behaviors, calling on_enter if it is a new behavior
  for (scan = 0; scan < m_current_behaviors.get_num_bits(); scan++) {
    if (m_current_behaviors.get(scan)) {
#if ALLOW_PARTS_DEBUG
      if (DEBUG_TEXT_MGR.can_add(DebugOptions::DEBUG_LVL_1)) { 
        all_behaviors += m_behavior_tree->get_behavior_name(scan);
        all_behaviors.add_lf();
      }
#endif
      if (m_last_behaviors.get(scan) == 0) {
        // found a behavior that is brand new!
        m_behavior_tree->run_on_enter(scan, o);
      }
      
      // figure out the maximum requested priority
      m_requested_priority = bhmax(m_requested_priority, m_behavior_tree->get_behavior_priority(scan));

      m_behavior_tree->run_on_process(scan, o);
      
      last = scan;
    }
  }

#if ALLOW_PARTS_DEBUG
  if (DEBUG_TEXT_MGR.can_add(DebugOptions::DEBUG_LVL_1)) { 
    DEBUG_TEXT_MGR.set(o, SC("behaviors running", 15500345, 15500345), all_behaviors);
  }
#endif

  // These checks for o > 0 and o != 0 are necessary so long as an object can die in the middle of running a behavior. [12/5/2008 michael.dawe]
  // If they aren't there, an object killing itself from a behavior will crash the game.
  // Pancake is looking into delaying a death request until the end of a frame; if that goes in, these checks can be eliminated
  if (o > 0 && last >= 0) {
    Part::request_process(250);

#if ALLOW_PARTS_DEBUG
    m_last_behavior_name = m_behavior_tree->get_behavior_name(last);
    // if it was running, keep track of how many times
    if (m_last_behaviors.get(last)) {
      m_last_behavior_count++;
    } else {
      m_last_behavior_count = 0;
    }
#endif
    if (m_behavior_tree->is_behavior_nonexclusive(last)) {
      WARN(WARN_GAME, SC("PartBehavior: $NAME0: last behavior was nonexclusive", 186779062, 189296828), GAME_OBJ(o).get_actual_display_name());
      stop_processing();
    }
    if (m_behavior_tree->is_behavior_parent(last)) {
      WARN(WARN_GAME, SC("PartBehavior: $NAME0: Last PRIORITY behavior was not a leaf node", 201698121, 207655707), GAME_OBJ(o).get_actual_display_name());
      stop_processing();
    }
  } else if (o != 0) {
    WARN(WARN_GAME, SC("PartBehavior: $NAME0 has no valid running behavior??", 181011274, 183529040), GAME_OBJ(o).get_actual_display_name());
    stop_processing();
#if ALLOW_PARTS_DEBUG
    m_last_behavior_name = EMPTY_STRING;
    m_last_behavior_count = 0;
#endif
  }

  //Disable Overrides
  use_overrides(OFF("onoff"));

#if USE_FUNCTION_PROFILER && 0
  dword ms = profile_monitor_get_ms_since(start_event);

  static dword count = 0;
  if (ms > 4) {
    OUTPUT_DEBUG_STRING(String().parse("PartBehavior ms = $NUM0; o = $NUM1, behavior = $NAME2, num_lua_calls = $NUM3, count = $NUM4, process_count = $NUM5\n", ms, o, m_current_behavior->data->get_name(), Behavior::num_lua_calls, count++, process_count).str());
    OUTPUT_DEBUG_STRING(String().parse("object name = $NAME0, Recursion calls = $NUM1, \n", GAME_OBJ(o).get_display_name(), recursion_calls).str());
  }
#endif
  ++process_count;

#if ALLOW_PARTS_DEBUG
  if ((DEBUG_TEXT_MGR.can_add(DebugOptions::DEBUG_LVL_3) || BEHAVIOR_DEBUG_TEXT_MGR.is_accepting_messages()) && o != 0) { //o could be 0 if the behavior just killed the object
    String debug_string = String().parse("$NAME0: ($NUM1): Behavior: $NAME2", GAME_OBJ(o).get_display_name(), o, m_last_behavior_name);
    DEBUG_TEXT_MGR.set(o, SC("behavior", 3291722, 3291722), debug_string);
    BEHAVIOR_DEBUG_TEXT_MGR.add(debug_string);
  }
#endif
}


