#include "pch.h"
#include "jedi_ai_actions.h"
#include "jedi_ai_memory.h"
#include "jedi.h"


/////////////////////////////////////////////////////////////////////////////
//
// globals
//
/////////////////////////////////////////////////////////////////////////////

// skill level constraints
CJediAiActionConstraintSkillLevel highSkillLevelConstraint(0.25f, 1.0f);
CJediAiActionConstraintSkillLevel lowSkillLevelConstraint(0.0f, 0.75f);
CJediAiActionConstraintSkillLevel padawanSkillLevelConstraint(0.0f, 0.25f);

// kill timer constraints
CJediAiActionConstraintKillTimer aboveDesiredKillTimerConstraint(CJediAiActionConstraintKillTimer::kKillTimeDesired, CJediAiActionConstraintKillTimer::kKillTimeIgnored, true);
CJediAiActionConstraintKillTimer belowDesiredKillTimerConstraint(CJediAiActionConstraintKillTimer::kKillTimeIgnored, CJediAiActionConstraintKillTimer::kKillTimeDesired, true);
CJediAiActionConstraintKillTimer belowDesiredKillTimerHighSkillLevelConstraint(CJediAiActionConstraintKillTimer::kKillTimeIgnored, CJediAiActionConstraintKillTimer::kKillTimeDesired, true);
CJediAiActionConstraintKillTimer belowDesiredKillTimerLowSkillLevelConstraint(CJediAiActionConstraintKillTimer::kKillTimeIgnored, CJediAiActionConstraintKillTimer::kKillTimeDesired, true);

// initialize our global constraints
static struct SInitGlobalConstraints {
	SInitGlobalConstraints() {
		belowDesiredKillTimerHighSkillLevelConstraint.nextConstraint = &highSkillLevelConstraint;
		belowDesiredKillTimerLowSkillLevelConstraint.nextConstraint = &lowSkillLevelConstraint;
	}
} __initGlobalConstraints;

static void incrementTimer(float &timer, float dt, float timerMax) {
	timer += dt;
	if (timer > timerMax) {
		timer = timerMax;
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// jedi action base class
//
/////////////////////////////////////////////////////////////////////////////

CJediAiAction::CJediAiAction() {
	name = NULL;
	memory = NULL;
	constraint = NULL;
	lastRunTime = 0.0f;
	minRunFrequency = 0.0f;
	memset(&simSummary, 0, sizeof(simSummary));
	flags = 0;
}

void CJediAiAction::init(CJediAiMemory *newWorldState) {
	memory = newWorldState;
}

bool CJediAiAction::ensureInitialization() const {
	if (memory == NULL) {
		EJediAiAction actionType = getType();
		const char *actionTypeName = lookupJediAiActionName(actionType);
		error("CJediAiAction::ensureInitialization() - action type '%s' hasn't been initialized!", actionTypeName);
		return false;
	}
	return true;
}

void CJediAiAction::reset() {
	lastRunTime = 0.0f;
	minRunFrequency = 0.0f;
	memset(&simSummary, 0, sizeof(simSummary));
	if (constraint != NULL) {
		constraint->reset();
	}
}

EJediAiActionResult CJediAiAction::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if we have a specific 'run' frequency and we've been run too recently, I can't run
	if (minRunFrequency > 0.0f && lastRunTime != 0.0f && !isInProgress()) {
		float timeSinceLastRun = (simMemory.currentTime - lastRunTime);
		if (timeSinceLastRun < minRunFrequency) {
			return eJediAiActionResult_Failure;
		}
	}

	// if I am dead, I can't do this
	if (simMemory.selfState.hitPoints <= 0.0f) {
		return eJediAiActionResult_Failure;
	}

	// check our constraints
	const CJediAiActionConstraint *nextConstraint = constraint;
	while (nextConstraint != NULL) {
		EJediAiActionResult result = nextConstraint->checkConstraint(simMemory, *this, simulating);
		if (result != eJediAiActionResult_InProgress) {
			return result;
		}
		nextConstraint = nextConstraint->nextConstraint;
	}

	// constraint passed
	return eJediAiActionResult_InProgress;
}

EJediAiActionResult CJediAiAction::onBegin() {

	// make sure we are initialized
	if (!ensureInitialization()) {
		return eJediAiActionResult_Failure;
	}

	// check our constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// we are now in progress
	flags |= kFlag_InProgress;

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiAction::onEnd() {

	// save off the time when we ended
	lastRunTime = memory->currentTime;

	// we are no longer in progress
	flags &= ~kFlag_InProgress;
}

void CJediAiAction::updateTimers(float dt) {
}

const char *CJediAiAction::getName() const {
	return (name ? name : lookupJediAiActionName(getType()));
}

bool CJediAiAction::isInProgress() const {
	return (flags & kFlag_InProgress);
}

bool CJediAiAction::isNotSelectable() const {
	return ((flags & kFlag_IsNotSelectable) != 0);
}

void CJediAiAction::setIsNotSelectable(bool notSelectable) {
	if (notSelectable) {
		flags |= kFlag_IsNotSelectable;
	} else {
		flags &= ~kFlag_IsNotSelectable;
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// multi-action
//
/////////////////////////////////////////////////////////////////////////////

void CJediAiActionComposite::init(CJediAiMemory *newWorldState) {

	// base class version
	BASECLASS::init(newWorldState);

	// init all actions
	int actionCount = 0;
	CJediAiAction **actionTable = getActionTable(&actionCount);
	if (actionTable != NULL) {
		for (int i = 0; i < actionCount; ++i) {
			CJediAiAction *action = actionTable[i];
			if (action != NULL) {
				action->init(newWorldState);
			}
		}
	}
}

void CJediAiActionComposite::reset() {

	// base class version
	BASECLASS::reset();

	// reset all actions
	int actionCount = 0;
	CJediAiAction **actionTable = getActionTable(&actionCount);
	if (actionTable != NULL) {
		for (int i = 0; i < actionCount; ++i) {
			CJediAiAction *action = actionTable[i];
			if (action != NULL) {
				action->reset();
			}
		}
	}
}

bool CJediAiActionComposite::isNotSelectable() const {

	// base class version
	if (BASECLASS::isNotSelectable()) {
		return true;
	}

	// if any of my subactions are selectable, I am as well
	int actionCount = 0;
	CJediAiAction *const *actionTable = getActionTable(&actionCount);
	if (actionTable != NULL) {
		for (int i = 0; i < actionCount; ++i) {
			CJediAiAction *action = actionTable[i];
			if (action != NULL && !action->isNotSelectable()) {
				return false;
			}
		}
	}

	// none of my subactions are selectable, so I am not either
	return true;
}

CJediAiAction *const *CJediAiActionComposite::getActionTable(int *actionCount) const {
	CJediAiActionComposite *me = const_cast<CJediAiActionComposite*>(this);
	return me->getActionTable(actionCount);
}

CJediAiAction *CJediAiActionComposite::getAction(int index) {

	// check params
	if (index < 0) {
		return NULL;
	}

	// get the action table
	int actionCount = 0;
	CJediAiAction **actionTable = getActionTable(&actionCount);
	if (actionTable == NULL || index >= actionCount) {
		return NULL;
	}

	// get the action
	return actionTable[index];
}

const CJediAiAction *CJediAiActionComposite::getAction(int index) const {
	CJediAiActionComposite *me = const_cast<CJediAiActionComposite*>(this);
	return me->getAction(index);
}


/////////////////////////////////////////////////////////////////////////////
//
// parallel
//
/////////////////////////////////////////////////////////////////////////////

EJediAiAction CJediAiActionParallelBase::getType() const {
	return eJediAiAction_Parallel;
}

void CJediAiActionParallelBase::reset() {

	// base class version
	BASECLASS::reset();

	// mark each subaction's result as 'in progress'
	int actionResultCount = 0;
	EJediAiActionResult *actionResultTable = getActionResultTable(&actionResultCount);
	if (actionResultTable != NULL) {
		for (int i = 0; i < actionResultCount; ++i) {
			actionResultTable[i] = eJediAiActionResult_InProgress;
		}
	}
}

EJediAiActionResult CJediAiActionParallelBase::onBegin() {

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// begin each sub-action
	int actionCount = 0;
	CJediAiAction *const *actionTable = getActionTable(&actionCount);
	EJediAiActionResult *actionResultTable = getActionResultTable(NULL);
	int actionResultCountTable[eJediAiActionResult_Count] = {};
	if (actionTable != NULL) {
		for (int i = 0; i < actionCount; ++i) {
			CJediAiAction *action = actionTable[i];
			if (action != NULL) {
				EJediAiActionResult result = action->onBegin();
				++actionResultCountTable[result];
				if (actionResultTable) {
					actionResultTable[i] = result;
				}
			}
		}
	}

	// if all actions failed, we've failed
	if (actionResultCountTable[eJediAiActionResult_Failure] >= actionCount) {
		return eJediAiActionResult_Failure;
	}

	// if all actions succeeded, we've succeded
	if (actionResultCountTable[eJediAiActionResult_Success] >= actionCount) {
		return eJediAiActionResult_Success;
	}

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionParallelBase::onEnd() {

	// end each sub-action
	int actionCount = 0;
	CJediAiAction *const *actionTable = getActionTable(&actionCount);
	EJediAiActionResult *actionResultTable = getActionResultTable(NULL);
	if (actionTable != NULL) {
		for (int i = (actionCount - 1); i >= 0; --i) {
			CJediAiAction *action = actionTable[i];
			if (action != NULL) {
				action->onEnd();
			}
			if (actionResultTable != NULL) {
				actionResultTable[i] = eJediAiActionResult_InProgress;
			}
		}
	}

	// base class version
	BASECLASS::onEnd();
}

void CJediAiActionParallelBase::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// if I am dead, I can't do this
	if (simMemory.selfState.hitPoints <= 0.0f) {
		return;
	}

	// if I am knocked around, I can't do this
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		setSimSummary(simSummary, simMemory);
		return;
	}

	// get my action table
	int actionCount = 0;
	CJediAiAction *const *actionTable = getActionTable(&actionCount);
	if (actionTable == NULL || actionCount <= 0) {
		return;
	}

	// get my action result table
	EJediAiActionResult *actionResultTable = getActionResultTable(NULL);
	int actionResultCounts[eJediAiActionResult_Count] = {};

	// simulate each sub-action
	for (int i = 0; i < actionCount; ++i) {

		// get the action
		CJediAiAction *action = actionTable[i];
		if (action == NULL) {
			++actionResultCounts[eJediAiActionResult_Failure];
			continue;
		}

		// if this action isn't still in progress, skip it
		if (actionResultTable != NULL && actionResultTable[i] != eJediAiActionResult_InProgress) {
			if (doesActionLoop(i)) {
				actionResultTable[i] = eJediAiActionResult_InProgress;
			} else {
				++actionResultCounts[actionResultTable[i]];
				continue;
			}
		}

		// simulate the action and save off its result in our table
		// TODO: implement parallel simulation support
		action->simulate(simMemory);
	}

	// if I am dead, I've failed
	if (simMemory.selfState.hitPoints <= 0.0f) {
		simSummary.result = eJediAiActionSimResult_Deadly;
		return;
	}

	// if all of our actions have failed, we've failed
	if (actionResultCounts[eJediAiActionResult_Failure] >= actionCount) {
		simSummary.result = eJediAiActionSimResult_Impossible;
		return;
	}

	// if all of our actions have succeeded, we won't do anything
	if (actionResultCounts[eJediAiActionResult_Success] >= actionCount) {
		simSummary.result = eJediAiActionSimResult_Irrelevant;
		return;
	}

	// success
	setSimSummary(simSummary, simMemory);
}

void CJediAiActionParallelBase::updateTimers(float dt) {

	// update each sub-action
	int actionCount = 0;
	CJediAiAction **actionTable = getActionTable(&actionCount);
	if (actionTable != NULL) {
		for (int i = 0; i < actionCount; ++i) {
			CJediAiAction *action = actionTable[i];
			if (action != NULL) {
				action->updateTimers(dt);
			}
		}
	}
}

EJediAiActionResult CJediAiActionParallelBase::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// get my action result table
	int actionResultCount = 0;
	EJediAiActionResult *actionResultTable = getActionResultTable(&actionResultCount);
	int actionResultCounts[eJediAiActionResult_Count] = {0};

	// update each sub-action
	int actionCount = 0;
	CJediAiAction *const *actionTable = getActionTable(&actionCount);
	if (actionTable != NULL) {
		for (int i = 0; i < actionCount; ++i) {
			CJediAiAction *action = actionTable[i];
			if (action != NULL) {

				// if this action isn't still in progress, skip it
				if (actionResultTable != NULL && actionResultTable[i] != eJediAiActionResult_InProgress) {
					if (!doesActionLoop(i)) {
						++actionResultCounts[actionResultTable[i]];
						continue;
					}
				}

				// update the action and save off its result in our table
				EJediAiActionResult result = action->update(dt);
				if (actionResultTable != NULL) {
					actionResultTable[i] = result;
				}
				++actionResultCounts[result];
			}
		}
	}

	// if all of our actions have failed, we've failed
	if (actionResultCounts[eJediAiActionResult_Failure] >= actionCount) {
		return eJediAiActionResult_Failure;
	}

	// if any of our actions are still in progress, we are still in progress
	if (actionResultCounts[eJediAiActionResult_InProgress] > 0) {
		return eJediAiActionResult_InProgress;
	}

	// all actions are completed, some successfully, so we've succeeded
	return eJediAiActionResult_Success;
}

EJediAiActionResult const *CJediAiActionParallelBase::getActionResultTable(int *actionResultCount) const {
	CJediAiActionParallelBase *me = const_cast<CJediAiActionParallelBase*>(this);
	return me->getActionResultTable(actionResultCount);
}

bool CJediAiActionParallelBase::doesActionLoop(int actionIndex) const {
	return false;
}


/////////////////////////////////////////////////////////////////////////////
//
// sequence
//
/////////////////////////////////////////////////////////////////////////////

EJediAiAction CJediAiActionSequenceBase::getType() const {
	return eJediAiAction_Sequence;
}

void CJediAiActionSequenceBase::reset() {

	// base class version
	BASECLASS::reset();

	// reset our params and data
	memset(&params, 0, sizeof(params));
	memset(&data, 0, sizeof(data));
}

EJediAiActionResult CJediAiActionSequenceBase::onBegin() {

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// reset my data
	memset(&data, 0, sizeof(data));

	// begin the first action in my sequence
	result = beginNextAction();
	return result;
}

void CJediAiActionSequenceBase::onEnd() {

	// end any current action
	if (data.currentAction != NULL) {
		data.currentAction->onEnd();
	}

	// reset my data
	memset(&data, 0, sizeof(data));

	// base class version
	BASECLASS::onEnd();
}

void CJediAiActionSequenceBase::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// if I am dead, I can't do this
	if (simMemory.selfState.hitPoints <= 0.0f) {
		return;
	}

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return;
	}

	// get our starting point
	// if we aren't yet started, start at the beginning
	int nextActionIndex = data.nextActionIndex;
	CJediAiAction *currentAction = data.currentAction;
	if (currentAction == NULL) {
		nextActionIndex = 0;
		currentAction = getNextAction(nextActionIndex);
	}

	// simulate the rest of my actions
	EJediAiActionSimResult bestSimResult = eJediAiActionSimResult_Irrelevant;
	SJediAiActionSimSummary *lastSimSummary = NULL;
	float timeBetweenActions = (params.timeBetweenActions - data.timer);
	while (currentAction != NULL) {

		// if we have any time between actions, simulate that
		if (nextActionIndex > 1 && timeBetweenActions < params.timeBetweenActions) {
			simMemory.simulate(timeBetweenActions, CJediAiMemory::SSimulateParams());
		}
		timeBetweenActions = params.timeBetweenActions;

		// simulate
		currentAction->simulate(simMemory);
		if (bestSimResult < currentAction->simSummary.result) {
			bestSimResult = currentAction->simSummary.result;
		}

		// if this action is impossible, I've failed
		if (!params.allowActionFailure && currentAction->simSummary.result <= params.minFailureResult) {
			simSummary = currentAction->simSummary;
			return;
		}

		// if I am dead, I've failed
		if (simMemory.selfState.hitPoints <= 0.0f) {
			break;
		}

		// if I am knocked around, fail
		if (simMemory.isSelfInState(eJediState_KnockedAround)) {
			return;
		}

		// move on to the next action
		currentAction = getNextAction(nextActionIndex);
	}

	// success
	setSimSummary(simSummary, simMemory);
	if (simSummary.result == eJediAiActionSimResult_Irrelevant && bestSimResult > eJediAiActionSimResult_Irrelevant) {
		simSummary.result = bestSimResult;
	}
}

void CJediAiActionSequenceBase::updateTimers(float dt) {

	// update my timer
	if (data.currentAction == NULL) {
		incrementTimer(data.timer, dt, params.timeBetweenActions);
		return;
	}

	// update my current action
	data.currentAction->updateTimers(dt);
}

EJediAiActionResult CJediAiActionSequenceBase::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if I have an action, update it
	if (data.currentAction != NULL) {
		EJediAiActionResult result = data.currentAction->update(dt);

		// if the action is still in progress, we are still in progress
		if (result == eJediAiActionResult_InProgress) {
			return eJediAiActionResult_InProgress;
		}

		// we are done with this action
		data.currentAction->onEnd();
		data.currentAction = NULL;
		data.timer = 0.0f;

		// if the action failed, we failed
		if (!params.allowActionFailure && result == eJediAiActionResult_Failure) {
			return eJediAiActionResult_Failure;
		}
	}

	// if our delay timer hasn't expired, we are still in progress
	if (data.timer < params.timeBetweenActions) {
		return eJediAiActionResult_InProgress;
	}

	// move on to the next action
	result = beginNextAction();
	return result;
}

CJediAiAction *CJediAiActionSequenceBase::getNextAction(int &nextActionIndex) const {

	// check params
	if (nextActionIndex < 0) {
		nextActionIndex = 0;
	}

	// get my action table
	// if we don't have one, return NULL
	int actionCount = 0;
	CJediAiAction *const *actionTable = getActionTable(&actionCount);
	if (actionTable == NULL) {
		return NULL;
	}

	// if we are looping and our index is out of bounds, loop back around
	if (params.loop && nextActionIndex >= actionCount) {
		nextActionIndex = 0;
	}

	// if we are out of actions, bail
	if (nextActionIndex >= actionCount) {
		return NULL;
	}

	// loop through the table until we find an available action
	while (nextActionIndex < actionCount) {
		int actionIndex = nextActionIndex++;
		CJediAiAction *action = actionTable[actionIndex];
		if (action != NULL) {
			return action;
		}
	}

	// we didn't find one
	return NULL;
}

EJediAiActionResult CJediAiActionSequenceBase::beginNextAction() {

	// clear my data
	CJediAiAction *currentAction = data.currentAction;
	int nextActionIndex = data.nextActionIndex;
	memset(&data, 0, sizeof(data));
	data.nextActionIndex = nextActionIndex;
	data.currentAction = currentAction;

	// move on to the next action
	EJediAiActionResult result = eJediAiActionResult_Failure;
	while (result != eJediAiActionResult_InProgress) {

		// if I have a current action, notify it that it is ended
		if (data.currentAction != NULL) {
			data.currentAction->onEnd();
		}

		// get the next action
		data.currentAction = getNextAction(data.nextActionIndex);
		if (data.currentAction == NULL) {
			return eJediAiActionResult_Success;
		}

		// begin the action
		// if the action failed and we care about such things, we failed
		result = data.currentAction->onBegin();
		if (!params.allowActionFailure && result == eJediAiActionResult_Failure) {
			return eJediAiActionResult_Failure;
		}
	}

	// success!
	return eJediAiActionResult_InProgress;
}

bool CJediAiActionSequenceBase::isNotSelectable() const {

	// base class version
	if (BASECLASS::isNotSelectable()) {
		return true;
	}

	// if any of my subactions are not selectable, I am not either
	int actionCount = 0;
	CJediAiAction *const *actionTable = getActionTable(&actionCount);
	if (actionTable != NULL) {
		for (int i = 0; i < actionCount; ++i) {
			CJediAiAction *action = actionTable[i];
			if (action != NULL && action->isNotSelectable()) {
				return true;
			}
		}
	}

	// all of my subactions are selectable, so I am as well
	return false;
}


/////////////////////////////////////////////////////////////////////////////
//
// selector
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionSelectorBase::CJediAiActionSelectorBase() {
}

EJediAiAction CJediAiActionSelectorBase::getType() const {
	return eJediAiAction_Selector;
}

void CJediAiActionSelectorBase::reset() {

	// base class version
	BASECLASS::reset();

	// reset my params and data
	memset(&selectorParams, 0, sizeof(selectorParams));
	memset(&selectorData, 0, sizeof(selectorData));

	// apply default values
	selectorParams.selectFrequency = -1.0f;
	selectorParams.ifEqualUseCurrentAction = true;
	selectorData.currentActionResult = eJediAiActionResult_Failure;
}

EJediAiActionResult CJediAiActionSelectorBase::onBegin() {

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// clear my selector data
	SSelectorData prevSelectorData = selectorData;
	memset(&selectorData, 0, sizeof(selectorData));
	selectorData.debouncedAction = prevSelectorData.debouncedAction;
	selectorData.bestAction = prevSelectorData.bestAction;
	selectorData.currentActionResult = eJediAiActionResult_Failure;

	// if we don't already one, choose our best option
	if (selectorData.bestAction == NULL) {
		selectorData.bestAction = selectAction(NULL);
	}

	// begin our best action
	result = setCurrentAction(selectorData.bestAction);
	return result;
}

void CJediAiActionSelectorBase::onEnd() {

	// base class version
	BASECLASS::onEnd();

	// clear my current action
	setCurrentAction(NULL);
}

void CJediAiActionSelectorBase::simulate(CJediAiMemory &simMemory) {

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		initSimSummary(simSummary, simMemory);
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// if I have an action and my select timer hasn't expired, just simulate it
	bool timerExpired = (selectorParams.selectFrequency >= 0.0f && selectorData.selectTimer >= selectorParams.selectFrequency);
	if (selectorData.currentAction != NULL && !timerExpired) {
		selectorData.currentAction->simulate(simMemory);
		simSummary = selectorData.currentAction->simSummary;
		return;
	}

	// evaluate my actions
	// if I already have a 'best action', just simulate it
	selectorData.bestAction = selectAction(&simMemory);
	if (selectorData.bestAction == NULL) {
		initSimSummary(simSummary, simMemory);
	} else {
		simSummary = selectorData.bestAction->simSummary;
	}
}

void CJediAiActionSelectorBase::updateTimers(float dt) {

	// if I have no current action, bail
	if (selectorData.currentAction == NULL) {
		return;
	}

	// update my timer
	if (selectorData.currentActionResult == eJediAiActionResult_InProgress) {
		if (selectorParams.selectFrequency >= 0.0f) {
			incrementTimer(selectorData.selectTimer, dt, selectorParams.selectFrequency);
		}
	}

	// update my current action
	selectorData.currentAction->updateTimers(dt);
}

EJediAiActionResult CJediAiActionSelectorBase::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if my select timer has expired, select a new action
	if (selectorParams.selectFrequency >= 0.0f) {
		if (selectorData.selectTimer >= selectorParams.selectFrequency) {

			// if I don't have a best action, get one
			if (selectorData.bestAction == NULL) {
				selectorData.bestAction = selectAction(NULL);
			}

			// select the new action
			CJediAiAction *prevAction = selectorData.currentAction;
			setCurrentAction(selectorData.bestAction);

			// if we didn't get a new action, we've failed
			if (selectorData.currentAction == NULL) {
				return eJediAiActionResult_Failure;
			}

			// if we selected a new action, don't update it this frame
			if (prevAction != selectorData.currentAction) {
				return eJediAiActionResult_InProgress;
			}
		}
	}

	// if I have no current action, I've failed
	if (selectorData.currentAction == NULL) {
		return eJediAiActionResult_Failure;
	}

	// update my current action
	selectorData.currentActionResult = selectorData.currentAction->update(dt);
	return selectorData.currentActionResult;
}

EJediAiActionResult CJediAiActionSelectorBase::setCurrentAction(CJediAiAction *action) {

	// clear our current 'best action'
	selectorData.bestAction = NULL;

	// if the new action is already my current action and is still in progress,
	// just let it keep working
	if (action == selectorData.currentAction && selectorData.currentActionResult == eJediAiActionResult_InProgress) {
		selectorData.selectTimer = 0.0f;
		return eJediAiActionResult_InProgress;
	}

	// end my current action
	if (selectorData.currentAction != NULL) {
		selectorData.debouncedAction = selectorData.currentAction;
		selectorData.currentAction->onEnd();
	}

	// set the new action
	selectorData.currentActionResult = eJediAiActionResult_Failure;
	selectorData.currentAction = action;

	// begin the new action
	if (selectorData.currentAction != NULL) {
		selectorData.currentActionResult = selectorData.currentAction->onBegin();
		selectorData.selectTimer = 0.0f;
		selectorData.debouncedAction = NULL;
	}

	// return the new action's result
	return selectorData.currentActionResult;
}

CJediAiAction *CJediAiActionSelectorBase::selectAction(CJediAiMemory *simMemory) const {

	// get my action table
	int actionCount = 0;
	CJediAiAction *const *actionTable = getActionTable(&actionCount);
	if (actionTable == NULL || actionCount < 0) {
		return NULL;
	}

	// if we are extracting simulation memory of our best action, create a memory table
	// to hold each action's sim memory until we choose one
	CJediAiMemory *memoryTable = NULL;
	if (simMemory != NULL) {
		memoryTable = new CJediAiMemory[actionCount];
		if (memoryTable == NULL) {
			error("CJediAiActionSelectorBase::simulate() - Out of memory allocating %d bytes", (sizeof(CJediAiMemory) * actionCount));
			return NULL;
		}
	}

	// simulate each action
	EJediAiActionSimResult bestResult = eJediAiActionSimResult_Impossible;
	for (int i = 0; i < actionCount; ++i) {

		// get the next action
		CJediAiAction *action = actionTable[i];
		if (action == NULL) {
			continue;
		}

		// if we can't select this action, skip it
		if (!canSelectAction(i)) {
			continue;
		}

		// if we have a memory table, simulate the action into it's memory in that table
		// otherwise, just give it a copy of our memory to simulate into
		if (memoryTable != NULL && simMemory != NULL) {
			memoryTable[i].copy(*simMemory);
			action->simulate(memoryTable[i]);
		} else {
			CJediAiMemory actionSimMemory(*memory);
			action->simulate(actionSimMemory);
		}

		// if this action has the best result so far, save it off
		if (bestResult < action->simSummary.result) {
			bestResult = action->simSummary.result;
		}
	}

	// select the best action
	CJediAiAction *bestAction = NULL;
	int bestActionIndex = compareAndSelectAction(actionCount, actionTable, bestResult);
	if (bestActionIndex > -1 && bestActionIndex < actionCount) {
		if (simMemory != NULL && memoryTable != NULL) {
			simMemory->copy(memoryTable[bestActionIndex]);
		}
		bestAction = actionTable[bestActionIndex];
	}

	// delete our temporary memory table
	if (memoryTable != NULL) {
		delete [] memoryTable;
		memoryTable = NULL;
	}

	// return our selected action
	return bestAction;
}

int CJediAiActionSelectorBase::compareAndSelectAction(int actionCount, CJediAiAction *const actionTable[], EJediAiActionSimResult bestResult) const {

	// if every action available is negative and we don't allow those, do nothing
	if (!selectorParams.allowNegativeActions && bestResult <= eJediAiActionSimResult_Irrelevant) {
		return -1;
	}

	// choose the first action which is the most beneficial
	// if our current action is most beneficial, choose that even if it isn't the first one
	int bestDebouncedAction = -1;
	int bestAction = -1;
	for (int i = 0; i < actionCount; ++i) {

		// get the next action
		CJediAiAction *action = actionTable[i];
		if (action == NULL) {
			continue;
		}

		// if this action's result is too low, skip it
		if (action->simSummary.result < bestResult) {
			continue;
		}

		// if we already have a best action and this isn't our current action, skip it
		if (bestAction >= 0) {
			if (!selectorParams.ifEqualUseCurrentAction || action != selectorData.currentAction) {
				continue;
			}
		}

		// if this action is debounced, save it off as the best debounced action
		// otherwise, save it off as the best action
		if (selectorParams.debounceActions && action == selectorData.debouncedAction) {
			bestDebouncedAction = i;
		} else {
			bestAction = i;
		}
	}

	// if we couldn't find anything except our debounced action, select it
	if (bestAction < 0) {
		bestAction = bestDebouncedAction;
	}

	// return the best action
	return bestAction;
}

bool CJediAiActionSelectorBase::canSelectAction(int actionIndex) const {
	return true;
}


/////////////////////////////////////////////////////////////////////////////
//
// binary selector
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionSelectorBinary::CJediAiActionSelectorBinary() {
	memset(actionTable, 0, sizeof(actionTable));
	condition = NULL;
}

CJediAiAction **CJediAiActionSelectorBinary::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}

CJediAiAction *CJediAiActionSelectorBinary::selectAction(CJediAiMemory *simMemory) const {

	// get our actions
	// if we can't select an action, leave it NULL
	CJediAiAction *failure = (canSelectAction(eAction_OnFailure) ? onFailure : NULL);
	CJediAiAction *success = (canSelectAction(eAction_OnSuccess) ? onSuccess : NULL);

	// if we have no success action, fail
	if (success == NULL) {
		if (failure != NULL) {
			failure->simulate(*simMemory);
		}
		return failure;
	}

	// if we have no condition, fail
	if (condition == NULL) {
		if (failure != NULL) {
			failure->simulate(*simMemory);
		}
		return failure;
	}

	// check our condition
	EJediAiActionResult result;
	if (simMemory != NULL) {
		bool simulating = (simMemory != memory);
		result = condition->checkConstraint(*simMemory, *this, simulating);
	} else {
		CJediAiMemory tempMemory(*memory);
		result = condition->checkConstraint(tempMemory, *this, false);
	}

	// if our condition failed, fail
	if (result == eJediAiActionResult_Failure) {
		if (failure != NULL) {
			failure->simulate(*simMemory);
		}
		return failure;
	}

	// success!
	success->simulate(*simMemory);
	return success;
}


/////////////////////////////////////////////////////////////////////////////
//
// random
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionRandomBase::CJediAiActionRandomBase() {
}

EJediAiAction CJediAiActionRandomBase::getType() const {
	return eJediAiAction_Random;
}

int CJediAiActionRandomBase::compareAndSelectAction(int actionCount, CJediAiAction *const actionTable[], EJediAiActionSimResult bestResult) const {

	// if every action available is hurtful, do nothing
	if (!selectorParams.allowNegativeActions && bestResult < eJediAiActionSimResult_Irrelevant) {
		return -1;
	}

	// get my odds table
	const float *actionOddsTable = getActionOddsTable(NULL);
	if (actionOddsTable == NULL) {
		assert(actionOddsTable != NULL);
		return BASECLASS::compareAndSelectAction(actionCount, actionTable, bestResult);
	}

	// create a local copy of the odds table
	int sizeofActionOddsTable = sizeof(float) * actionCount;
	float *localActionOddsTable = (float*)alloca(sizeofActionOddsTable);
	if (localActionOddsTable == NULL) {
		error("alloca(%d) failed in CJediAiActionRandomBase::compareAndSelectAction()\n", sizeofActionOddsTable);
		return -1;
	}
	memset(localActionOddsTable, 0, sizeofActionOddsTable);

	// if we already have a selected action and it is one of the best and we are supposed to, just return it
	if (isInProgress()) {
		if (selectorParams.ifEqualUseCurrentAction) {
			if (selectorData.currentAction != NULL && selectorData.currentAction->simSummary.result >= bestResult) {
				for (int i = 0; i < actionCount; ++i) {
					if (actionTable[i] == selectorData.currentAction) {
						return i;
					}
				}
			}
		}
	}

	// build the odds table for choosing an action
	int debouncedActionIndex = -1;
	bool allOddsZero = true;
	for (int i = 0; i < actionCount; ++i) {

		// get the next action
		CJediAiAction *action = actionTable[i];
		if (action == NULL) {
			continue;
		}

		// only use the odds for actions which are the most beneficial
		// leave the odds of everything else at zero
		if (action->simSummary.result < bestResult) {
			localActionOddsTable[i] = 0.0f;
		} else if (selectorParams.debounceActions && action == selectorData.debouncedAction) {
			debouncedActionIndex = i;
		} else {
			localActionOddsTable[i] = actionOddsTable[i];
			if (actionOddsTable[i] != 0.0f) {
				allOddsZero = false;
			}
		}
	}

	// if we couldn't find anything except our debounced action, try it
	if (allOddsZero && debouncedActionIndex != -1) {
		localActionOddsTable[debouncedActionIndex] = actionOddsTable[debouncedActionIndex];
		if (actionOddsTable[debouncedActionIndex] != 0.0f) {
			allOddsZero = false;
		}
	}

	// if no action has a chance of happening, do nothing
	if (allOddsZero) {
		return -1;
	}

	// choose an action
	int actionIndex = randChoice(actionCount, localActionOddsTable);
	return actionIndex;
}

bool CJediAiActionRandomBase::canSelectAction(int actionIndex) const {

	// get my odds table
	int actionCount = 0;
	const float *actionOddsTable = getActionOddsTable(&actionCount);
	if (actionOddsTable == NULL) {
		assert(actionOddsTable != NULL);
		return false;
	}

	// if the index is out of fail
	if (actionIndex < 0 || actionIndex >= actionCount) {
		return false;
	}

	// if there is no chance of selecting this action, return false
	if (actionOddsTable[actionIndex] <= 0.0f) {
		return false;
	}

	// sure, we can select this action
	return true;
}

const float *CJediAiActionRandomBase::getActionOddsTable(int *actionCount) const {
	CJediAiActionRandomBase *me = const_cast<CJediAiActionRandomBase*>(this);
	return me->getActionOddsTable(actionCount);
}


/////////////////////////////////////////////////////////////////////////////
//
// decorator
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionDecorator::CJediAiActionDecorator() {
	decoratedAction = NULL;
}

EJediAiAction CJediAiActionDecorator::getType() const {
	return eJediAiAction_Decorator;
}

void CJediAiActionDecorator::init(CJediAiMemory *newWorldState) {

	// base class version
	BASECLASS::init(newWorldState);

	// if we have an action, initialize it
	if (decoratedAction != NULL) {
		decoratedAction->init(newWorldState);
	}
}

void CJediAiActionDecorator::reset() {

	// base class version
	BASECLASS::reset();

	// reset our action
	if (decoratedAction != NULL) {
		decoratedAction->reset();
	}
}

EJediAiActionResult CJediAiActionDecorator::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if we have no action, bail
	if (decoratedAction == NULL) {
		return eJediAiActionResult_Failure;
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionDecorator::onBegin() {

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// begin our action
	if (decoratedAction != NULL) {
		result = decoratedAction->onBegin();
	}
	return result;
}

void CJediAiActionDecorator::onEnd() {

	// base class version
	BASECLASS::onEnd();

	// end our action
	if (decoratedAction != NULL) {
		decoratedAction->onEnd();
	}
}

void CJediAiActionDecorator::simulate(CJediAiMemory &simMemory) {

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		initSimSummary(simSummary, simMemory);
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// update our action
	if (decoratedAction != NULL) {
		decoratedAction->simulate(simMemory);
		simSummary = decoratedAction->simSummary;
	} else {
		initSimSummary(simSummary, simMemory);
	}
}

void CJediAiActionDecorator::updateTimers(float dt) {

	// if we have no action, bail
	if (decoratedAction == NULL) {
		return;
	}

	// update our action's timers
	decoratedAction->updateTimers(dt);
}

EJediAiActionResult CJediAiActionDecorator::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// update our action
	if (decoratedAction != NULL) {
		result = decoratedAction->update(dt);
	}
	return result;
}

bool CJediAiActionDecorator::isNotSelectable() const {

	// base class version
	if (BASECLASS::isNotSelectable()) {
		return true;
	}

	// if my subaction is not selectable, I am not either
	if (decoratedAction != NULL && decoratedAction->isNotSelectable()) {
		return true;
	}

	// I am selectable
	return false;
}


/////////////////////////////////////////////////////////////////////////////
//
// fake simulation
//
/////////////////////////////////////////////////////////////////////////////

const float CJediAiActionFakeSim::kKillVictim = -1.0f;

CJediAiActionFakeSim::CJediAiActionFakeSim() {
	modifySimConstraint = NULL;
	reset();
}

EJediAiAction CJediAiActionFakeSim::getType() const {
	return eJediAiAction_FakeSim;
}

void CJediAiActionFakeSim::reset() {

	// base class version
	BASECLASS::reset();

	// if I have a 'modify sim' constraint, reset it
	if (modifySimConstraint != NULL) {
		modifySimConstraint->reset();
	}

	// clear my params
	memset(&params, 0, sizeof(params));
	params.minResult = eJediAiActionSimResult_Count;
}

EJediAiActionResult CJediAiActionFakeSim::onBegin() {

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	return result;
}

void CJediAiActionFakeSim::onEnd() {

	// base class version
	BASECLASS::onEnd();
}

void CJediAiActionFakeSim::simulate(CJediAiMemory &simMemory) {

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		initSimSummary(simSummary, simMemory);
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// keep our sim summary in a local summary, as BASECLASS::simulate() will override our member summary
	SJediAiActionSimSummary localSimSummary;
	initSimSummary(localSimSummary, simMemory);
	bool shouldSimulate = (params.duration > 0.0f);

	// if we should simulate our decorated action before faking our sim, do so now
	if (params.postSim) {
		BASECLASS::simulate(simMemory);
	}

	// only modify the sim if this constraint passes
	bool allowModifySim = ((modifySimConstraint == NULL) || (modifySimConstraint->checkConstraint(simMemory, *this, true) == eJediAiActionResult_InProgress));
	if (allowModifySim) {

		// handle victim stuff
		if (simMemory.victimState->actor != NULL) {

			// simulate victim damage
			float damage = (params.damageVictim == kKillVictim ? simMemory.victimState->hitPoints : params.damageVictim);
			if (damage > 0.0f) {
				simMemory.simulateDamage(damage, *simMemory.victimState);
				shouldSimulate = true;
			}

			// clear victim threats
			for (int i = 0; i < simMemory.threatStateCount; ++i) {
				SJediAiThreatState &threatState = simMemory.threatStates[i];
				if (threatState.attackerState == simMemory.victimState && params.clearVictimThreats[threatState.type]) {
					threatState.duration = 0.0f;
					threatState.strength = 0.0f;
					shouldSimulate = true;
				}
			}

			// break victim shield
			if (params.breakVictimShield) {
				simMemory.victimState->flags &= ~kJediAiActorStateFlag_Shielded;
				shouldSimulate = true;
			}

			// make victim stumble
			if (params.makeVictimStumble) {
				simMemory.victimState->flags |= kJediAiActorStateFlag_Stumbling;
				shouldSimulate = true;
			}
		}

		// if we changed the sim or have a duration to burn off, do it
		if (shouldSimulate) {
			simMemory.simulate(max(params.duration, 0.0f), CJediAiMemory::SSimulateParams());
		}

		// if we are ignoring damage, act as if we weren't hurt during our sim
		if (params.ignoreDamage && simMemory.selfState.hitPoints < memory->selfState.hitPoints) {
			simMemory.selfState.hitPoints = memory->selfState.hitPoints;
			simMemory.setSelfInState(eJediState_KnockedAround, false);
		}
	}

	// if we should simulate our decorated action after faking our sim, do so now
	if (!params.postSim) {
		BASECLASS::simulate(simMemory);
	}

	// update our sim summary
	bool decoratedActionImpossible = (simSummary.result == eJediAiActionSimResult_Impossible);
	setSimSummary(localSimSummary, simMemory);
	simSummary = localSimSummary;

	// only modify the sim if this constraint passes
	if (allowModifySim) {

		// if our decorated action was impossible, consider this action impossible
		if (decoratedActionImpossible) {
			simSummary.result = eJediAiActionSimResult_Impossible;

		// otherwise, if this action was irrelevant and our decorated action's summary was better, use it instead
		} else if (simSummary.result == eJediAiActionSimResult_Irrelevant) {
			if (decoratedAction != NULL && decoratedAction->simSummary.result > eJediAiActionSimResult_Irrelevant) {
				simSummary.result = decoratedAction->simSummary.result;
			}
		}

		// if we have specified a minimum result, enforce it (unless our current result is 'impossible')
		if (params.minResult != eJediAiActionSimResult_Count && simSummary.result < params.minResult && !decoratedActionImpossible) {
			simSummary.result = params.minResult;
		}

	// otherwise, if our decorated action was impossible, consider this action impossible
	} else if (decoratedActionImpossible) {
		simSummary.result = eJediAiActionSimResult_Impossible;
	}
}

EJediAiActionResult CJediAiActionFakeSim::update(float dt) {

	// base class version
	EJediAiActionResult result = BASECLASS::update(dt);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
//
// walk/run
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionWalkRun::CJediAiActionWalkRun() {
	reset();
}

EJediAiAction CJediAiActionWalkRun::getType() const {
	return eJediAiAction_WalkRun;
}

void CJediAiActionWalkRun::reset() {

	// base class version
	BASECLASS::reset();

	// reset my data
	memset(&params, 0, sizeof(params));
}

EJediAiActionResult CJediAiActionWalkRun::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I can't do this, I can't do this
	if (!simMemory.canSelfDoAction(eJediAction_WalkRun)) {
		return eJediAiActionResult_Failure;
	}

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return eJediAiActionResult_Failure;
	}

	// if my destination is my victim and I can't see it, bail
	if (!simMemory.victimInView && params.destination == eJediAiDestination_Victim) {
		return eJediAiActionResult_Failure;
	}

	// get my destination actor
	float minDistance = 0.5f;
	float maxDistance = 1e20f;
	SJediAiActorState *destActorState = lookupJediAiDestinationActorState(params.destination, simMemory, &minDistance, &maxDistance);
	if (destActorState == NULL) {
		return eJediAiActionResult_Failure;
	}

	// if I'm already close enough, succeed
	float distanceFromTarget = limit(minDistance, params.distance, maxDistance);
	if (destActorState->distanceToSelf <= distanceFromTarget) {
		return eJediAiActionResult_Success;
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	if (result == eJediAiActionResult_Failure) {
		return result;
	}

	// if I can't get to the specified position, I can't do this
	if (simulating) {

		// compute our target position
		CVector wDestActorPos = destActorState->wPos;
		if (params.destination == eJediAiDestination_Victim) {
			wDestActorPos.y = simMemory.victimFloorHeight;
		}
		CVector wTargetPos = wDestActorPos + (destActorState->iToSelfDir * distanceFromTarget);

		// see if we can pathfind to this point
		if (!determinePathFindValidity(simMemory.selfState.wPos, wTargetPos)) {
			return eJediAiActionResult_Failure;
		}
	}

	// done!
	return result;
}

EJediAiActionResult CJediAiActionWalkRun::onBegin() {

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionWalkRun::onEnd() {
	BASECLASS::onEnd();

	// stop immediately
	memory->selfState.jedi->stop("CJediAiActionWalkRun::onEnd");
}

void CJediAiActionWalkRun::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// get my destination actor
	float minDistance = 0.5f;
	float maxDistance = 1e20f;
	SJediAiActorState *destActorState = lookupJediAiDestinationActorState(params.destination, simMemory, &minDistance, &maxDistance);
	if (destActorState == NULL) {
		return;
	}

	// assume that my victim isn't moving
	destActorState->iVelocity.zero();

	// estimate how long it will take to reach my target
	float distanceFromTarget = limit(minDistance, params.distance, maxDistance);
	CVector wTargetPos = destActorState->wPos + (destActorState->iToSelfDir * distanceFromTarget);
	float distance = simMemory.selfState.wPos.distanceTo(wTargetPos);
	float duration = distance / kJediMoveSpeedMax;

	// update the sim memory states which are relative to the actor
	CVector iSelfToTargetDir = -destActorState->iToSelfDir;
	CJediAiMemory::SSimulateParams simMemoryParams;
	simMemoryParams.wSelfPos = &wTargetPos;
	simMemoryParams.iSelfFrontDir = &iSelfToTargetDir;
	simMemory.simulate(duration, simMemoryParams);

	// success
	setSimSummary(simSummary, simMemory);
}

EJediAiActionResult CJediAiActionWalkRun::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// get my destination actor
	float minDistance = 0.5f;
	float maxDistance = 1e20f;
	SJediAiActorState *destActorState = lookupJediAiDestinationActorState(params.destination, *memory, &minDistance, &maxDistance);
	if (destActorState == NULL) {
		return eJediAiActionResult_Failure;
	}

	// if I'm close enough, I'm done walking
	float distanceFromTarget = limit(minDistance, params.distance, maxDistance);
	if (destActorState->distanceToSelf <= distanceFromTarget) {

		// if we aren't facing our target enough, we are still in progress
		if (destActorState->selfFacePct < params.facePct) {
			return eJediAiActionResult_InProgress;
		}

		// succeed
		return eJediAiActionResult_Success;
	}

	// compute the destination position
	// if my target is my victim, set the destination position's y component to the victim's floor y
	CVector wDestPos = destActorState->wPos;
	if (params.destination == eJediAiDestination_Victim) {
		wDestPos.y = memory->victimFloorHeight;
	}

	// compute the target position
	CVector wTargetPos = wDestPos + (destActorState->iToSelfDir * distanceFromTarget);

	// if I can't get to the specified position, I can't do this
	if (!determinePathFindValidity(memory->selfState.wPos, wTargetPos)) {
		return eJediAiActionResult_Failure;
	}

	// move toward the specified position
	memory->selfState.jedi->passNearPoint(destActorState->wPos, distanceFromTarget, false, "CJediAiActionWalkRun::update");

	// adjust move speed based on distance
	float remainingDistance = (destActorState->distanceToSelf - distanceFromTarget);
	if (remainingDistance < 0.0f) {
		remainingDistance = 0.0f;
	}
	float walkDistance = distanceFromTarget;
	float runDistance = walkDistance + max(params.minRunDistance, 0.25f);
	float clampedDistance = limit(walkDistance, remainingDistance, runDistance);
	float speed = linterp(walkDistance, runDistance, 0.75f, 1.0f, clampedDistance);
	memory->selfState.jedi->setDesiredMoveSpeed(speed);

	// still running
	return eJediAiActionResult_InProgress;
}


/////////////////////////////////////////////////////////////////////////////
//
// dash
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionDash::CJediAiActionDash() {
	reset();
}

EJediAiAction CJediAiActionDash::getType() const {
	return eJediAiAction_Dash;
}

void CJediAiActionDash::reset() {

	// base class version
	BASECLASS::reset();

	// reset my data
	memset(&params, 0, sizeof(params));
	memset(&data, 0, sizeof(data));
}

EJediAiActionResult CJediAiActionDash::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I can't do this, I can't do this
	if (!simMemory.canSelfDoAction(eJediAction_Dash)) {
		return eJediAiActionResult_Failure;
	}

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return eJediAiActionResult_Failure;
	}

	// if I'm attacking, we need a lightsaber to do this
	if (params.attack && simMemory.selfState.saberDamage <= 0.0f) {
		return eJediAiActionResult_Failure;
	}

	// I can't do this while deflecting
	if (simMemory.isSelfInState(eJediState_Deflecting)) {
		return eJediAiActionResult_Failure;
	}

	// if my destination is my victim and I can't see it, bail
	if (!simMemory.victimInView && params.destination == eJediAiDestination_Victim) {
		return eJediAiActionResult_Failure;
	}

	// get my destination actor
	float minDistance = 0.5f;
	float maxDistance = 1e20f;
	SJediAiActorState *destActorState = lookupJediAiDestinationActorState(params.destination, simMemory, &minDistance, &maxDistance);
	if (destActorState == NULL) {
		return eJediAiActionResult_Failure;
	}

	// if I'm ignoring the min distance, set it to my distance parameter
	if (params.ignoreMinDistance) {
		minDistance = params.distance;
	}

	// if I haven't started dashing and I'm too close, fail
	if (!isInProgress()) {
		float activationDistanceFromTarget = limit(minDistance, params.activationDistance, maxDistance);
		if (destActorState->distanceToSelf <= activationDistanceFromTarget) {
			return eJediAiActionResult_Failure;
		}
	}

	// if my victim is my destination, update how close I can get to it
	float distanceFromTarget = limit(minDistance, params.distance, maxDistance);
	if (params.destination == eJediAiDestination_Victim) {
		float possibleDistance = (destActorState->distanceToSelf - simMemory.selfState.nearestCollisionTable[CJediAiMemory::eCollisionDir_Victim].distance + kJediDashExitDistance);
		if (distanceFromTarget < possibleDistance) {
			distanceFromTarget = possibleDistance;
		}

	// otherwise, check the navmesh directly (only when simulating) to determine how close I can get
	} else if (simulating) {

		// compute our target position
		CVector wDestActorPos = destActorState->wPos;
		if (params.destination == eJediAiDestination_Victim) {
			wDestActorPos.y = simMemory.victimFloorHeight;
		}
		CVector wTargetPos = wDestActorPos + (destActorState->iToSelfDir * distanceFromTarget);

		//// see if we can pathfind to this point
		//CVector wClosestPoint = kZeroVector;
		//SFindClosestPolyInfo cpInfo;
		//if (gNavMesh->collideRay(simMemory.selfState.wPos, wTargetPos, cpInfo, true, &wClosestPoint)) {
		//	float possibleDistance = simMemory.selfState.wPos.distanceTo(wClosestPoint);
		//	if (distanceFromTarget < possibleDistance) {
		//		distanceFromTarget = possibleDistance;
		//	}
		//}
	}

	// if I'm already close enough, succeed
	if (destActorState->distanceToSelf <= distanceFromTarget) {
		return eJediAiActionResult_Success;
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	if (result == eJediAiActionResult_Failure) {
		return result;
	}

	// done!
	return result;
}

EJediAiActionResult CJediAiActionDash::onBegin() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionDash::onEnd() {
	BASECLASS::onEnd();
}

void CJediAiActionDash::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// get my destination actor
	float minDistance = 0.5f;
	float maxDistance = 1e20f;
	SJediAiActorState *destActorState = lookupJediAiDestinationActorState(params.destination, simMemory, &minDistance, &maxDistance);
	if (destActorState == NULL) {
		return;
	}

	// if I'm ignoring the min distance, set it to my distance parameter
	if (params.ignoreMinDistance) {
		minDistance = params.distance;
	}

	// estimate how long it will take to reach my target
	float distanceFromTarget = limit(minDistance, params.distance, maxDistance);
	float distance = (destActorState->distanceToSelf - distanceFromTarget);
	float duration = distance / kJediDashSpeed;

	// if we are attacking as well, damage our target
	if (params.attack) {
		if (simMemory.selfState.saberDamage > 0.0f) {
			if (simMemory.victimState->flags & kJediAiActorStateFlag_Shielded) {
				simMemory.victimState->flags &= ~kJediAiActorStateFlag_Shielded;
			} else {
				simMemory.simulateDamage(simMemory.selfState.saberDamage, *destActorState);
			}
		}
	}

	// compute our target position
	CVector wDestActorPos = destActorState->wPos;
	if (params.destination == eJediAiDestination_Victim) {
		wDestActorPos.y = simMemory.victimFloorHeight;
	}
	CVector wTargetPos = wDestActorPos + (destActorState->iToSelfDir * distanceFromTarget);

	// update the sim memory states which are relative to the actor
	CVector iSelfToTargetDir = -destActorState->iToSelfDir;
	CJediAiMemory::SSimulateParams simMemoryParams;
	simMemoryParams.wSelfPos = &wTargetPos;
	simMemoryParams.iSelfFrontDir = &iSelfToTargetDir;
	simMemory.simulate(duration, simMemoryParams);

	// success
	setSimSummary(simSummary, simMemory);
}

EJediAiActionResult CJediAiActionDash::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// get my destination actor
	float minDistance = 0.5f;
	float maxDistance = 1e20f;
	SJediAiActorState *destActorState = lookupJediAiDestinationActorState(params.destination, *memory, &minDistance, &maxDistance);
	if (destActorState == NULL) {
		return eJediAiActionResult_Failure;
	}

	// if I'm ignoring the min distance, set it to my distance parameter
	if (params.ignoreMinDistance) {
		minDistance = params.distance;
	}

	// if I am not already dashing, try to do it
	float activationDistanceFromTarget = limit(minDistance, params.activationDistance, maxDistance);
	float distanceFromTarget = limit(minDistance, params.distance, maxDistance);
	bool wasDashing = data.wasDashing;
	bool isDashing = memory->isSelfInState(eJediState_Dashing);
	data.wasDashing = isDashing;
	if (!isDashing && !wasDashing) {

		// first, face my target
		CVector iSelfToTargetDir = -destActorState->iToSelfDir;
		float facePct = memory->selfState.iFrontDir.dotProduct(iSelfToTargetDir);
		const float kMinFacePct = 0.9f;
		if (facePct < kMinFacePct) {
			return eJediAiActionResult_InProgress;
		}

		// if I'm too close, succeed
		if (destActorState->distanceToSelf <= activationDistanceFromTarget) {
			return eJediAiActionResult_Success;
		}

		// if I'm already close enough, succeed
		if (destActorState->distanceToSelf <= distanceFromTarget) {
			return eJediAiActionResult_Success;
		}

		// compute our target position
		CVector wDestActorPos = destActorState->wPos;
		if (params.destination == eJediAiDestination_Victim) {
			wDestActorPos.y = memory->victimFloorHeight;
		}
		CVector wTargetPos = wDestActorPos + (destActorState->iToSelfDir * distanceFromTarget);

		// if I can't get to the specified position, I can't do this
		if (!determinePathFindValidity(memory->selfState.wPos, wTargetPos)) {
			return eJediAiActionResult_Failure;
		}

		// send the dash command
		float distance = (params.attack ? 0.0f : (params.distance > minDistance ? params.distance - minDistance : minDistance));
		memory->selfState.jedi->setCommandJediDash(destActorState->actor, distance, params.attack);
		data.wasDashing = true;
	}

	// if I'm dashing or haven't started, I'm still in progress
	if (isDashing || !wasDashing) {
		return eJediAiActionResult_InProgress;
	}

	// if I didn't arrive at my target, I failed
	if (!params.attack && destActorState->distanceToSelf > distanceFromTarget) {
		return eJediAiActionResult_Failure;
	}

	// success!
	return eJediAiActionResult_Success;
}


/////////////////////////////////////////////////////////////////////////////
//
// strafe
//
/////////////////////////////////////////////////////////////////////////////

CJediAiMemory::ECollisionDir convertStrafeDirToCollisionDir(EStrafeDir strafeDir) {
	switch (strafeDir) {
		case eStrafeDir_Left: return CJediAiMemory::eCollisionDir_Left;
		case eStrafeDir_Right: return CJediAiMemory::eCollisionDir_Right;
		case eStrafeDir_Forward: return CJediAiMemory::eCollisionDir_Forward;
		case eStrafeDir_Backward: return CJediAiMemory::eCollisionDir_Backward;
	}
	return CJediAiMemory::eCollisionDir_Count;
}

CJediAiActionStrafe::CJediAiActionStrafe() {
	reset();
}

EJediAiAction CJediAiActionStrafe::getType() const {
	return eJediAiAction_Strafe;
}

void CJediAiActionStrafe::reset() {

	// base class version
	BASECLASS::reset();

	// reset my data
	memset(&params, 0, sizeof(params));
	memset(&data, 0, sizeof(data));
	params.dir = eStrafeDir_Count;
	params.moveDistanceMin = 1.0f;
	params.moveDistanceMax = 10.0f;
	params.distanceFromVictimMin = 0.0f;
	params.distanceFromVictimMax = 1e20f;
	params.requireVictim = true;
}

EJediAiActionResult CJediAiActionStrafe::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I can't do this, I can't do this
	if (!simMemory.canSelfDoAction(eJediAction_WalkRun)) {
		return eJediAiActionResult_Failure;
	}

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return eJediAiActionResult_Failure;
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	if (result == eJediAiActionResult_Failure) {
		return result;
	}

	// if I require a victim and don't have one, bail
	if (simMemory.victimState->actor == NULL) {
		if (params.requireVictim) {
			return eJediAiActionResult_Failure;
		}

	// otherwise, if my end position is too close to or too far away from my victim, I may fail
	} else {
		float endDistanceToVictim = data.wSelfEndPos.distanceTo(simMemory.victimState->wPos);
		if (endDistanceToVictim < params.distanceFromVictimMin) {
			if (endDistanceToVictim < (simMemory.victimState->distanceToSelf + 1.0f)) {
				return eJediAiActionResult_Failure;
			}
		} else if (endDistanceToVictim > params.distanceFromVictimMax) {
			if (endDistanceToVictim > (simMemory.victimState->distanceToSelf - 1.0f)) {
				return eJediAiActionResult_Failure;
			}
		}
	}

	// if anything is in my way, bail
	CJediAiMemory::ECollisionDir collisionDir = convertStrafeDirToCollisionDir(params.dir);
	if (collisionDir < CJediAiMemory::eCollisionDir_Count) {
		float distance = simMemory.selfState.nearestCollisionTable[collisionDir].distance;
		CActor *actor = simMemory.selfState.nearestCollisionTable[collisionDir].actor;
		if (data.desiredMoveDistance >= distance) {
			return eJediAiActionResult_Failure;
		}
	}

	// done!
	return result;
}

EJediAiActionResult CJediAiActionStrafe::onBegin() {

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// set my start position
	data.wSelfPrevPos = memory->selfState.wPos;
	data.wSelfStartPos = memory->selfState.wPos;

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionStrafe::onEnd() {
	BASECLASS::onEnd();

	// stop moving
	memory->selfState.jedi->stop("CJediAiActionStrafe::onEnd");

	// clear my data
	memset(&data, 0, sizeof(data));
}

void CJediAiActionStrafe::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// if I am not currently in progress, generate my data
	if (!isInProgress()) {
		data.desiredMoveDistance = fRand(params.moveDistanceMin, params.moveDistanceMax);
		data.wSelfStartPos = simMemory.selfState.wPos;
		data.wSelfEndPos = computeTargetPos(params.dir, data.desiredMoveDistance, simMemory);
	}

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// compute my new face direction
	CVector iFrontDir;
	if (simMemory.victim != NULL) {
		iFrontDir = data.wSelfEndPos.directionTo(simMemory.victimState->wPos);
	} else {
		iFrontDir = simMemory.selfState.iFrontDir;
	}

	// estimate how long it will take to reach my target
	float distance = simMemory.selfState.wPos.distanceTo(data.wSelfEndPos);
	float duration = distance / kJediStrafeSpeed;

	// update the sim memory states which are relative to the actor
	CJediAiMemory::SSimulateParams simMemoryParams;
	simMemoryParams.wSelfPos = &data.wSelfEndPos;
	simMemoryParams.iSelfFrontDir = &iFrontDir;
	simMemory.simulate(duration, simMemoryParams);

	// success
	setSimSummary(simSummary, simMemory);
	if (simSummary.result == eJediAiActionSimResult_Irrelevant) {
		simSummary.result = eJediAiActionSimResult_Cosmetic;
	}
}

EJediAiActionResult CJediAiActionStrafe::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// how far have we walked since last update?
	data.moveDistance += data.wSelfPrevPos.distanceTo(memory->selfState.wPos);
	data.wSelfPrevPos = memory->selfState.wPos;

	// if I've not strafed the specified distance, I'm still in progress
	if (data.moveDistance < data.desiredMoveDistance) {

		// walk in the specified direction
		CVector iWalkDir = computeStrafeDir(params.dir, *memory);
		CVector wTargetPos = memory->selfState.wPos + iWalkDir;
		memory->selfState.jedi->passNearPoint(wTargetPos, 0.5f, false, "CJediAiActionStrafe::update");
		memory->selfState.jedi->setDesiredMoveSpeed(1.0f);
		memory->selfState.jedi->setCommandJediStrafe();

		// we are still in progress
		return eJediAiActionResult_InProgress;
	}

	// success!
	return eJediAiActionResult_Success;
}

CVector CJediAiActionStrafe::computeStrafeDir(EStrafeDir dir, const CJediAiMemory &memory) {
	switch (dir) {
		case eStrafeDir_Left: return -memory.selfState.iRightDir;
		case eStrafeDir_Right: return memory.selfState.iRightDir;
		case eStrafeDir_Forward: return memory.selfState.iFrontDir;
		case eStrafeDir_Backward: return -memory.selfState.iFrontDir;
		default: return kZeroVector;
	}
}

CVector CJediAiActionStrafe::computeTargetPos(EStrafeDir dir, float distance, const CJediAiMemory &memory) {

	// if moving horizontally, we rotate around our target
	CVector wTargetPos;
	if (memory.victimState->actor != NULL && (dir == eStrafeDir_Left || dir == eStrafeDir_Right)) {

		// compute the angle delta
		float radius = memory.victimState->distanceToSelf;
		float circumference = PI * 2.0f * radius;
		float deltaAngle = TWOPI * (distance / circumference);
		if (dir == eStrafeDir_Left) {
			deltaAngle = -deltaAngle;
		}

		// convert the angle delta into x-z delta and update our strafe delta
		float deltaX, deltaZ;
		sinCos(deltaX, deltaZ, deltaAngle);
		CVector iOffsetFromVictim = (
			((deltaX * radius) * memory.victimState->iToSelfDir.crossProduct(kUnitVectorY)) + 
			((deltaZ * radius) * memory.victimState->iToSelfDir)
		);

		// compute our target pos
		wTargetPos = (memory.victimState->wPos + iOffsetFromVictim);

	// otherwise, we just offset from our current pos
	} else {
		wTargetPos = memory.selfState.wPos + (computeStrafeDir(dir, memory) * distance);
	}

	// return the target pos
	return wTargetPos;
}


/////////////////////////////////////////////////////////////////////////////
//
// jump
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionJumpForward::CJediAiActionJumpForward() {
	reset();
}

EJediAiAction CJediAiActionJumpForward::getType() const {
	return eJediAiAction_JumpForward;
}

void CJediAiActionJumpForward::reset() {

	// base class version
	BASECLASS::reset();

	// reset my data
	memset(&params, 0, sizeof(params));
}

EJediAiActionResult CJediAiActionJumpForward::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I can't do this, I can't do this
	if (!simMemory.canSelfDoAction(eJediAction_Jump)) {
		return eJediAiActionResult_Failure;
	}

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return eJediAiActionResult_Failure;
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	if (result == eJediAiActionResult_Failure) {
		return result;
	}

	// if I'm attacking, we need a lightsaber to do this
	if (params.attack != eJediAiJumpForwardAttack_None && simMemory.selfState.saberDamage <= 0.0f) {
		return eJediAiActionResult_Failure;
	}

	// only check these constraints when I am not in progress
	if (!isInProgress()) {

		// we need a victim to do this
		if (simMemory.victimState->actor == NULL) {
			return eJediAiActionResult_Failure;
		}

		// if I can't see my victim, bail
		if (!simMemory.victimInView) {
			return eJediAiActionResult_Failure;
		}

		// if my victim is off the ground, I can't jump to him
		float victimHeightOffGround = (simMemory.victimState->wPos.y - simMemory.victimFloorHeight);
		if (victimHeightOffGround > simMemory.selfState.wBoundsCenterPos.y) {
			return eJediAiActionResult_Failure;
		}

		// if my victim is being tk'ed, I can't jump to him
		if (simMemory.victimState->flags & kJediAiActorStateFlag_Gripped) {
			return eJediAiActionResult_Failure;
		}

		// if my victim is incapacitated, I can't jump to him
		if (simMemory.victimState->flags & kJediAiActorStateFlag_Incapacitated) {
			return eJediAiActionResult_Failure;
		}

		// if I'm not facing my target, I can't jump to him
		const float kMinFacePct = 0.9f;
		if (simMemory.victimState->selfFacePct < kMinFacePct) {
			return eJediAiActionResult_Failure;
		}

		// compute my target position
		float boundsRadius = simMemory.selfState.collisionRadius;
		CVector wTargetPos = computeTargetPos(simMemory, params.distance);
		CVector iTargetToSelfDir = -simMemory.victimState->iToSelfDir;
		float distanceSq = simMemory.selfState.wPos.distanceSqTo(wTargetPos);

		// if I'm attacking and I'm too far away from the target position, I can't jump to it
		if (params.attack != eJediAiJumpForwardAttack_None) {
			float maxJumpDistance = (params.attack ? simMemory.selfState.maxJumpForwardDistance + kJediMeleeCorrectionTweak : simMemory.selfState.maxJumpForwardDistance);
			if (distanceSq > SQ(maxJumpDistance)) {
				return eJediAiActionResult_Failure;
			}
		}

		// if I'm too close to the target position, I can't jump to it
		if (distanceSq <= SQ(params.activationDistance)) {
			return eJediAiActionResult_Failure;
		}

		// check collision
		if (simulating) {
			if (!checkCollision(simMemory, params.distance)) {
				return eJediAiActionResult_Failure;
			}
		}
	}

	// done!
	return result;
}

EJediAiActionResult CJediAiActionJumpForward::onBegin() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionJumpForward::onEnd() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	BASECLASS::onEnd();
}

void CJediAiActionJumpForward::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// compute my target position
	float boundsRadius = simMemory.selfState.collisionRadius;
	CVector wTargetPos = computeTargetPos(simMemory, params.distance);
	CVector iTargetToSelfDir = -simMemory.victimState->iToSelfDir;

	// estimate how long it will take to reach my target
	float distance = simMemory.selfState.wPos.distanceTo(wTargetPos);
	float duration = (
		(kJediJumpForwardEnterDuration - kJediJumpForwardEnterInFlightDuration) + // launch
		(distance / kJediForwardJumpSpeed) +                                                // flight
		(kJediJumpForwardExitDuration - kJediJumpForwardExitInFlightDuration)     // landing
	);

	// if we are attacking as well, update our duration
	float postAttackSimDuration = 0.0f;
	if (params.attack != eJediAiJumpForwardAttack_None) {
		duration -= kJediJumpForwardExitDuration;
		duration += kJediJumpForwardAttackEnterDuration;
		if (params.attack == eJediAiJumpForwardAttack_Vertical) {
			duration += kJediJumpForwardAttackVerticalExitDuration;
		} else if (params.attack == eJediAiJumpForwardAttack_Vertical) {
			duration += kJediJumpForwardAttackHorizontalExitDuration;
		}
		postAttackSimDuration = min(duration * 0.1f, 0.1f);

		// also, clear my victim's velocity
		simMemory.victimState->iVelocity.zero();
	}

	// simulate the jump
	simMemory.setSelfInState(eJediState_Jumping, true);
	CJediAiMemory::SSimulateParams simMemoryParams;
	simMemoryParams.wSelfPos = &wTargetPos;
	simMemoryParams.iSelfFrontDir = &iTargetToSelfDir;
	simMemory.simulate(duration - postAttackSimDuration, simMemoryParams);

	// if we are attacking as well, simulate the attack
	if (params.attack != eJediAiJumpForwardAttack_None) {

		// if we are doing a horizontal attack, compute my damage cone self face pct
		// this is actually cos(a), where a is the damage cone angle
		// conveniently, cos(b), where b is the angle between the direction my self is facing and the
		// direction from my self to a given actor state, is conveniently stored in actorState.selfFacePct,
		// so it is easy to see which actors are within the damage cone by comparing cos(a) and cos(b)
		// also, we give a little room for error (10%) so that we don't miss any hits
		float damageConeSelfFactPct = -1.0f;
		if (params.attack == eJediAiJumpForwardAttack_Horizontal) {
			float adjacent = kJediSaberEffectConeRadius;
			float hypotenuse = sqrt(SQ(kJediSaberEffectConeRadius) + SQ(kJediSaberEffectSlashVicinityRadius));
			damageConeSelfFactPct = (adjacent / hypotenuse) - 0.1f;
		}

		// damage all actors my vicinity
		for (int i = 0; i < simMemory.enemyStateCount; ++i) {
			SJediAiActorState &actorState = simMemory.enemyStates[i];

			// if we are doing a horizontal attack and this actor isn't in our damage cone, ignore it
			if (params.attack == eJediAiJumpForwardAttack_Horizontal) {
				// if this actor isn't close enough, ignore it
				if (actorState.distanceToSelf > kJediSaberEffectSlashVicinityRadius) {
					continue;
				}

				if (actorState.selfFacePct < damageConeSelfFactPct) {
					continue;
				}
			} else if (params.attack == eJediAiJumpForwardAttack_Vertical) {
				// if this actor isn't close enough, ignore it
				if (actorState.distanceToSelf > kJediSaberEffectSlamVicinityRadius) {
					continue;
				}
			}

			// if this actor is shielded, pop the shield
			// otherwise, just apply damage
			if (simMemory.victimState->flags & kJediAiActorStateFlag_Shielded) {
				simMemory.victimState->flags &= ~kJediAiActorStateFlag_Shielded;
			} else {
				simMemory.simulateDamage(simMemory.selfState.saberDamage + kJediSaberEffectDamageStrength, actorState);
			}
			actorState.flags |= kJediAiActorStateFlag_Stumbling;
		}

		// complete the simulation
		simMemory.simulate(postAttackSimDuration, CJediAiMemory::SSimulateParams());
	}

	// we are no longer jumping
	simMemory.setSelfInState(eJediState_Jumping, false);

	// success
	setSimSummary(simSummary, simMemory);
}

void CJediAiActionJumpForward::updateTimers(float dt) {

	// base class version
	BASECLASS::updateTimers(dt);

	// if I'm not already jumping, check if I am now
	if (!data.isJumping) {
		data.isJumping = memory->isSelfInState(eJediState_Jumping);
	}
}

EJediAiActionResult CJediAiActionJumpForward::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// am I jumping this frame?
	bool wasJumping = data.isJumping;
	data.isJumping = memory->isSelfInState(eJediState_Jumping);

	// if I am jumping, I am in progress
	if (data.isJumping) {
		return eJediAiActionResult_InProgress;
	}

	// if I am not already jumping, send the jump command
	if (!wasJumping) {
		memory->selfState.jedi->setCommandJediJumpForward(memory->victimState->actor, params.attack, params.distance);
		return eJediAiActionResult_InProgress;
	}

	// I am done jumping, make sure I got where I wanted to be
	CVector wTargetPos = computeTargetPos(*memory, params.distance);
	float distSq = memory->selfState.wPos.xzDistanceSqTo(wTargetPos);
	const float kMinDist = 2.0f;
	if (distSq > SQ(kMinDist)) {
		return eJediAiActionResult_Failure;
	}

	// success
	return eJediAiActionResult_Success;
}

bool CJediAiActionJumpForward::checkCollision(const CJediAiMemory &memory, float distance) {

	// make sure we have a victim
	if (memory.victimState->actor == NULL || memory.victimState->hitPoints <= 0.0f) {
		return false;
	}

	// compute my target position
	float boundsRadius = memory.selfState.collisionRadius;
	CVector wTargetPos = computeTargetPos(memory, distance);

	// if I can't get to the specified position, I can't do this
	if (!determinePathFindValidity(memory.selfState.wPos, wTargetPos)) {
		return false;
	}

	// if there is something in my way, I can't do this
	CVector iHeightDelta = kUnitVectorY * (10.0f + boundsRadius);
	if (!collideWorldWithMovingSphere(memory.selfState.wPos + iHeightDelta, boundsRadius, wTargetPos - memory.selfState.wPos, NULL)) {
		return false;
	}

	// success!
	return true;
}

CVector CJediAiActionJumpForward::computeTargetPos(const CJediAiMemory &memory, float distance) {

	// get my target's position
	CVector wTargetPos = memory.victimState->wPos;
	if (memory.victimState->actor != NULL) {
		wTargetPos.y = memory.victimFloorHeight;
	}

	// move as close to my victim as possible
	distance += memory.selfState.collisionRadius + memory.victimState->collisionRadius;
	wTargetPos += (memory.victimState->iToSelfDir * distance);
	return wTargetPos;
}


/////////////////////////////////////////////////////////////////////////////
//
// jump over enemy
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionJumpOver::CJediAiActionJumpOver() {
	reset();
}

EJediAiAction CJediAiActionJumpOver::getType() const {
	return eJediAiAction_JumpOver;
}

void CJediAiActionJumpOver::reset() {

	// base class version
	BASECLASS::reset();

	// reset my data
	memset(&params, 0, sizeof(params));
}

EJediAiActionResult CJediAiActionJumpOver::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I can't do this, I can't do this
	if (!simMemory.canSelfDoAction(eJediAction_Jump)) {
		return eJediAiActionResult_Failure;
	}

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return eJediAiActionResult_Failure;
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	if (result == eJediAiActionResult_Failure) {
		return result;
	}

	// if I'm attacking, we need a lightsaber to do this
	if (params.attack && simMemory.selfState.saberDamage <= 0.0f) {
		return eJediAiActionResult_Failure;
	}

	// only check these constraints when I am not in progress
	if (!isInProgress()) {

		// if I have no victim, fail
		if (simMemory.victimState->actor == NULL) {
			return eJediAiActionResult_Failure;
		}

		// if my victim is dead, fail
		if (simMemory.victimState->hitPoints <= 0.0f) {
			return eJediAiActionResult_Failure;
		}

		// if I can't see my victim, bail
		if (!simMemory.victimInView) {
			return eJediAiActionResult_Failure;
		}

		// if I'm not facing my target, fail
		const float kMinFacePct = 0.9f;
		if (simMemory.victimState->selfFacePct < kMinFacePct) {
			return eJediAiActionResult_Failure;
		}

		// if I'm too far away from my victim, I can't jump over it
		if (simMemory.victimState->distanceToSelf > kJediCombatMaxJumpDistance) {
			return eJediAiActionResult_Failure;
		}

		// check collision
		if (simulating) {
			if (!checkCollision(simMemory)) {
				return eJediAiActionResult_Failure;
			}
		}
	}

	// done!
	return result;
}

EJediAiActionResult CJediAiActionJumpOver::onBegin() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionJumpOver::onEnd() {
	BASECLASS::onEnd();
}

void CJediAiActionJumpOver::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// compute my target position
	float boundsRadius = simMemory.selfState.collisionRadius;
	CVector wTargetPos = simMemory.victimState->wPos;
	CVector iSelfToTargetDir = simMemory.selfState.wPos.directionTo(wTargetPos);
	wTargetPos += (iSelfToTargetDir * boundsRadius * 2.0f);
	iSelfToTargetDir = -iSelfToTargetDir;

	// compute my actual jump duration
	float jumpDuration = max(0.0f, kJediJumpOverDuration - kJediJumpOverCrouchDuration);
	float postAttackSimDuration = 0.0f;
	if (params.attack) {
		postAttackSimDuration = min(jumpDuration * 0.1f, 0.1f);
		jumpDuration -= postAttackSimDuration;
	}

	// simulate the jump prep
	CJediAiMemory::SSimulateParams simMemoryParams;
	if (kJediJumpOverCrouchDuration > 0.0f) {
		simMemory.setSelfInState(eJediState_Crouching, true);
		simMemory.simulate(kJediJumpOverCrouchDuration, simMemoryParams);
		simMemory.setSelfInState(eJediState_Crouching, false);
	}

	// simulate the jump
	if (jumpDuration > 0.0f) {
		simMemory.setSelfInState(eJediState_Jumping, true);
		simMemoryParams.wSelfPos = &wTargetPos;
		simMemoryParams.iSelfFrontDir = &iSelfToTargetDir;
		simMemory.simulate(jumpDuration, simMemoryParams);
	}

	// if we are attacking as well, simulate the attack
	if (params.attack) {

		// damage all actors my vicinity
		for (int i = 0; i < simMemory.enemyStateCount; ++i) {
			SJediAiActorState &actorState = simMemory.enemyStates[i];

			// if this actor isn't close enough, ignore it
			if (actorState.distanceToSelf > kJediSaberEffectSlamVicinityRadius) {
				continue;
			}

			// if this actor is shielded, pop the shield
			// otherwise, just apply damage
			if (simMemory.victimState->flags & kJediAiActorStateFlag_Shielded) {
				simMemory.victimState->flags &= ~kJediAiActorStateFlag_Shielded;
			} else {
				simMemory.simulateDamage(simMemory.selfState.saberDamage + kJediSaberEffectDamageStrength, actorState);
			}
			actorState.flags |= kJediAiActorStateFlag_Stumbling;
		}

		// complete the simulation
		simMemory.simulate(postAttackSimDuration, CJediAiMemory::SSimulateParams());
	}

	// I am no longer jumping
	simMemory.setSelfInState(eJediState_Jumping, false);

	// success
	setSimSummary(simSummary, simMemory);
}

EJediAiActionResult CJediAiActionJumpOver::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// am I jumping this frame?
	bool wasJumping = data.isJumping;
	data.isJumping = memory->isSelfInState(eJediState_Jumping);

	// if I am jumping, I am in progress
	if (data.isJumping) {
		return eJediAiActionResult_InProgress;
	}

	// if I am not already jumping, send the jump command
	if (!wasJumping) {
		memory->selfState.jedi->setCommandJediJumpOver(memory->victimState->actor, params.attack);
		data.isJumping = true;
		return eJediAiActionResult_InProgress;
	}

	// I am done
	return eJediAiActionResult_Success;
}

bool CJediAiActionJumpOver::checkCollision(const CJediAiMemory &memory) {

	// make sure we have a victim
	if (memory.victimState->actor == NULL || memory.victimState->hitPoints <= 0.0f) {
		return false;
	}

	// compute positional data
	float boundsRadius = memory.selfState.collisionRadius;
	CVector wTargetPos = memory.victimState->wPos;
	wTargetPos.y = memory.victimFloorHeight;
	CVector iDelta = memory.selfState.wPos.directionTo(wTargetPos) * boundsRadius * 2.0f;
	CVector wDestPos = wTargetPos + iDelta;
	wTargetPos.y += boundsRadius + 0.25f;

	// if I can't get to the specified position, I can't do this
	if (!determinePathFindValidity(memory.selfState.wPos, wDestPos)) {
		return false;
	}

	// if there is something in my way, I can't do this
	CVector iHeightDelta = kUnitVectorY * (10.0f + boundsRadius);
	if (!collideWorldWithMovingSphere(wTargetPos, boundsRadius, iDelta, NULL)) {
		return false;
	}

	// success!
	return true;
}


/////////////////////////////////////////////////////////////////////////////
//
// dodge
//
/////////////////////////////////////////////////////////////////////////////

CJediAiMemory::ECollisionDir convertDodgeDirToCollisionDir(EJediDodgeDir dodgeDir) {
	switch (dodgeDir) {
		case eJediDodgeDir_Left: return CJediAiMemory::eCollisionDir_Left;
		case eJediDodgeDir_Right: return CJediAiMemory::eCollisionDir_Right;
		case eJediDodgeDir_Back: return CJediAiMemory::eCollisionDir_Backward;
	}
	return CJediAiMemory::eCollisionDir_Count;
}

CJediAiActionDodge::CJediAiActionDodge() {
	reset();
}

EJediAiAction CJediAiActionDodge::getType() const {
	return eJediAiAction_Dodge;
}

void CJediAiActionDodge::reset() {

	// base class version
	BASECLASS::reset();

	// reset my data
	memset(&params, 0, sizeof(params));
	memset(&data, 0, sizeof(data));
}

EJediAiActionResult CJediAiActionDodge::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I am in progress, I am complete
	if (isInProgress()) {
		return eJediAiActionResult_Success;
	}

	// if I can't do this, I can't do this
	if (!simMemory.canSelfDoAction(eJediAction_Dodge)) {
		return eJediAiActionResult_Failure;
	}

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return eJediAiActionResult_Failure;
	}

	// if I'm attacking, we need a lightsaber to do this
	if (params.attack && simMemory.selfState.saberDamage <= 0.0f) {
		return eJediAiActionResult_Failure;
	}

	// if there are no threats which I can dodge, bail
	if (params.onlyWhenThreatened && simMemory.highestDodgeableThreatLevel <= 0.0f) {
		return eJediAiActionResult_Failure;
	}

	// if anything is in my way, bail
	CJediAiMemory::ECollisionDir collisionDir = convertDodgeDirToCollisionDir(params.dir);
	if (collisionDir < CJediAiMemory::eCollisionDir_Count) {
		float distance = simMemory.selfState.nearestCollisionTable[collisionDir].distance;
		float requiredDistance = (shouldFlipDodge(simMemory) ? kJediDodgeFlipDistance : kJediDodgeDistance);
		if (distance <= requiredDistance) {
			CActor *actor = simMemory.selfState.nearestCollisionTable[collisionDir].actor;
			return eJediAiActionResult_Failure;
		}
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionDodge::onBegin() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if I've no dodge direction specified, select the best one
	data.dir = params.dir;
	if (data.dir == eJediDodgeDir_None) {
		data.dir = chooseBestDir(*memory);
		if (data.dir == eJediDodgeDir_None) {
			return eJediAiActionResult_Failure;
		}
	}

	// should I flip?
	// if I'm not attacking and I'm either not in melee range or have any explosive threats, I should flip
	bool flip = false;
	if (!params.attack) {
		int numFlippableAttacks = (
			memory->threatTypeDataTable[eJediThreatType_Rush].count +
			memory->threatTypeDataTable[eJediThreatType_Explosion].count +
			memory->threatTypeDataTable[eJediThreatType_Grenade].count +
			memory->threatTypeDataTable[eJediThreatType_Rocket].count
		);
		if ((memory->victimState->distanceToSelf > kJediMeleeCorrectionTweak) || (numFlippableAttacks > 0)) {
			flip = true;
		}
	}

	// send the command
	if (flip) {
		memory->selfState.jedi->setCommandJediDodgeFlip(data.dir);
	} else {
		memory->selfState.jedi->setCommandJediDodge(data.dir, params.attack);
	}

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionDodge::onEnd() {

	// base class version
	BASECLASS::onEnd();

	// clear my data
	memset(&data, 0, sizeof(data));
}

void CJediAiActionDodge::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// if I've no dodge direction specified, select the best one
	EJediDodgeDir dodgeDir = (data.dir != eJediDodgeDir_None ? data.dir : params.dir);
	if (dodgeDir == eJediDodgeDir_None) {
		dodgeDir = chooseBestDir(simMemory);
		if (dodgeDir == eJediDodgeDir_None) {
			return;
		}
	}

	// does this action dodge a threat?
	bool dodgedThreat = (simMemory.dodgeDirThreatLevelTable[dodgeDir] > 0.0f);

	// if there are no threats which I can dodge, bail
	if (params.onlyWhenThreatened && !dodgedThreat) {
		return;
	}

	// compute my target position
	CVector wTargetPos = computeTargetPos(dodgeDir, params.attack, simMemory);
	CVector iFrontDir;
	if (simMemory.victim != NULL) {
		iFrontDir = wTargetPos.directionTo(simMemory.victimState->wPos);
	} else {
		iFrontDir = simMemory.selfState.iFrontDir;
	}

	// if we are attacking as well, damage our target
	if (params.attack && !(simMemory.victimState->flags & kJediAiActorStateFlag_Shielded) && simMemory.selfState.saberDamage > 0.0f) {
		simMemory.simulateDamage(simMemory.selfState.saberDamage, *simMemory.victimState);
	}

	// compute the duration for this dodge
	bool flipDodge = shouldFlipDodge(simMemory);
	float duration = (flipDodge ? kJediDodgeFlipDuration : kJediDodgeDuration);

	// update the sim memory states which are relative to the actor
	CJediAiMemory::SSimulateParams simMemoryParams;
	simMemoryParams.wSelfPos = &wTargetPos;
	simMemoryParams.iSelfFrontDir = &iFrontDir;
	simMemory.setSelfInState(eJediState_Dodging, true);
	simMemory.simulate(duration, simMemoryParams);
	simMemory.setSelfInState(eJediState_Dodging, false);

	// if I was flip-dodging, I am not deflecting
	if (flipDodge) {
		simMemory.setSelfInState(eJediState_Deflecting, false);
	}

	// success
	bool selfWasTooCloseToAnotherJedi = simSummary.selfIsTooCloseToAnotherJedi;
	setSimSummary(simSummary, simMemory);

	// if there were no threats in the direction I am dodging, this dodge is irrelevant
	if (!selfWasTooCloseToAnotherJedi && !simSummary.selfIsTooCloseToAnotherJedi) {
		if (!dodgedThreat && simSummary.result > eJediAiActionSimResult_Irrelevant) {
			simSummary.result = eJediAiActionSimResult_Irrelevant;
		}
	}
}

EJediAiActionResult CJediAiActionDodge::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if we are still dodging, we are still in progress
	if (memory->isSelfInState(eJediState_Dodging)) {
		return eJediAiActionResult_InProgress;
	}

	// success!
	return eJediAiActionResult_Success;
}

EJediDodgeDir CJediAiActionDodge::chooseBestDir(const CJediAiMemory &simMemory) {

	// compute the dodge distance
	float distance = (shouldFlipDodge(simMemory) ? kJediDodgeFlipDistance : kJediDodgeDistance);

	// randomly select between any directions that dodge the max threat level
	int numDirsAvailable = 0;
	float oddsTable[eJediDodgeDir_Count] = {0.0f};
	for (int i = 0; i < eJediDodgeDir_Count; ++i) {
		if (i != eJediDodgeDir_None) {
			if (simMemory.dodgeDirThreatLevelTable[i] >= simMemory.highestDodgeableThreatLevel) {
				if (simMemory.selfState.nearestCollisionTable[i].distance >= distance) {
					oddsTable[i] = 1.0f;
					numDirsAvailable++;
				}
			}
		}
	}

	// if no dirs were available, bail
	if (numDirsAvailable <= 0) {
		return eJediDodgeDir_None;
	}

	// randomly select our direction
	EJediDodgeDir dir = (EJediDodgeDir)randChoice(TR_COUNTOF(oddsTable), oddsTable);
	if (dir < 0) {
		dir = eJediDodgeDir_None;
	}
	return dir;
}

EJediDodgeDir CJediAiActionDodge::chooseRandomDir() {
	EJediDodgeDir table[] = {
		eJediDodgeDir_Left,
		eJediDodgeDir_Right,
		eJediDodgeDir_Back
	};
	int index = rand() % TR_COUNTOF(table);
	return table[index];
}

bool CJediAiActionDodge::shouldFlipDodge(const CJediAiMemory &memory) {

	// if any rocket threats are incoming, flip
	if (memory.threatTypeDataTable[eJediThreatType_Rocket].count > 0) {
		return true;
	}

	// if any explosion threats are close enough, flip
	if (memory.threatTypeDataTable[eJediThreatType_Explosion].count > 0) {
		const SJediAiThreatState *threat = memory.threatTypeDataTable[eJediThreatType_Explosion].shortestDistanceThreat;
		if (threat != NULL && threat->distanceToSelf < (threat->damageRadius + memory.selfState.collisionRadius)) {
			return true;
		}
	}

	// if any grenade threats are close enough, flip
	if (memory.threatTypeDataTable[eJediThreatType_Grenade].count > 0) {
		const SJediAiThreatState *threat = memory.threatTypeDataTable[eJediThreatType_Grenade].shortestDistanceThreat;
		if (threat != NULL && threat->distanceToSelf < (threat->damageRadius + memory.selfState.collisionRadius)) {
			return true;
		}
	}

	// if we are far enough from our target, flip
	if (memory.victimState->distanceToSelf > kJediMeleeCorrectionTweak) {
		return true;
	}

	// no, don't flip
	return false;
}

float CJediAiActionDodge::checkCollision(EJediDodgeDir dir, const CJediAiMemory &memory) {

	// figure out which way we are trying to dodge
	CVector iDodgeDir = computeDodgeDir(dir, memory);

	// compute the dodge distance
	float distance = (shouldFlipDodge(memory) ? kJediDodgeFlipDistance : kJediDodgeDistance);

	// compute my target position
	float radius = memory.selfState.collisionRadius * 0.5f;
	CVector wStartPos = memory.selfState.wBoundsCenterPos + (iDodgeDir * radius);
	CVector iDelta = iDodgeDir * (distance - radius);
	CVector wTargetPos = wStartPos + iDelta;

	// if I can't get to the specified position, I can't do this
	if (!determinePathFindValidity(wStartPos, wTargetPos)) {
		return kJediDodgeDistance;
	}

	// if anything is in my way, we can't do this
	CVector wCollisionPos;
	if (collideWorldWithMovingSphere(wStartPos, radius, iDelta, &wCollisionPos)) {
		float distance = memory.selfState.wPos.distanceTo(wCollisionPos);
		return distance;
	}

	// success!
	return 0.0f;
}

CVector CJediAiActionDodge::computeDodgeDir(EJediDodgeDir dir, const CJediAiMemory &memory) {
	switch (dir) {
		case eJediDodgeDir_Left: return -memory.selfState.iRightDir;
		case eJediDodgeDir_Right: return memory.selfState.iRightDir;
		case eJediDodgeDir_Back: return -memory.selfState.iFrontDir;
		default: return kZeroVector;
	}
}

CVector CJediAiActionDodge::computeTargetPos(EJediDodgeDir dir, bool attack, const CJediAiMemory &memory) {

	// compute the dodge distance
	float distance = (shouldFlipDodge(memory) ? kJediDodgeFlipDistance : kJediDodgeDistance);

	// if moving horizontally, we rotate around our target
	CVector wTargetPos;
	if (memory.victimState->actor != NULL && (dir == eJediDodgeDir_Left || dir == eJediDodgeDir_Right)) {

		// if we are attacking, we move directly to our target pos
		if (attack) {
			wTargetPos = memory.selfState.wPos + ((memory.selfState.iFrontDir * distance * 0.5f) + (computeDodgeDir(dir, memory) * distance));

		// otherwise, we rotate around our target
		} else {

			// compute the angle delta
			float radius = memory.victimState->distanceToSelf;
			float circumference = PI * 2.0f * radius;
			float deltaAngle = TWOPI * (distance / circumference);
			if (dir == eJediDodgeDir_Left) {
				deltaAngle = -deltaAngle;
			}

			// convert the angle delta into x-z delta and update our strafe delta
			float deltaX, deltaZ;
			sinCos(deltaX, deltaZ, deltaAngle);
			CVector iOffsetFromVictim = (
				((deltaX * radius) * memory.victimState->iToSelfDir.crossProduct(kUnitVectorY)) + 
				((deltaZ * radius) * memory.victimState->iToSelfDir)
			);

			// compute our target pos
			wTargetPos = (memory.victimState->wPos + iOffsetFromVictim);
		}

	// otherwise, we just offset from our current pos
	} else {
		wTargetPos = memory.selfState.wPos + (computeDodgeDir(dir, memory) * distance);
	}

	// return the target position
	return wTargetPos;
}


/////////////////////////////////////////////////////////////////////////////
//
// crouch
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionCrouch::CJediAiActionCrouch() {
	reset();
}

EJediAiAction CJediAiActionCrouch::getType() const {
	return eJediAiAction_Crouch;
}

void CJediAiActionCrouch::reset() {

	// base class version
	BASECLASS::reset();

	// reset my data
	memset(&params, 0, sizeof(params));
	memset(&data, 0, sizeof(data));
}

EJediAiActionResult CJediAiActionCrouch::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I can't do this, I can't do this
	if (!simMemory.canSelfDoAction(eJediAction_Crouch)) {
		return eJediAiActionResult_Failure;
	}

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return eJediAiActionResult_Failure;
	}

	// if I am attacking, make sure that I can hit my victim
	if (params.attack) {

		// we need a lightsaber to do this
		if (simMemory.selfState.saberDamage <= 0.0f) {
			return eJediAiActionResult_Failure;
		}

		// if I have no victim, I can't hit it
		if (simMemory.victimState->actor == NULL) {
			return eJediAiActionResult_Failure;
		}

		// if my victim is dead, fail
		if (simMemory.victimState->hitPoints <= 0.0f) {
			return eJediAiActionResult_Failure;
		}

		// if I am not facing my target, I can't hit it
		const float kMinFacePct = 0.5f;
		if (simMemory.victimState->selfFacePct < kMinFacePct) {
			return eJediAiActionResult_Failure;
		}

		// if the target is shielded, I can't do this
		if (simMemory.victimState->flags & kJediAiActorStateFlag_Shielded) {
			return eJediAiActionResult_Failure;
		}

		/// if the target is too far away and isn't in a rush attack, we can't hit it
		//float swingDistance = simMemory.victimState->collisionRadius + kJediSwingSaberDistance;
		//if (simMemory.victimState->distanceToSelf > swingDistance) {
		//	if (!(simMemory.victimState->flags & kJediAiActorStateFlag_InRushAttack)) {
		//		return eJediAiActionResult_Failure;
		//	}
		//}
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionCrouch::onBegin() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if I have no duration, crouch for the recommended duration
	if (params.duration <= 0.0f) {
		data.duration = memory->recommendedCrouchDuration + 0.5f;
		if (params.attack) {
			data.duration = max(0.0f, data.duration - (kJediCrouchAttackDuration / 2.0f));
		} else if (memory->recommendedCrouchDuration <= 0.0f) {
			return eJediAiActionResult_Failure;
		}
	}

	// send the crouch command
	memory->selfState.jedi->setCommandJediCrouch();

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionCrouch::onEnd() {

	// base class version
	BASECLASS::onEnd();

	// clear data
	memset(&data, 0, sizeof(data));
}

void CJediAiActionCrouch::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// if I have no duration, select the best duration
	float duration = data.duration;
	if (duration <= 0.0f) {
		duration = params.duration;
	}
	if (duration <= 0.0f) {
		duration = simMemory.recommendedCrouchDuration;
	}
	if (duration <= 0.0f) {
		simSummary.result = eJediAiActionSimResult_Irrelevant;
		return;
	}

	// remove our expired time
	if (duration < data.timer) {
		duration = 0.0f;
	} else {
		duration -= data.timer;
	}

	// if we are attacking as well, damage our target
	bool attackedVictim = false;
	if (params.attack) {
		if (!(simMemory.victimState->flags & kJediAiActorStateFlag_Shielded) && simMemory.selfState.saberDamage > 0.0f) {
			simMemory.simulateDamage(simMemory.selfState.saberDamage, *simMemory.victimState);
		}
		duration += kJediCrouchAttackDuration;
		attackedVictim = true;
	}

	// update the sim memory states which are relative to the actor
	if (duration > 0.0f || attackedVictim) {
		CJediAiMemory::SSimulateParams simMemoryParams;
		simMemory.setSelfInState(eJediState_Crouching, true);
		simMemory.simulate(duration, simMemoryParams);
		simMemory.setSelfInState(eJediState_Crouching, false);
	}

	// success
	setSimSummary(simSummary, simMemory);
}

void CJediAiActionCrouch::updateTimers(float dt) {

	// update timer
	incrementTimer(data.timer, dt, data.duration);
}

EJediAiActionResult CJediAiActionCrouch::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if we are no longer crouching, bail
	bool willAttack = (params.attack && !data.attacked);
	if ((data.duration <= 0.0f && !willAttack) || !memory->isSelfInState(eJediState_Crouching)) {
		return eJediAiActionResult_Success;
	}

	// if my timer hasn't yet expired, we are still in progress
	if (data.timer < data.duration) {
		memory->selfState.jedi->setCommandJediCrouch();
		return eJediAiActionResult_InProgress;
	}

	// if I am attacking, send the command now
	if (willAttack) {
		memory->selfState.jedi->setCommandJediCrouchAttack();
		data.attacked = true;
		return eJediAiActionResult_InProgress;
	}

	// success!
	return eJediAiActionResult_Success;
}


/////////////////////////////////////////////////////////////////////////////
//
// deflect
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionDeflect::CJediAiActionDeflect() {
	reset();
}

EJediAiAction CJediAiActionDeflect::getType() const {
	return eJediAiAction_Deflect;
}

void CJediAiActionDeflect::reset() {

	// base class version
	BASECLASS::reset();

	// reset my data
	memset(&params, 0, sizeof(params));
	memset(&data, 0, sizeof(data));
}

EJediAiActionResult CJediAiActionDeflect::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I can't do this, I can't do this
	if (!simMemory.canSelfDoAction(eJediAction_Deflect)) {
		return eJediAiActionResult_Failure;
	}

	// if my saber isn't active, I can't deflect
	if (simMemory.selfState.saberDamage <= 0.0f) {
		return eJediAiActionResult_Failure;
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionDeflect::onBegin() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if I have no specified duration, use the recommended one
	data.duration = params.duration;
	if (data.duration <= 0.0f) {
		data.duration = memory->recommendedDeflectionDuration;
		if (data.duration <= 0.0f) {
			return eJediAiActionResult_Failure;
		}
	}

	// send the deflect command
	bool deflectAtEnemies = (params.deflectAtEnemies && !memory->selfState.defensiveModeEnabled);
	memory->selfState.jedi->setCommandJediDeflect(deflectAtEnemies);

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionDeflect::onEnd() {
	BASECLASS::onEnd();

	// clear my data
	memset(&data, 0, sizeof(data));
}

void CJediAiActionDeflect::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// use the correct duration
	float duration = (data.duration > 0.0f ? data.duration : params.duration);

	// remove our expired time
	if (duration < data.timer) {
		duration = 0.0f;
	} else {
		duration -= data.timer;
	}

	// should we extend our duration?
	if (duration <= simMemory.recommendedDeflectionDuration) {
		duration = simMemory.recommendedDeflectionDuration;
		if (duration <= 0.0f) {
			simSummary.result = eJediAiActionSimResult_Irrelevant;
			return;
		}
	}

	// update the sim memory states which are relative to the actor
	if (duration > 0.0f) {
		CJediAiMemory::SSimulateParams simMemoryParams;
		simMemory.setSelfInState(eJediState_Deflecting, true);
		simMemory.setSelfInState(eJediState_Reflecting, (params.deflectAtEnemies && !simMemory.selfState.defensiveModeEnabled));
		simMemory.simulate(duration, simMemoryParams);
		simMemory.setSelfInState(eJediState_Deflecting, false);
		simMemory.setSelfInState(eJediState_Reflecting, false);
	}

	// success
	setSimSummary(simSummary, simMemory);

	// if I'm in progress, this action is cosmetic
	if (simSummary.result == eJediAiActionSimResult_Irrelevant && isInProgress()) {
		simSummary.result = eJediAiActionSimResult_Safe;
	}
}

void CJediAiActionDeflect::updateTimers(float dt) {

	// update the duration while we are in progress
	if (params.duration <= 0.0f ) {
		float remainingTime = (data.duration - data.timer);
		if (remainingTime < memory->recommendedDeflectionDuration) {
			data.duration = (data.timer + memory->recommendedDeflectionDuration);
		}
	}

	// update the timer
	if (data.duration > 0.0f) {
		incrementTimer(data.timer, dt, data.duration);
	}
}

EJediAiActionResult CJediAiActionDeflect::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if my timer is less than the recommended deflection duration, use the larger one
	if (params.duration <= 0.0f ) {
		float remainingTime = (data.duration - data.timer);
		if (remainingTime < memory->recommendedDeflectionDuration) {
			data.duration = (data.timer + memory->recommendedDeflectionDuration);
		}
	}

	// if my timer has expired and there are no more threats which I can deflect, I'm done
	if (data.timer >= data.duration) {
		if (memory->recommendedDeflectionDuration <= 0.0f) {
			return eJediAiActionResult_Success;
		}
	}

	// I'm still deflecting
	bool deflectAtEnemies = (params.deflectAtEnemies && !memory->selfState.defensiveModeEnabled);
	memory->selfState.jedi->setCommandJediDeflect(deflectAtEnemies);
	return eJediAiActionResult_InProgress;
}


/////////////////////////////////////////////////////////////////////////////
//
// block
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionBlock::CJediAiActionBlock() {
	reset();
}

EJediAiAction CJediAiActionBlock::getType() const {
	return eJediAiAction_Block;
}

void CJediAiActionBlock::reset() {

	// base class version
	BASECLASS::reset();

	// reset my data
	memset(&params, 0, sizeof(params));
	memset(&data, 0, sizeof(data));
}

EJediAiActionResult CJediAiActionBlock::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I can't do this, I can't do this
	if (!simMemory.canSelfDoAction(eJediAction_Block)) {
		return eJediAiActionResult_Failure;
	}

	// if my saber isn't active, I can't block
	if (simMemory.selfState.saberDamage <= 0.0f) {
		return eJediAiActionResult_Failure;
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionBlock::onBegin() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if I've no block direction specified, select the best one
	chooseBestParams(*memory, data.params);
	if (data.params.dir == eJediBlockDir_None) {
		return eJediAiActionResult_Failure;
	}

	// send the block command
	memory->selfState.jedi->setCommandJediBlock(data.params.dir);

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionBlock::onEnd() {

	// base class version
	BASECLASS::onEnd();

	// clear my data
	memset(&data, 0, sizeof(data));
}

void CJediAiActionBlock::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return;
	}

	// if I've no block direction specified, select the best one
	SBlockParams blockParams;
	chooseBestParams(simMemory, blockParams);
	if (blockParams.dir == eJediBlockDir_None) {
		simSummary.result = eJediAiActionSimResult_Irrelevant;
		return;
	}

	// update the sim memory states which are relative to the actor
	CJediAiMemory::SSimulateParams simMemoryParams;
	simMemoryParams.blockDir = blockParams.dir;
	simMemory.simulate(blockParams.duration, simMemoryParams);

	// success
	setSimSummary(simSummary, simMemory);
}

void CJediAiActionBlock::updateTimers(float dt) {

	// update the timer
	incrementTimer(data.timer, dt, data.params.duration);
}

EJediAiActionResult CJediAiActionBlock::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if I'm still blocking, wait for my timer to expire
	if (memory->isSelfInState(eJediState_Blocking)) {

		// if my timer hasn't yet expired, we are still in progress
		if (data.timer < data.params.duration) {
			memory->selfState.jedi->setCommandJediBlock(data.params.dir);
			return eJediAiActionResult_InProgress;
		}
	}

	// success!
	return eJediAiActionResult_Success;
}

void CJediAiActionBlock::chooseBestParams(CJediAiMemory &simMemory, SBlockParams &bestParams) const {

	// start with our specified parameters
	bestParams = (data.params.dir != eJediBlockDir_None ? data.params : params);

	// if the specified params don't specify a direction, choose the best one
	if (bestParams.dir == eJediBlockDir_None) {
		bestParams.dir = chooseBestDir(simMemory);
	}

	// if the specified params don't specify a duration, use the one recommended for our direction
	if (bestParams.duration <= 0.0f && bestParams.dir != eJediBlockDir_None) {
		bestParams.duration = simMemory.blockDirThreatInfoTable[bestParams.dir].maxDuration + 0.5f;
	}
}

EJediBlockDir CJediAiActionBlock::chooseBestDir(CJediAiMemory &simMemory) const {

	// if there are no threats to block, bail
	if (simMemory.highestBlockableThreatLevel <= 0.0f) {
		return eJediBlockDir_None;
	}

	// randomly select between any directions that block the max threat level
	int numDirsAvailable = 0;
	float oddsTable[eJediBlockDir_Count] = {0.0f};
	for (int i = 0; i < eJediBlockDir_Count; ++i) {
		if (i != eJediBlockDir_None) {
			if (simMemory.blockDirThreatInfoTable[i].threatLevel >= simMemory.highestBlockableThreatLevel) {
				oddsTable[i] = 1.0f;
				numDirsAvailable++;
			}
		}
	}

	// if no dirs were available, bail
	if (numDirsAvailable <= 0) {
		return eJediBlockDir_None;
	}

	// if we had any dirs besides 'Mid', don't use 'Mid'
	if (numDirsAvailable > 1 && oddsTable[eJediBlockDir_Mid] > 0.0f) {
		oddsTable[eJediBlockDir_Mid] = 0.0f;
		--numDirsAvailable;
	}

	// randomly select our direction
	EJediBlockDir dir = (EJediBlockDir)randChoice(TR_COUNTOF(oddsTable), oddsTable);
	if (dir < 0) {
		dir = eJediBlockDir_None;
	}
	return dir;
}


/////////////////////////////////////////////////////////////////////////////
//
// swing saber
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionSwingSaber::CJediAiActionSwingSaber() {
	reset();
}

EJediAiAction CJediAiActionSwingSaber::getType() const {
	return eJediAiAction_SwingSaber;
}

void CJediAiActionSwingSaber::reset() {

	// base class version
	BASECLASS::reset();

	// reset my data
	memset(&params, 0, sizeof(params));
	memset(&data, 0, sizeof(data));
}

EJediAiActionResult CJediAiActionSwingSaber::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I can't do this, I can't do this
	if (!simMemory.canSelfDoAction(eJediAction_SwingSaber)) {
		return eJediAiActionResult_Failure;
	}

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return eJediAiActionResult_Failure;
	}

	// I can't do this in defensive mode
	if (simMemory.selfState.defensiveModeEnabled) {
		return eJediAiActionResult_Failure;
	}

	// we need a lightsaber to do this
	if (simMemory.selfState.saberDamage <= 0.0f) {
		return eJediAiActionResult_Failure;
	}

	// we need parameters
	if (params.numSwings <= 0) {
		return eJediAiActionResult_Failure;
	}

	// if I have no victim, I can't hit it
	if (simMemory.victimState->actor == NULL) {
		return eJediAiActionResult_Failure;
	}

	// if my victim is dead while simulating, succeed
	if (simulating && simMemory.victimState->hitPoints <= 0.0f) {
		EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
		if (result == eJediAiActionResult_Failure) {
			return result;
		}
		return eJediAiActionResult_Success;
	}

	// if I am not facing my target, I can't hit it
	const float kMinFacePct = 0.5f;
	if (simMemory.victimState->selfFacePct < kMinFacePct) {
		return eJediAiActionResult_Failure;
	}

	// if my target is facing me and I can only attack from behind, I can't do this
	if (params.onlyFromBehind && simMemory.victimState->faceSelfPct > 0.0f) {
		return eJediAiActionResult_Failure;
	}

	// if the target is too far away and isn't in a rush attack, we can't hit it
	float swingDistance = kJediMeleeCorrectionTweak + simMemory.victimState->collisionRadius + simMemory.selfState.collisionRadius;
	if (simMemory.victimState->distanceToSelf > swingDistance) {
		if (!(simMemory.victimState->flags & kJediAiActorStateFlag_InRushAttack)) {
			return eJediAiActionResult_Failure;
		}
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionSwingSaber::onBegin() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionSwingSaber::onEnd() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	BASECLASS::onEnd();
}

void CJediAiActionSwingSaber::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// are we still in the process of swinging?
	bool wasAlreadySwingingSaber = memory->isSelfInState(eJediState_SwingingSaber);

	// iteratively apply all swings
	for (int i = max(0, data.counter); i < params.numSwings; ++i) {

		// unless we are already close enough, move us closer to the victim
		CJediAiMemory::SSimulateParams simParams;
		CVector wNewPos;
		if (kJediSwingSaberDistance < simMemory.victimState->distanceToSelf) {
			wNewPos = simMemory.victimState->wPos + (simMemory.victimState->iToSelfDir * kJediKickDistance);
			simParams.wSelfPos = &wNewPos;
		}

		// simulate half a swing before we simulate damage
		float duration = kJediSwingSaberDuration;
		if (i == 0) {
			if (data.timer < params.timeBeforeSwings) {
				duration += (params.timeBeforeSwings - data.timer);
			}
			duration += kJediSwingSaberEnterDuration;
		}
		duration *= 0.5f;
		if (i > 0) {
			duration += params.timeBetweenSwings;
		}
		simMemory.simulate(duration, simParams);

		// check constraints
		result = checkConstraints(simMemory, true);
		if (result != eJediAiActionResult_InProgress) {
			if (result == eJediAiActionResult_Success) {
				setSimSummary(simSummary, simMemory);
			}
			return;
		}

		// can this swing hit our target?
		bool canHitTarget = true;
		if (simMemory.victimState->flags & kJediAiActorStateFlag_Shielded) {
			canHitTarget = false;
		} else if (
			(simMemory.victimState->combatType == eJediCombatType_Brawler) &&
			(simMemory.victimState->faceSelfPct > 0.25f) &&
			!(simMemory.victimState->flags & kJediAiActorStateFlag_Stumbling)
		) {
			canHitTarget = false;
		}

		// apply damage to our target
		if (canHitTarget) {
			if (simMemory.selfState.saberDamage > 0.0f) {
				simMemory.simulateDamage(simMemory.selfState.saberDamage, *simMemory.victimState);
			}
		}
		bool targetDied = (simMemory.victimState->hitPoints <= 0.0f);

		// if my target is dead, I'm done
		if (targetDied) {
			break;
		}
	}

	// simulate the last half of the final swing
	if (params.numSwings > 0) {
		simMemory.simulate(kJediSwingSaberDuration * 0.5f, CJediAiMemory::SSimulateParams());
	}

	// success
	setSimSummary(simSummary, simMemory);

	// if this action was irrelevant, but we were already in the process of swinging our saber,
	// it looks better to just go ahead and finish it up
	if (simSummary.result == eJediAiActionSimResult_Irrelevant && wasAlreadySwingingSaber && isInProgress()) {
		simSummary.result = eJediAiActionSimResult_Cosmetic;
	}
}

void CJediAiActionSwingSaber::updateTimers(float dt) {

	// update the timer
	float maxTimer = (data.counter <= 0 ? params.timeBeforeSwings : params.timeBetweenSwings);
	incrementTimer(data.timer, dt, maxTimer);
}

EJediAiActionResult CJediAiActionSwingSaber::update(float dt) {

	// if we have swung all of our swings, we are done
	if (data.counter >= params.numSwings) {
		return eJediAiActionResult_Success;
	}

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if I'm not still swinging, determine if it is time to swing again
	if (!memory->isSelfInState(eJediState_SwingingSaber)) {
		float maxTimer = (data.counter <= 0 ? params.timeBeforeSwings : params.timeBetweenSwings);
		if (data.timer >= maxTimer) {
			enqueueSaberSwing();
		}
	}

	// we are still in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionSwingSaber::enqueueSaberSwing() {

	// request a swing
	memory->selfState.jedi->setCommandJediSwingSaber();

	// update our data
	data.timer = 0.0f;
	++data.counter;
}


/////////////////////////////////////////////////////////////////////////////
//
// kick
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionKick::CJediAiActionKick() {
	reset();
}

EJediAiAction CJediAiActionKick::getType() const {
	return eJediAiAction_Kick;
}

void CJediAiActionKick::reset() {

	// base class version
	BASECLASS::reset();

	// reset my params
	memset(&params, 0, sizeof(params));
	params.allowDisplacement = true;
}

EJediAiActionResult CJediAiActionKick::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I can't do this, I can't do this
	if (!simMemory.canSelfDoAction(eJediAction_Kick)) {
		return eJediAiActionResult_Failure;
	}

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return eJediAiActionResult_Failure;
	}

	// I can't do this in defensive mode
	if (simMemory.selfState.defensiveModeEnabled) {
		return eJediAiActionResult_Failure;
	}

	// only check these constraints when I am not in progress
	if (!isInProgress()) {

		// if a victim was not specified, this is irrelevant
		if (simMemory.victimState->actor == NULL) {
			return eJediAiActionResult_Failure;
		}

		// if my victim is dead while simulating, succeed
		if (simulating && simMemory.victimState->hitPoints <= 0.0f) {
			EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
			if (result == eJediAiActionResult_Failure) {
				return result;
			}
			return eJediAiActionResult_Success;
		}

		// if I am not facing my target, I can't hit it
		const float kMinFacePct = 0.75f;
		if (simMemory.victimState->selfFacePct < kMinFacePct) {
			return eJediAiActionResult_Failure;
		}

		// will the victim be coming to me?
		bool victimWillComeToMe = false;
		if ((simMemory.victimState->flags & kJediAiActorStateFlag_InRushAttack)) {
			victimWillComeToMe  = true;
		} else if (simMemory.victimState->enemyType == eJediEnemyType_TrandoshanMelee) {
			if (simMemory.victimState->threatState != NULL && simMemory.victimState->threatState->type == eJediThreatType_Melee) {
				victimWillComeToMe = true;
			}
		}

		// if the victim will not come to me, make sure I can get to him
		if (!victimWillComeToMe) {

			// where will my target be after I start the kick?
			float timeTillDamage = kJediKickDuration * 0.25f;
			CVector wVictimPos = simMemory.victimState->wPos + (simMemory.victimState->iVelocity * timeTillDamage);
			float distToVictimSq = simMemory.selfState.wPos.distanceSqTo(wVictimPos);

			// if the target is too far away and isn't in a rush attack, we can't hit it
			float kickDistance = simMemory.victimState->collisionRadius + simMemory.selfState.collisionRadius;
			if (params.allowDisplacement) {
				kickDistance += kJediMeleeCorrectionTweak;
			}
			if (distToVictimSq > SQ(kickDistance)) {
				return eJediAiActionResult_Failure;
			}
		}
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionKick::onBegin() {

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// send the kick command
	memory->selfState.jedi->setCommandJediKick(params.allowDisplacement);

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionKick::onEnd() {

	// base class version
	BASECLASS::onEnd();
}

void CJediAiActionKick::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// if I'm in progress, bail
	if (isInProgress()) {
		setSimSummary(simSummary, simMemory);
		return;
	}

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// can this kick hit our target?
	// kicks do nothing to armored trandoshan concussives or b2's
	bool canHitTarget = true;
	if (
		(simMemory.victimState->enemyType == eJediEnemyType_B2BattleDroid) ||
		(simMemory.victimState->enemyType == eJediEnemyType_B2RocketDroid) ||
		((simMemory.victimState->enemyType == eJediEnemyType_TrandoshanConcussive) && (simMemory.victimState->flags & kJediAiActorStateFlag_Shielded))
	) {
		canHitTarget = false;
	}

	// apply damage to our target, if we can
	bool victimInRushAttack = ((simMemory.victimState->flags & kJediAiActorStateFlag_InRushAttack) != 0);
	if (canHitTarget) {
		float damage = (victimInRushAttack ? simMemory.victimState->hitPoints : simMemory.selfState.kickDamage);
		simMemory.simulateDamage(damage, *simMemory.victimState);
	}

	// if the enemy isn't dead, these enemy types will stumble back
	if (canHitTarget && simMemory.victimState->hitPoints > 0.0f) {
		switch (simMemory.victimState->enemyType) {
			case eJediEnemyType_TrandoshanInfantry:
			case eJediEnemyType_TrandoshanMelee:
			case eJediEnemyType_TrandoshanCommando:
			case eJediEnemyType_TrandoshanConcussive:
			case eJediEnemyType_B1MeleeDroid: {
				simMemory.victimState->flags |= kJediAiActorStateFlag_Stumbling;
			} break;
		}
	}

	// when we kick an enemy, we stop them
	simMemory.victimState->iVelocity.zero();

	// unless we are already close enough, move us closer to the victim
	CJediAiMemory::SSimulateParams simParams;
	CVector wNewPos;
	if (kJediKickDistance < simMemory.victimState->distanceToSelf && params.allowDisplacement) {
		wNewPos = simMemory.victimState->wPos + (simMemory.victimState->iToSelfDir * kJediKickDistance);
		simParams.wSelfPos = &wNewPos;
	}

	// unless we are already facing closely enough, make us face the victim
	CVector iNewFrontDir = -simMemory.victimState->iToSelfDir;
	float frontDirNoChangePct = iNewFrontDir.dotProduct(simMemory.selfState.iFrontDir);
	if (frontDirNoChangePct < 0.95f) {
		simParams.iSelfFrontDir = &iNewFrontDir;
	}

	// if our victim is in a rush attack, don't worry about the victim timer
	if (victimInRushAttack) {
		simMemory.victimTimer = (simMemory.victimDesiredKillTime - kJediKickDuration + 0.1f);
	}

	// simulate
	simMemory.simulate(kJediKickDuration, simParams);

	// success
	setSimSummary(simSummary, simMemory);
}

EJediAiActionResult CJediAiActionKick::update(float dt) {

	// success!
	return eJediAiActionResult_Success;
}


/////////////////////////////////////////////////////////////////////////////
//
// force push
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionForcePush::CJediAiActionForcePush() {
	reset();
}

EJediAiAction CJediAiActionForcePush::getType() const {
	return eJediAiAction_ForcePush;
}

void CJediAiActionForcePush::reset() {

	// base class version
	BASECLASS::reset();

	// reset my data
	memset(&params, 0, sizeof(params));
	memset(&data, 0, sizeof(data));
}

EJediAiActionResult CJediAiActionForcePush::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I can't do this, I can't do this
	if (!simMemory.canSelfDoAction(eJediAction_ForcePush)) {
		return eJediAiActionResult_Failure;
	}

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return eJediAiActionResult_Failure;
	}

	// only check these constraints when I am not in progress
	if (!isInProgress()) {

		// if I must hit a grenade and there are none, fail
		if (params.mustHitGrenade) {
			if (simMemory.threatTypeDataTable[eJediThreatType_Grenade].count <= 0) {
				return eJediAiActionResult_Failure;
			}

		// otherwise, if we have no enemies, fail
		} else if (simMemory.enemyStateCount <= 0) {
			return eJediAiActionResult_Failure;
		}

		// if we have no victim or our victim is too far away, I can't do this
		if (params.maxVictimDistance > 0.0f && !params.mustHitGrenade) {
			if (simMemory.victimState->actor == NULL) {
				return eJediAiActionResult_Failure;
			} else if (simMemory.victimState->hitPoints <= 0.0f) {
				return eJediAiActionResult_Failure;
			} else if (simMemory.victimState->distanceToSelf > params.maxVictimDistance) {
				return eJediAiActionResult_Failure;
			}
		}
	}

	// if I have a victim, make sure that it's okay to hit him
	if (simMemory.victimState->actor != NULL) {

		// if my victim is targeted by a player, skip it
		if (simMemory.victimState->flags & kJediAiActorStateFlag_TargetedByPlayer) {
			return eJediAiActionResult_Failure;
		}

		// if my victim is engaged with another Jedi, skip it
		if (simMemory.victimState->flags & kJediAiActorStateFlag_EngagedWithOtherJedi) {
			return eJediAiActionResult_Failure;
		}

		// if my victim is being force tk'ed, skip it
		if (simMemory.victimState->flags & kJediAiActorStateFlag_Gripped) {
			return eJediAiActionResult_Failure;
		}
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionForcePush::onBegin() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// stop doing anything else
	memory->selfState.jedi->stop("CJediAiActionForcePush::onBegin");

	// prep the force push
	if (params.skipCharge) {
		memory->selfState.jedi->setCommandJediForcePushThrow();
		data.forcePushed = true;
	} else {
		memory->selfState.jedi->setCommandJediForcePushCharge();
	}

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionForcePush::onEnd() {

	// base class version
	BASECLASS::onEnd();

	// clear my data
	memset(&data, 0, sizeof(data));
}

struct SForcePushSimulateIgnoreActorCbArgs {
	CVector wSelfPos;
	float closestEnemyDistanceSq;
};

void CJediAiActionForcePush::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// if I've already force pushed, I'll do nothing
	if (data.forcePushed) {
		simSummary.result = eJediAiActionSimResult_Irrelevant;
		return;
	}

	// if I'm already force pushing (charging), ignore all blaster threats
	bool isForcePushing = simMemory.isSelfInState(eJediState_ForcePushing);
	if (isForcePushing) {
		int blasterThreatCount = simMemory.threatTypeDataTable[eJediThreatType_Blaster].count;
		for (int i = 0; blasterThreatCount > 0 && i < simMemory.threatStateCount; ++i) {
			SJediAiThreatState &threatState = simMemory.threatStates[i];
			if (threatState.type == eJediThreatType_Blaster) {
				threatState.duration = 0.0f;
				threatState.strength = 0.0f;
				--blasterThreatCount;
			}
		}
	}

	// if I am not fully charged, simulate entering and charging
	float chargeDuration = (params.chargeDuration - data.chargeTimer);
	if (chargeDuration < kJediForcePushEnterDuration && !isForcePushing) {
		chargeDuration = kJediForcePushEnterDuration;
	}
	simMemory.setSelfInState(eJediState_ForcePushing, true);
	if (chargeDuration > 0.0f && !params.skipCharge) {

		// do the sim
		simMemory.simulate(chargeDuration, CJediAiMemory::SSimulateParams());

		// check constraints
		result = checkConstraints(simMemory, true);
		if (result != eJediAiActionResult_InProgress) {
			if (result == eJediAiActionResult_Success) {
				setSimSummary(simSummary, simMemory);
			}
			return;
		}
	}

	// if my closest enemy is outside force push range, it does nothing
	float fpMaxDistance = simMemory.selfState.forcePushMaxDist;
	const float kMinEnemyDistance = (fpMaxDistance * 0.75f);
	float closestEnemyDistance = simMemory.enemyStates[0].distanceToSelf;
	if (closestEnemyDistance > kMinEnemyDistance && !params.mustHitGrenade && !params.mustHitRocket) {
		simSummary.result = eJediAiActionSimResult_Irrelevant;
		return;
	}

	// see if the force push hits anything
	float fpCollisionDistance = fpMaxDistance * 2.0f;
	CVector wStartPos = simMemory.selfState.wPos;
	CVector iDelta = simMemory.selfState.iFrontDir * fpMaxDistance;
	CVector wCollisionPos;
	if (collideWorldWithMovingSphere(wStartPos, simMemory.selfState.forcePushDamageRadius, iDelta, &wCollisionPos)) {
		fpCollisionDistance = wStartPos.distanceTo(wCollisionPos);
	}

	// if I am only force pushing grenades, only simulate what happens to them
	bool pushedSomeone = false;
	if (params.mustHitGrenade) {
		int grenadeThreatCount = simMemory.threatTypeDataTable[eJediThreatType_Grenade].count;
		for (int i = 0, gt = 0; gt < grenadeThreatCount && i < simMemory.threatStateCount; ++i) {

			// skip non-grenade threats
			SJediAiThreatState &threatState = simMemory.threatStates[i];
			if (threatState.type != eJediThreatType_Grenade) {
				continue;
			}
			++gt;

			// get this grenade's force tk object
			SJediAiActorState *objectState = threatState.objectState;
			if (objectState == NULL) {
				continue;
			}

			// if the target is not in the force push cone, skip it
			if (objectState->selfFacePct <= 0.5f) {
				continue;
			}

			// if this target is outside of my force push range, we are done
			if (objectState->distanceToSelf > fpMaxDistance) {
				continue;
			}

			// push this grenade back along our push dir
			float objectSpeed = 20.0f; // stubbed for this example
			threatState.wPos = objectState->wPos;
			threatState.wBoundsCenterPos = objectState->wBoundsCenterPos;
			threatState.wEndPos = objectState->wPos;
			threatState.iFrontDir = objectState->iFrontDir = -objectState->iToSelfDir;
			threatState.iRightDir = objectState->iRightDir = objectState->iFrontDir.crossProduct(kUnitVectorY);
			threatState.iVelocity = objectState->iVelocity = (objectState->iFrontDir * objectSpeed);
			threatState.duration = kJediForcePushExitDuration;
			threatState.wEndPos = objectState->wPos + (threatState.iVelocity * kJediForcePushExitDuration);
			simMemory.updateEntityToSelfState(*objectState);
			simMemory.updateEntityToSelfState(threatState);
			pushedSomeone = true;
		}

		// if I didn't hit a grenade, bail
		if (!pushedSomeone) {
			return;
		}

	// otherwise, if I am only force pushing rockets, simulate what happens to them
	} else if (params.mustHitRocket) {
		int rocketThreatCount = simMemory.threatTypeDataTable[eJediThreatType_Rocket].count;
		for (int i = 0, rt = 0; rt < rocketThreatCount && i < simMemory.threatStateCount; ++i) {

			// skip non-rocket threats
			SJediAiThreatState &threatState = simMemory.threatStates[i];
			if (threatState.type != eJediThreatType_Rocket) {
				continue;
			}
			++rt;

			// if the target is not in the force push cone, skip it
			if (threatState.selfFacePct <= 0.5f) {
				continue;
			}

			// remove this threat
			threatState.duration = 0.0f;
			threatState.strength = 0.0f;
			pushedSomeone = true;
		}

		// if I didn't hit a rocket, bail
		if (!pushedSomeone) {
			return;
		}

	// otherwise, simulate what happens to enemies
	} else {

		// simulate this force push against all enemies
		// the list is sorted by distance to me
		float totalExtraKilledTimers = 0.0f;
		int totalExtraKills = 0;
		for (int i = 0; i < simMemory.enemyStateCount; ++i) {
			float damage = 0.0f;

			// if this target is outside of my force push range, we are done
			SJediAiActorState *target = &simMemory.enemyStates[i];
			if (target->distanceToSelf > fpMaxDistance) {
				break;
			}

			// if the target has no hit points, skip it
			if (target->hitPoints <= 0.0f) {
				continue;
			}

			// if the target is not in the force push cone, skip it
			if (target->selfFacePct <= 0.6f) {
				continue;
			}

			// if the target is targetted by a player or engaged with another Jedi, skip it
			if ((target->flags & kJediAiActorStateFlag_TargetedByPlayer) || (target->flags & kJediAiActorStateFlag_EngagedWithOtherJedi)) {
				continue;
			}

			// if the target is being force tk'ed, skip it
			if (target->flags & kJediAiActorStateFlag_Gripped) {
				continue;
			}

			// droideka lose their shield, but are otherwise immune to force push
			if (target->enemyType == eJediEnemyType_Droideka) {
				if (target->flags & kJediAiActorStateFlag_Shielded) {
					target->flags &= ~kJediAiActorStateFlag_Shielded;
				}
				pushedSomeone = true;
				continue;

			// these units are immune to force push
			} else if (
				(target->enemyType == eJediEnemyType_B2BattleDroid) ||
				(target->enemyType == eJediEnemyType_B2RocketDroid)
			) {
				pushedSomeone = true;
				continue;

			// these units die on force push
			} else if (
				(target->enemyType == eJediEnemyType_B1BattleDroid) ||
				(target->enemyType == eJediEnemyType_B1GrenadeDroid) ||
				(target->enemyType == eJediEnemyType_B1JetpackDroid)
			) {
				pushedSomeone = true;
				if (target->actor != simMemory.victimState->actor) {
					totalExtraKilledTimers += computeTimeToKill(simMemory.selfState.jedi, target->enemyType, simMemory.selfState.skillLevel);
					++totalExtraKills;
				}
				simMemory.simulateDamage(target->hitPoints, *target);
				continue;

			// handle everything else on a case-by-case basis
			} else {

				// compute biped damage
				if (target->actor != NULL) {

					// compute the force push damage tier
					EForcePushDamageTier fpDamageTier = computeForcePushDamageTier(simMemory.selfState.wPos, target->wPos);
					if (fpDamageTier != eForcePushDamageTier_Three) {

						// handle each type specifically
						switch (target->combatType) {

							// melee only takes damage at tier 1 while rushing
							// if he is not lunging but is in tier 1, he rushes
							case eJediCombatType_Brawler: {
								if (fpDamageTier == eForcePushDamageTier_One) {

									// trandoshan melee
									if (target->enemyType == eJediEnemyType_TrandoshanMelee) {
										pushedSomeone = true;
										if (target->flags & kJediAiActorStateFlag_InRushAttack) {
											damage = target->hitPoints;
										} else {
											target->flags |= kJediAiActorStateFlag_InRushAttack;
										}

									// b1 melee droids
									} else if (target->enemyType == eJediEnemyType_B1MeleeDroid) {
										if ((target->flags & kJediAiActorStateFlag_Stumbling)) {
											pushedSomeone = true;
											damage = target->hitPoints;
										}
									}
								}
							} break;

							// these types get pushed and take damage on impact
							case eJediCombatType_Concussive:

								// if the enemy is still armored, this just breaks his armor
								if (simMemory.victimState->flags & kJediAiActorStateFlag_Shielded) {
									simMemory.victimState->flags &= ~kJediAiActorStateFlag_Shielded;
									break;
								}

							case eJediCombatType_AirUnit:
							case eJediCombatType_Infantry: {
								pushedSomeone = true;

								// compute the force push distance
								float fpDistance = 0.0f;
								switch (fpDamageTier) {
									case eForcePushDamageTier_One: {
										target->flags |= (target->combatType == eJediCombatType_Concussive ? kJediAiActorStateFlag_Stumbling : kJediAiActorStateFlag_Incapacitated);
										target->flags |= kJediAiActorStateFlag_Incapacitated;
									} break;
									case eForcePushDamageTier_Two: {
										target->flags |= kJediAiActorStateFlag_Stumbling;
									} break;
								}

								// if the force push distance is greater than the force push collision distance,
								// this actor should hit the collision and take damage
								if (target->distanceToSelf < fpCollisionDistance) {
									float fpTotalDistance = target->distanceToSelf + fpDistance;
									if (fpTotalDistance >= fpCollisionDistance) {
										fpDistance = fpCollisionDistance - target->distanceToSelf;
										iDelta.setLength(fpDistance);
										damage = simMemory.selfState.forcePushDamage;
									}
								}

								// move the target
								target->wPos += iDelta;
								target->wBoundsCenterPos += iDelta;
								target->iFrontDir = target->wPos.directionTo(simMemory.selfState.wPos);
								target->iRightDir = target->iFrontDir.crossProduct(kUnitVectorY);
								target->iVelocity = target->iFrontDir * target->iVelocity.length();
								simMemory.updateEntityToSelfState(*target);

							} break;
						}
					}
				}
			}

			// apply damage to this target
			if (damage > 0.0f) {
				if (damage >= target->hitPoints && (target->actor != simMemory.victimState->actor)) {
					totalExtraKilledTimers += computeTimeToKill(simMemory.selfState.jedi, target->enemyType, simMemory.selfState.skillLevel);
					++totalExtraKills;
				}
				simMemory.simulateDamage(damage, *target);
			}
		}

		// if we've killed more timers than we have already taken, we can't do this
		float extraKillTime = max(0.0f, (simMemory.victimTimer - simMemory.victimDesiredKillTime));
		if (totalExtraKilledTimers > extraKillTime) {
			return;
		}
	}

	// simulate throwing the force push
	simMemory.simulate(kJediForcePushExitDuration, CJediAiMemory::SSimulateParams());
	simMemory.setSelfInState(eJediState_ForcePushing, false);

	// success
	setSimSummary(simSummary, simMemory);

	// if I can push someone, this action is cosmetic
	if (simSummary.result == eJediAiActionSimResult_Irrelevant && pushedSomeone) {
		simSummary.result = eJediAiActionSimResult_Cosmetic;
	}
}

void CJediAiActionForcePush::updateTimers(float dt) {

	// update the timer
	incrementTimer(data.chargeTimer, dt, params.chargeDuration);
}

EJediAiActionResult CJediAiActionForcePush::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if my timer hasn't yet expired, we are still in progress
	if (data.chargeTimer < params.chargeDuration) {
		memory->selfState.jedi->setCommandJediForcePushCharge();
		return eJediAiActionResult_InProgress;
	}

	// if my timer just expired, activate the force push
	if (memory->isSelfInState(eJediState_ForcePushing)) {
		data.forcePushed = true;
		memory->selfState.jedi->setCommandJediForcePushThrow();
		return eJediAiActionResult_InProgress;
	}

	// success!
	return eJediAiActionResult_Success;
}


/////////////////////////////////////////////////////////////////////////////
//
// force tk
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionForceTk::CJediAiActionForceTk() {
	reset();
}

EJediAiAction CJediAiActionForceTk::getType() const {
	return eJediAiAction_ForceTk;
}

void CJediAiActionForceTk::reset() {

	// base class version
	BASECLASS::reset();

	// reset my data
	memset(&params, 0, sizeof(params));
	memset(&data, 0, sizeof(data));
	params.minActivationDistance = kJediAiForceTkActivationDistanceMin;
}

EJediAiActionResult CJediAiActionForceTk::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I can't do this, I can't do this
	if (!simMemory.canSelfDoAction(eJediAction_ForceTk)) {
		return eJediAiActionResult_Failure;
	}

	// if my target is my victim and I can't see it, bail
	if (params.gripTarget == eJediAiForceTkTarget_Victim && !simMemory.victimInView) {
		return eJediAiActionResult_Failure;
	}

	// grab our tk target
	SJediAiActorState *gripTarget = simMemory.forceTkTargetState;
	if (gripTarget->actor == NULL) {
		gripTarget = lookupJediAiForceTkTargetActorState(false, params.gripTarget, simMemory);
		if (gripTarget == NULL) {
			return eJediAiActionResult_Failure;
		}
	}

	// if my target is engaged with another Jedi, bail
	if (gripTarget->flags & kJediAiActorStateFlag_EngagedWithOtherJedi) {
		return eJediAiActionResult_Failure;
	}

	// if my target is a player's target, bail
	if (gripTarget->flags & kJediAiActorStateFlag_TargetedByPlayer) {
		return eJediAiActionResult_Failure;
	}

	// grab our throw target
	SJediAiActorState *throwTarget = lookupJediAiForceTkTargetActorState(true, params.throwTarget, simMemory);
	if (gripTarget == throwTarget) {
		if (params.throwTarget == eJediAiForceTkTarget_Recommended) {
			throwTarget = NULL;
		} else {
			return eJediAiActionResult_Failure;
		}
	}

	// if we are already using forceTk, skip these checks
	if (!simMemory.isSelfInState(eJediState_UsingForceTk)) {

		// if I'm too close to my victim, I can't do this
		if (simMemory.victimState->actor != NULL && simMemory.victimState->distanceToSelf < params.minActivationDistance) {
			return eJediAiActionResult_Failure;
		}

		// if I'm too close to my target, I can't grip it
		if (gripTarget->distanceToSelf < params.minActivationDistance) {
			return eJediAiActionResult_Failure;
		}

		// if I'm too far away from my target, I can't grip it
		if (gripTarget->distanceToSelf > kJediForceSelectRange) {
			return eJediAiActionResult_Failure;
		}

		// if I'm not facing my target enough, I can't grip it
		if (gripTarget->selfFacePct < kJediAiForceTkFacePctMin) {
			return eJediAiActionResult_Failure;
		}

		// if this object isn't throwable and I care, fail
		if (!(gripTarget->flags & kJediAiActorStateFlag_Throwable) && params.failIfNotThrowable) {
			return eJediAiActionResult_Failure;
		}
	}

	// if the target is already being force tk'ed and it isn't a two-player object, I can't do this
	if (
		(gripTarget->flags & kJediAiActorStateFlag_Gripped) &&
		!(gripTarget->flags & kJediAiActorStateFlag_GrippedBySelf) &&
		!(gripTarget->flags & kJediAiActorStateFlag_GripWithTwoPlayers)
	) {
		return eJediAiActionResult_Failure;
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionForceTk::onBegin() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// grab our tk target
	SJediAiActorState *gripTarget = memory->forceTkTargetState;
	if (gripTarget->actor == NULL) {
		gripTarget = lookupJediAiForceTkTargetActorState(false, params.gripTarget, *memory);
		if (gripTarget == NULL) {
			return eJediAiActionResult_Failure;
		}
	}

	// does this object require two hands?
	if (gripTarget->flags & kJediAiActorStateFlag_GripWithTwoHands) {
		data.twoHanded = true;
	}

	// stop doing anything else
	memory->selfState.jedi->stop("CJediAiActionForceTk::onBegin");

	// send the force tk grip command
	memory->selfState.jedi->setCommandJediForceTkGrip(gripTarget->actor, data.twoHanded, 1.0f, params.skipEnter);

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionForceTk::onEnd() {
	BASECLASS::onEnd();

	// clear our data
	memset(&data, 0, sizeof(data));
}

void CJediAiActionForceTk::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// am I already using force tk?
	bool usingForceTk = simMemory.isSelfInState(eJediState_UsingForceTk);

	// get my grip target
	SJediAiActorState *gripTarget = simMemory.forceTkTargetState;
	if (gripTarget->actor == NULL) {
		gripTarget = lookupJediAiForceTkTargetActorState(false, params.gripTarget, simMemory);
		if (gripTarget == NULL) {
			return;
		}
	}

	// if I'm already using force tk (gripping), ignore all blaster threats
	if (simMemory.isSelfInState(eJediState_UsingForceTk)) {
		int blasterThreatCount = simMemory.threatTypeDataTable[eJediThreatType_Blaster].count;
		for (int i = 0; blasterThreatCount > 0 && i < simMemory.threatStateCount; ++i) {
			SJediAiThreatState &threatState = simMemory.threatStates[i];
			if (threatState.type == eJediThreatType_Blaster) {
				threatState.duration = 0.0f;
				threatState.strength = 0.0f;
				--blasterThreatCount;
			}
		}
	}

	// if my target is throwable or we don't care either way, set my grip target as gripped by me
	bool targetCanBeThrown = ((gripTarget->flags & kJediAiActorStateFlag_Throwable) != 0);
	if (!params.failIfNotThrowable || targetCanBeThrown) {
		gripTarget->flags |= kJediAiActorStateFlag_Gripped;
		gripTarget->flags |= kJediAiActorStateFlag_GrippedBySelf;
	}

	// if I am not already finished gripping this object, simulate my remaining grip time
	bool wasUsingForceTk = simMemory.isSelfInState(eJediState_UsingForceTk);
	simMemory.setSelfInState(eJediState_UsingForceTk, true);
	float remainingGripDuration = (params.gripDuration - data.gripTimer);
	bool twoHanded = ((gripTarget->flags & kJediAiActorStateFlag_GripWithTwoHands) != 0);
	if (!data.throwing && (!wasUsingForceTk || remainingGripDuration > 0.0f)) {

		// simulate gripping
		float simDuration = remainingGripDuration;
		if (!wasUsingForceTk && !params.skipEnter) {
			simDuration += (twoHanded ? kJediForceTkTwoHandEnterDuration : kJediForceTkOneHandEnterDuration);
		}
		simMemory.simulate(simDuration, CJediAiMemory::SSimulateParams());
	}

	// if this target is throwable, simulate throwing the target
	if (targetCanBeThrown) {

		// if we have a throw target, both targets will die
		SJediAiActorState *throwTarget = lookupJediAiForceTkTargetActorState(true, params.throwTarget, simMemory);
		if (throwTarget != NULL && throwTarget != gripTarget) {

			// move my grip target to my throw target
			gripTarget->wPos = throwTarget->wPos;
			gripTarget->wBoundsCenterPos = throwTarget->wBoundsCenterPos;
			gripTarget->iFrontDir = simMemory.selfState.wPos.directionTo(gripTarget->wPos);
			gripTarget->iRightDir = gripTarget->iFrontDir.crossProduct(kUnitVectorY);
			gripTarget->iVelocity.zero();
			simMemory.updateEntityToSelfState(*gripTarget);

			// if my grip target is a grenade, update it's threat
			if (gripTarget->threatState != NULL && gripTarget->threatState->type == eJediThreatType_Grenade) {
				gripTarget->threatState->wPos = gripTarget->wPos;
				gripTarget->threatState->wBoundsCenterPos = gripTarget->wBoundsCenterPos;
				gripTarget->threatState->iFrontDir = gripTarget->iFrontDir;
				gripTarget->threatState->iRightDir = gripTarget->iRightDir;
				gripTarget->threatState->iVelocity.zero();
				gripTarget->threatState->duration = 0.0f;
				simMemory.updateEntityToSelfState(*gripTarget->threatState);

			// otherwise, kill both targets
			} else {
				simMemory.simulateDamage(gripTarget->hitPoints, *gripTarget);
				if (throwTarget->flags & kJediAiActorStateFlag_Shielded) {
					throwTarget->flags &= ~kJediAiActorStateFlag_Shielded;
				} else {
					simMemory.simulateDamage(throwTarget->hitPoints, *throwTarget);
				}
			}

		// otherwise, we'll just move our throw grip target
		} else {

			// if we have a specific throw velocity, throw at it
			CVector iThrowDelta;
			if (!params.iThrowVelocity.isCloseTo(kZeroVector, 0.001f)) {
				iThrowDelta = params.iThrowVelocity;

			// otherwise, compute where we'll throw our target
			} else {
				bool throwRight = randBool(0.5f);
				float throwRange = (throwRight ? kJediThrowRange : -kJediThrowRange);
				iThrowDelta = simMemory.selfState.iRightDir * throwRange;
			}

			// throw the target in the specified direction
			gripTarget->wPos += iThrowDelta;
			gripTarget->wBoundsCenterPos += iThrowDelta;
			gripTarget->iFrontDir = gripTarget->wPos.directionTo(simMemory.selfState.wPos);
			gripTarget->iRightDir = gripTarget->iFrontDir.crossProduct(kUnitVectorY);
			simMemory.updateEntityToSelfState(*gripTarget);

			// if the target is a flyer, we only damage it if we are throwing down
			float damage = gripTarget->hitPoints;
			if (gripTarget->combatType == eJediCombatType_AirUnit) {
				if (iThrowDelta.y >= 0.0f) {
					damage = 0.0f;
				}
			}

			// damage the target
			if (damage > 0.0f) {
				simMemory.simulateDamage(damage, *gripTarget);
			}
		}

		// simulate throwing
		float exitDuration = (twoHanded ? kJediForceTkTwoHandExitDuration : kJediForceTkOneHandExitDuration);
		simMemory.simulate(exitDuration, CJediAiMemory::SSimulateParams());

		// if I'm already using force tk (gripping), assume that my kill timer has expired
		if (simMemory.victimTimer <= simMemory.victimDesiredKillTime && wasUsingForceTk) {
			simMemory.victimTimer = simMemory.victimDesiredKillTime + 0.01f;
		}
	}

	// set my grip target as no longer gripped by me
	gripTarget->flags &= ~kJediAiActorStateFlag_Gripped;
	gripTarget->flags &= ~kJediAiActorStateFlag_GrippedBySelf;

	// we are no longer using force tk
	simMemory.setSelfInState(eJediState_UsingForceTk, false);

	// success
	setSimSummary(simSummary, simMemory);

	// if I'm already using force tk (gripping), it looks better for me to continue gripping
	// unless I have good reason to stop
	// I also consider gripping a non-throwable actor as cosmetic
	if (simSummary.result == eJediAiActionSimResult_Irrelevant) {
		if (wasUsingForceTk) {
			simSummary.result = eJediAiActionSimResult_Cosmetic;
		} else if (!targetCanBeThrown) {
			simSummary.result = eJediAiActionSimResult_Cosmetic;
		}
	}
}

void CJediAiActionForceTk::updateTimers(float dt) {

	// we only increment our timer while we aren't blocked
	if (!memory->isSelfInState(eJediState_AiBlocked)) {
		incrementTimer(data.gripTimer, dt, params.gripDuration);
	}
}

EJediAiActionResult CJediAiActionForceTk::update(float dt) {
	bool isInProgress = false;

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// grab our tk target
	SJediAiActorState *gripTarget = memory->forceTkTargetState;
	if (gripTarget->actor == NULL) {
		gripTarget = lookupJediAiForceTkTargetActorState(false, params.gripTarget, *memory);
		if (gripTarget != NULL) {
			return eJediAiActionResult_Failure;
		}
	}

	// if I am using forceTk on a two-player object, just hold it as long as it is tk-able and alive
	// as the Ai, we don't throw a two-player object, we let the player throw it
	if (gripTarget->flags & kJediAiActorStateFlag_GripWithTwoPlayers) {

		// if the object is not alive or not tk-able, we are done
		bool grippable = (gripTarget->flags & kJediAiActorStateFlag_Grippable);
		bool alive = ((gripTarget->flags & kJediAiActorStateFlag_Dead) != 0);
		if (!alive || !grippable) {
			return eJediAiActionResult_Success;
		}

		// otherwise, we are still in progress
		isInProgress = true;

	// otherwise, if my timer hasn't yet expired, we are still in progress
	} else if (data.gripTimer < params.gripDuration) {
		isInProgress = true;
	}

	// if we are still in progress, send the tk command
	if (isInProgress) {
		memory->selfState.jedi->setCommandJediForceTkGrip(gripTarget->actor, data.twoHanded, 1.0f, params.skipEnter);
		return eJediAiActionResult_InProgress;
	}

	// our grip timer just expired, throw our tk target
	if (!data.throwing) {

		// if our target isn't actually gripped by us, just bail
		if (!(gripTarget->flags & kJediAiActorStateFlag_GrippedBySelf)) {
			return eJediAiActionResult_Failure;
		}

		// if we can't throw our grip target and this is a problem, fail
		if (!(gripTarget->flags & kJediAiActorStateFlag_Throwable)) {
			if (params.failIfNotThrowable) {
				return eJediAiActionResult_Failure;
			}

		// if we CAN throw our target, do so
		} else {

			// if we have a throw target, throw at it
			SJediAiActorState *throwTargetState = lookupJediAiForceTkTargetActorState(true, params.throwTarget, *memory);
			if (throwTargetState != NULL) {
				memory->selfState.jedi->setCommandJediForceTkThrowAtTarget(throwTargetState->actor);

			// otherwise, if I have a throw velocity, use it
			} else if (!params.iThrowVelocity.isCloseTo(kZeroVector, 0.001f)) {
				memory->selfState.jedi->setCommandJediForceTkThrow(params.iThrowVelocity);

			// otherwise, throw randomly left or right
			} else {
				bool throwRight = randBool(0.5f);
				float throwRange = (throwRight ? kJediThrowRange : -kJediThrowRange);
				CVector iThrowVelocity = memory->selfState.iRightDir * throwRange;
				iThrowVelocity.y += getGravity() / 2.0f;
				memory->selfState.jedi->setCommandJediForceTkThrow(iThrowVelocity);
			}
			data.throwing = true;
			return eJediAiActionResult_InProgress;
		}
	}

	// if we are still using force tk, we are still in progress
	if (memory->isSelfInState(eJediState_UsingForceTk)) {
		return eJediAiActionResult_InProgress;
	}

	// success!
	return eJediAiActionResult_Success;
}


/////////////////////////////////////////////////////////////////////////////
//
// defensive stance
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionDefensiveStance::CJediAiActionDefensiveStance() {
	reset();
}

EJediAiAction CJediAiActionDefensiveStance::getType() const {
	return eJediAiAction_DefensiveStance;
}

void CJediAiActionDefensiveStance::reset() {

	// base class version
	BASECLASS::reset();

	// reset my data
	memset(&params, 0, sizeof(params));
	memset(&data, 0, sizeof(data));
	params.duration = -1.0f;
}

EJediAiActionResult CJediAiActionDefensiveStance::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return eJediAiActionResult_Failure;
	}

	// if I am exiting on some threat type and there is one, I'm done
	for (int i = 0; i < eJediThreatType_Count; ++i) {
		if (params.exitOnThreat[i]) {
			if (simMemory.threatTypeDataTable[i].count > 0) {
				return eJediAiActionResult_Success;
			}
			break;
		}
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionDefensiveStance::onBegin() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// stand defensively
	memory->selfState.jedi->setCommandJediStandDefensive();

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionDefensiveStance::onEnd() {

	// base class version
	BASECLASS::onEnd();

	// clear my data
	memset(&data, 0, sizeof(data));
}

void CJediAiActionDefensiveStance::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);
	simSummary.ignoreTooCloseToAnotherJedi = true;

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// simulate a frame
	float dt = 1.0f / 30.0f;
	CJediAiMemory::SSimulateParams params;
	simMemory.simulate(dt, params);

	// success
	setSimSummary(simSummary, simMemory);
	if (simSummary.result == eJediAiActionSimResult_Irrelevant) {
		simSummary.result = eJediAiActionSimResult_Cosmetic;
	}
}

void CJediAiActionDefensiveStance::updateTimers(float dt) {

	// update the timer
	if (params.duration >= 0.0f) {
		incrementTimer(data.timer, dt, params.duration);
	}
}

EJediAiActionResult CJediAiActionDefensiveStance::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if my timer hasn't yet expired, we are still in progress
	if (params.duration < 0.0f || data.timer < params.duration) {
		memory->selfState.jedi->setCommandJediStandDefensive();
		return eJediAiActionResult_InProgress;
	}

	// success!
	return eJediAiActionResult_Success;
}


/////////////////////////////////////////////////////////////////////////////
//
// wait for threat
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionWaitForThreat::CJediAiActionWaitForThreat() {
	reset();
}

EJediAiAction CJediAiActionWaitForThreat::getType() const {
	return eJediAiAction_WaitForThreat;
}

void CJediAiActionWaitForThreat::reset() {

	// base class version
	BASECLASS::reset();

	// reset my data
	memset(&params, 0, sizeof(params));
	memset(&data, 0, sizeof(data));
}

EJediAiActionResult CJediAiActionWaitForThreat::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// am I exiting on some threat
	for (int i = 0; i < eJediThreatType_Count; ++i) {
		const SThreatParams &threatParams = params.threatParamTable[i];
		const CJediAiMemory::SJediThreatTypeData &threatTypeData = simMemory.threatTypeDataTable[i];
		if (threatTypeData.count > 0) {
			if (threatParams.inUse) {
				float waitDuration = computeWaitDurationForThreat(threatTypeData, threatParams);
				if (waitDuration <= 0.0f) {
					return eJediAiActionResult_Success;
				}
			}
		}
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionWaitForThreat::onBegin() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// stand defensively
	memory->selfState.jedi->setCommandJediStandDefensive();

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionWaitForThreat::onEnd() {

	// base class version
	BASECLASS::onEnd();

	// clear my data
	memset(&data, 0, sizeof(data));
}

void CJediAiActionWaitForThreat::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// compute my sim duration
	// if there are any threats that will cause me to complete early, use their duration
	float simDuration = (params.duration - data.timer);
	for (int i = 0; i < eJediThreatType_Count; ++i) {
		const SThreatParams &threatParams = params.threatParamTable[i];
		CJediAiMemory::SJediThreatTypeData &threatTypeData = simMemory.threatTypeDataTable[i];
		if (threatParams.inUse && threatTypeData.count > 0) {
			float waitDuration = computeWaitDurationForThreat(threatTypeData, threatParams);
			if (simDuration > waitDuration) {
				simDuration = waitDuration;
			}
		}
	}

	// simulate a frame
	if (simDuration > 0.0f) {
		simMemory.simulate(simDuration, CJediAiMemory::SSimulateParams());
	}

	// success
	setSimSummary(simSummary, simMemory);
	if (simSummary.result == eJediAiActionSimResult_Irrelevant) {
		simSummary.result = eJediAiActionSimResult_Cosmetic;
	}
}

void CJediAiActionWaitForThreat::updateTimers(float dt) {

	// increment our timer
	incrementTimer(data.timer, dt, params.duration);
}

EJediAiActionResult CJediAiActionWaitForThreat::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if my timer hasn't yet expired, we are still in progress
	if (data.timer < params.duration) {
		memory->selfState.jedi->setCommandJediStandDefensive();
		return eJediAiActionResult_InProgress;
	}

	// no threats occurred, I failed
	return eJediAiActionResult_Failure;
}

float CJediAiActionWaitForThreat::computeWaitDurationForThreat(const CJediAiMemory::SJediThreatTypeData &threatTypeData, const SThreatParams &threatParams) const {

	// start with the remaining duration
	float duration = (params.duration - data.timer);
	if (duration < 0.0f) {
		duration = 1e20f;
	}

	// if a max distance was specified, only simulate up until that distance
	bool newDurationSet = false;
	float newDuration = 0.0f;
	if (threatParams.distance > 0.0f && threatTypeData.shortestDistanceThreat != NULL) {
		float remainingDistance = (threatTypeData.shortestDistanceThreat->distanceToSelf - threatParams.distance);
		float speed = threatTypeData.shortestDistanceThreat->iVelocity.length();
		if (speed > 0.0f) {
			newDuration = (remainingDistance / speed);
			newDurationSet = true;
		}
	}

	// if there wasn't one, only simulate up until the next threat arrives
	if (!newDurationSet && threatTypeData.shortestDurationThreat != NULL) {
		newDuration = (threatTypeData.shortestDurationThreat->duration);
	}

	// if our new duration is less than our current duration, use it instead
	if (duration > newDuration) {
		duration = newDuration;
	}

	// if we have a specified duration offset, remove it from our duration
	if (threatParams.durationOffset > 0.0f) {
		duration -= threatParams.durationOffset;
		if (duration < 0.0f) {
			duration = 0.0f;
		}
	}

	// success
	return duration;
}


/////////////////////////////////////////////////////////////////////////////
//
// taunt
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionTaunt::CJediAiActionTaunt() {
	reset();
}

EJediAiAction CJediAiActionTaunt::getType() const {
	return eJediAiAction_Taunt;
}

void CJediAiActionTaunt::reset() {

	// base class version
	BASECLASS::reset();

	// clear params and data
	memset(&params, 0, sizeof(params));
	memset(&data, 0, sizeof(data));
}

EJediAiActionResult CJediAiActionTaunt::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if my victim is already rushing, I'm done
	if (params.skipIfAlreadyRushing && (simMemory.victimState->flags & kJediAiActorStateFlag_InRushAttack)) {
		return eJediAiActionResult_Success;
	}

	// if I can't taunt, fail
	if (!simMemory.canSelfDoAction(eJediAction_Taunt)) {
		return eJediAiActionResult_Failure;
	}

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return eJediAiActionResult_Failure;
	}

	// I can't do this in defensive mode
	if (simMemory.selfState.defensiveModeEnabled) {
		return eJediAiActionResult_Failure;
	}

	// only check these constraints when I am not in progress
	if (!isInProgress()) {

		// if I have no victim, fail
		if (simMemory.victimState->actor == NULL) {
			return eJediAiActionResult_Failure;
		}

		// if my victim is not tauntable, fail
		if (!(simMemory.victimState->flags & kJediAiActorStateFlag_CanBeTaunted)) {
			return eJediAiActionResult_Failure;
		}

		// if my victim is dead, fail
		if (simulating && simMemory.victimState->hitPoints <= 0.0f) {
			return eJediAiActionResult_Failure;
		}

		// if I can't see my victim, bail
		if (!simMemory.victimInView) {
			return eJediAiActionResult_Failure;
		}

		// if my victim is too close, fail
		if (simMemory.victimState->distanceToSelf < params.minDistance) {
			return eJediAiActionResult_Failure;
		}

		// if I'm not facing my victim, I can't do this
		const float kMinSelfFacePct = 0.8f;
		if (simMemory.victimState->selfFacePct < kMinSelfFacePct) {
			return eJediAiActionResult_Failure;
		}

		// if my victim isn't targeting me, fail
		if (!(simMemory.victimState->flags & kJediAiActorStateFlag_TargetingSelf)) {
			return eJediAiActionResult_Failure;
		}
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionTaunt::onBegin() {

	// clear my data
	memset(&data, 0, sizeof(data));

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// send the taunt command
	memory->selfState.jedi->setCommandJediTaunt();

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionTaunt::onEnd() {

	// base class version
	BASECLASS::onEnd();

	// clear my data
	memset(&data, 0, sizeof(data));
}

void CJediAiActionTaunt::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// enrage my victim
	if (simMemory.victimState->actor != NULL) {
		simMemory.victimState->flags |= kJediAiActorStateFlag_InRushAttack;
	}

	// simulate
	float simDuration = (kJediTauntDuration - data.timer);
	simMemory.simulate(simDuration, CJediAiMemory::SSimulateParams());

	// success
	setSimSummary(simSummary, simMemory);
	if (simSummary.result == eJediAiActionSimResult_Irrelevant) {
		simSummary.result = eJediAiActionSimResult_Cosmetic;
	}
}

void CJediAiActionTaunt::updateTimers(float dt) {

	// if I'm over halfway through the taunt, enrage my victim
	float prevTimer = data.timer;
	incrementTimer(data.timer, dt, kJediTauntDuration);
	if ((memory->victimState->flags & kJediAiActorStateFlag_InRushAttack) == 0) {
		float halfDuration = (kJediTauntDuration / 2.0f);
		if (prevTimer < halfDuration && data.timer >= halfDuration) {
			if (randBool(params.enrageTargetOdds)) {
				memory->victimState->actor->taunt();
			}
		}
	}
}

EJediAiActionResult CJediAiActionTaunt::update(float dt) {

	// check constraints
	EJediAiActionResult result = checkConstraints(*memory, false);
	if (result != eJediAiActionResult_InProgress) {
		return result;
	}

	// if our target isn't rushing, fail
	if (params.failUnlessRushing) {
		if (!data.enragedEnemy && (memory->victimState->flags & kJediAiActorStateFlag_InRushAttack) == 0) {
			return eJediAiActionResult_Failure;
		}
	}

	// success!
	return eJediAiActionResult_Success;
}


/////////////////////////////////////////////////////////////////////////////
//
// defend
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionDefend::CJediAiActionDefend() {

	// setup jump forward
	jumpForward.constraint = &jumpForwardThreatConstraint;

	// setup jump over
	jumpOver.constraint = &jumpOverThreatConstraint;

	// build my action table
	memset(actionTable, 0, sizeof(actionTable));
	actionTable[eAction_DodgeLeft] = &dodgeLeft;
	actionTable[eAction_DodgeRight] = &dodgeRight;
	actionTable[eAction_DodgeBack] = &dodgeBack;
	actionTable[eAction_Crouch] = &crouch;
	actionTable[eAction_Deflect] = &deflect;
	actionTable[eAction_Block] = &block;
	actionTable[eAction_JumpForward] = &jumpForward;
	actionTable[eAction_JumpOver] = &jumpOver;
	compileTimeAssert(eAction_Count == 8);

	// setup odds table
	actionOddsTable[eAction_DodgeLeft] = 0.33f;
	actionOddsTable[eAction_DodgeRight] = 0.33f;
	actionOddsTable[eAction_DodgeBack] = 0.33f;
	actionOddsTable[eAction_Crouch] = 1.0f;
	actionOddsTable[eAction_Deflect] = 1.0f;
	actionOddsTable[eAction_Block] = 0.75f;
	actionOddsTable[eAction_JumpForward] = 0.5f;
	actionOddsTable[eAction_JumpOver] = 0.5f;
	compileTimeAssert(eAction_Count == 8);

	// reset my data
	reset();
}

EJediAiAction CJediAiActionDefend::getType() const {
	return eJediAiAction_Defend;
}

void CJediAiActionDefend::reset() {

	// base class version
	BASECLASS::reset();

	// I re-evaluate my actions this often
	selectorParams.selectFrequency = 0.25f;

	// setup dodge left
	dodgeLeft.name = "Dodge Left";
	dodgeLeft.params.onlyWhenThreatened = true;
	dodgeLeft.params.dir = eJediDodgeDir_Left;

	// setup dodge right
	dodgeRight.name = "Dodge Right";
	dodgeRight.params.onlyWhenThreatened = true;
	dodgeRight.params.dir = eJediDodgeDir_Right;

	// setup dodge back
	dodgeBack.name = "Dodge Back";
	dodgeBack.params.onlyWhenThreatened = true;
	dodgeBack.params.dir = eJediDodgeDir_Back;

	// setup crouch
	crouch.name = "Crouch";
	crouch.minRunFrequency = 5.0f;

	// setup deflect
	deflect.name = "Deflect";
	deflect.params.deflectAtEnemies = false;

	// setup block
	block.name = "Block";

	// setup jump forward
	jumpForward.name = "Jump Forward";
	jumpForward.params.distance = 10.0f;
	jumpForward.params.activationDistance = 30.0f;
	jumpForwardThreatConstraint.params.addThreatReaction(eJediThreatType_Blaster, CJediAiActionConstraintThreat::eThreatReaction_FailIfNone);

	// setup jump over
	jumpOver.name = "Jump Over";
	jumpOver.params.attack = false;
	jumpOver.minRunFrequency = 5.0f;
	jumpOverThreatConstraint.params.addThreatReaction(eJediThreatType_Melee, CJediAiActionConstraintThreat::eThreatReaction_FailIfNone);
}

void CJediAiActionDefend::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// if I am dead, I can't do this
	if (simMemory.selfState.hitPoints <= 0.0f) {
		return;
	}

	// if there are no threats, I can't do this
	if (simMemory.threatStateCount <= 0 && !isInProgress()) {
		return;
	}

	// base class version
	BASECLASS::simulate(simMemory);
}

CJediAiAction **CJediAiActionDefend::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}

float *CJediAiActionDefend::getActionOddsTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionOddsTable;
}


/////////////////////////////////////////////////////////////////////////////
//
// idle
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionIdle::CJediAiActionIdle() {

	// setup my ambient actions
	ambient.setAction(0, &ambientActions.strafeLeft, 1.0f);
	ambient.setAction(1, &ambientActions.strafeRight, 1.0f);
	ambient.setAction(2, &ambientActions.strafeForward, 1.0f);
	ambient.setAction(3, &ambientActions.strafeBackward, 1.0f);
	ambient.setAction(4, &ambientActions.flourish, 1.0f);
	compileTimeAssert(ambient.eAction_Count == 5);

	// build my action table
	memset(actionTable, 0, sizeof(actionTable));
	actionTable[eAction_DefensiveStance] = &defensiveStance;
	actionTable[eAction_Ambient] = &ambient;
	compileTimeAssert(eAction_Count == 2);

	// reset my data
	reset();
}

EJediAiAction CJediAiActionIdle::getType() const {
	return eJediAiAction_Idle;
}

void CJediAiActionIdle::reset() {

	// base class version
	BASECLASS::reset();

	// setup defensive stance
	defensiveStance.name = "Defensive Stance";
	defensiveStance.params.duration = 2.0f;

	// setup ambient
	ambient.name = "Ambient";
	ambient.selectorParams.debounceActions = true;
	{
		// strafing constants
		float kStrafeMoveDistMin = 3.0f;
		float kStrafeMoveDistMax = kStrafeMoveDistMin * 2.0f;
		float kStrafeLateralDistMin = kJediSwingSaberDistance;
		float kStrafeLateralDistMax = kStrafeLateralDistMin * 2.0f;

		// setup strafe left
		ambientActions.strafeLeft.name = "Strafe Left";
		ambientActions.strafeLeft.params.dir = eStrafeDir_Left;
		ambientActions.strafeLeft.params.requireVictim = true;
		ambientActions.strafeLeft.params.moveDistanceMin = kStrafeMoveDistMin;
		ambientActions.strafeLeft.params.moveDistanceMax = kStrafeMoveDistMax;
		ambientActions.strafeLeft.params.distanceFromVictimMin = kStrafeLateralDistMin;
		ambientActions.strafeLeft.params.distanceFromVictimMax = kStrafeLateralDistMax;

		// setup strafe right
		ambientActions.strafeRight.name = "Strafe Right";
		ambientActions.strafeRight.params.dir = eStrafeDir_Right;
		ambientActions.strafeRight.params.requireVictim = true;
		ambientActions.strafeRight.params.moveDistanceMin = kStrafeMoveDistMin;
		ambientActions.strafeRight.params.moveDistanceMax = kStrafeMoveDistMax;
		ambientActions.strafeRight.params.distanceFromVictimMin = kStrafeLateralDistMin;
		ambientActions.strafeRight.params.distanceFromVictimMax = kStrafeLateralDistMax;

		// setup strafe forward
		ambientActions.strafeForward.name = "Strafe Forward";
		ambientActions.strafeForward.params.dir = eStrafeDir_Forward;
		ambientActions.strafeForward.params.requireVictim = true;
		ambientActions.strafeForward.params.moveDistanceMin = kStrafeMoveDistMin;
		ambientActions.strafeForward.params.moveDistanceMax = kStrafeMoveDistMax;
		ambientActions.strafeForward.params.distanceFromVictimMin = kStrafeLateralDistMin;

		// setup strafe backward
		ambientActions.strafeBackward.name = "Strafe Backward";
		ambientActions.strafeBackward.params.dir = eStrafeDir_Backward;
		ambientActions.strafeBackward.params.requireVictim = true;
		ambientActions.strafeBackward.params.moveDistanceMin = kStrafeMoveDistMin;
		ambientActions.strafeBackward.params.moveDistanceMax = kStrafeMoveDistMax;
		ambientActions.strafeBackward.params.distanceFromVictimMax = kStrafeLateralDistMax;

		// setup flourish
		ambientActions.flourish.name = "Flourish";
		ambientActions.flourish.params.deflectAtEnemies = false;
		ambientActions.flourish.params.duration = 0.1f;
	}

	// clear my params and data
	memset(&params, 0, sizeof(params));
}

EJediAiActionResult CJediAiActionIdle::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionIdle::onBegin() {

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	return result;
}

void CJediAiActionIdle::onEnd() {

	// base class version
	BASECLASS::onEnd();
}

void CJediAiActionIdle::simulate(CJediAiMemory &simMemory) {

	// base class version
	BASECLASS::simulate(simMemory);

	// we are, at best, cosmetic
	simSummary.result = eJediAiActionSimResult_Cosmetic;
}

EJediAiActionResult CJediAiActionIdle::update(float dt) {

	// base class version
	EJediAiActionResult result = BASECLASS::update(dt);
	return result;
}

CJediAiAction **CJediAiActionIdle::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}


/////////////////////////////////////////////////////////////////////////////
//
// move
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionMove::CJediAiActionMove() {

	// setup move
	move.actionTable[0] = &moveActions.dash;
	move.actionTable[1] = &moveActions.walkRun;
	compileTimeAssert(move.eAction_Count == 2);

	// setup my action table
	actionTable[eAction_Deflect] = &deflect;
	actionTable[eAction_Move] = &move;
	compileTimeAssert(eAction_Count == 2);

	// reset my data
	reset();
}

EJediAiAction CJediAiActionMove::getType() const {
	return eJediAiAction_Move;
}

void CJediAiActionMove::reset() {

	// base class version
	BASECLASS::reset();

	// clear my params and data
	memset(&params, 0, sizeof(params));

	// setup my sequence params
	move.params.allowActionFailure = true;
}

void CJediAiActionMove::updateParams() {

	// update my subaction params
	deflect.params.deflectAtEnemies = false;
	moveActions.walkRun.params.destination = params.destination;
	moveActions.walkRun.params.facePct = params.facePct;
	moveActions.walkRun.params.distance = params.minWalkRunDistance;
	moveActions.dash.params.destination = params.destination;
	moveActions.dash.params.distance = params.minDashDistance;
	moveActions.dash.params.activationDistance = (params.dashActivationDistance == 0.0f ? params.minDashDistance * 2.0f : params.dashActivationDistance);
}

EJediAiActionResult CJediAiActionMove::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return eJediAiActionResult_Failure;
	}

	// if we have an activation distance, check it
	if (params.activationDistance > 0.0f) {

		// get my destination actor
		float minDistance = 0.5f;
		float maxDistance = 1e20f;
		SJediAiActorState *destActorState = lookupJediAiDestinationActorState(params.destination, simMemory, &minDistance, &maxDistance);
		if (destActorState == NULL) {
			return eJediAiActionResult_Failure;
		}

		// if I'm already close enough, succeed or fail
		float distanceFromTarget = limit(minDistance, params.activationDistance, maxDistance);
		if (destActorState->distanceToSelf <= distanceFromTarget) {
			if (params.failIfTooClose) {
				return eJediAiActionResult_Failure;
			} else {
				return eJediAiActionResult_Success;
			}
		}
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionMove::onBegin() {

	// update my parameters
	updateParams();

	// base class version
	EJediAiActionResult result = BASECLASS::onBegin();
	return result;
}

void CJediAiActionMove::onEnd() {

	// base class version
	BASECLASS::onEnd();
}

void CJediAiActionMove::simulate(CJediAiMemory &simMemory) {
	initSimSummary(simSummary, simMemory);

	// get my destination actor
	SJediAiActorState *destActorState = lookupJediAiDestinationActorState(params.destination, simMemory, NULL, NULL);
	if (destActorState == NULL) {
		return;
	}

	// update my parameters
	updateParams();

	// base class version
	BASECLASS::simulate(simMemory);

	// if I'm relevant, make sure that I am
	if (params.isRelevant && simSummary.result == eJediAiActionSimResult_Irrelevant) {
		simSummary.result = eJediAiActionSimResult_Cosmetic;
	}
}

EJediAiActionResult CJediAiActionMove::update(float dt) {

	// update my parameters
	updateParams();

	// base class version
	EJediAiActionResult result = BASECLASS::update(dt);
	return result;
}

CJediAiAction **CJediAiActionMove::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}

EJediAiActionResult *CJediAiActionMove::getActionResultTable(int *actionResultCount) {
	if (actionResultCount != NULL) {
		*actionResultCount = eAction_Count;
	}
	return actionResultTable;
}

bool CJediAiActionMove::doesActionLoop(int actionIndex) const {
	return (actionIndex == eAction_Deflect);
}


/////////////////////////////////////////////////////////////////////////////
//
// blaster counter attack
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionBlasterCounterAttack::CJediAiActionBlasterCounterAttack() {

	// setup move back
	moveBack.name = "Move Back";
	moveBack.constraint = &moveBackDistanceConstraint;
	moveBack.actionTable[0] = &moveBackActions.dodgeBack;
	moveBack.actionTable[1] = &moveBackActions.deflect;
	compileTimeAssert(moveBack.eAction_Count == 2);

	// setup wait for threat
	waitForThreat.constraint = &waitForThreatDistanceConstraint;
	waitForThreatDistanceConstraint.nextConstraint = &waitForThreatBlasterThreatConstraint;
	waitForThreatBlasterThreatConstraint.nextConstraint = &waitForThreatKillTimerConstraint;

	// build my action table
	// if you hit the compileTimeAssert below, you've likely added an
	// action and need to set it up here
	actionTable[eAction_MoveBack] = &moveBack;
	actionTable[eAction_WaitForThreat] = &waitForThreat;
	actionTable[eAction_CounterThreat] = &deflectAttack;
	compileTimeAssert(eAction_Count == 3);

	// reset my data
	reset();
}

EJediAiAction CJediAiActionBlasterCounterAttack::getType() const {
	return eJediAiAction_BlasterCounterAttack;
}

void CJediAiActionBlasterCounterAttack::reset() {

	// base class version
	BASECLASS::reset();

	// move back
	moveBack.name = "Move Back";
	moveBackDistanceConstraint.params.destination = eJediAiDestination_Victim;
	moveBackDistanceConstraint.params.maxDistance = kJediDodgeDistance;
	moveBackDistanceConstraint.params.aboveMaxResult = eJediAiActionResult_Success;
	{
		moveBackActions.dodgeBack.params.dir = eJediDodgeDir_Back;
		moveBackActions.deflect.params.deflectAtEnemies = true;
	}

	// wait for threat
	waitForThreat.name = "Wait for Threat";
	waitForThreatDistanceConstraint.params.destination = eJediAiDestination_Victim;
	waitForThreatDistanceConstraint.params.minDistance = moveBackDistanceConstraint.params.maxDistance;
	waitForThreatBlasterThreatConstraint.params.addThreatReaction(eJediThreatType_Blaster, CJediAiActionConstraintThreat::eThreatReaction_SucceedIfAny);
	waitForThreatKillTimerConstraint.params.maxKillTime = CJediAiActionConstraintKillTimer::kKillTimeDesired;
	waitForThreat.params.threatParamTable[eJediThreatType_Blaster].set(999.0f, 999.0f);
	waitForThreat.params.duration = 10.0f;

	// counter threat
	deflectAttack.name = "Deflect";
	deflectAttack.params.deflectAtEnemies = true;
}

EJediAiActionResult CJediAiActionBlasterCounterAttack::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// if I don't have a victim, fail
	if (simMemory.victimState->actor == NULL) {
		return eJediAiActionResult_Failure;
	}

	// if my target isn't targeting me, fail
	if (!(simMemory.victimState->flags & kJediAiActorStateFlag_TargetingSelf)) {
		return eJediAiActionResult_Failure;
	}

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

void CJediAiActionBlasterCounterAttack::simulate(CJediAiMemory &simMemory) {

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		initSimSummary(simSummary, simMemory);
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// I must be able to deflect to do this
	if (!simMemory.canSelfDoAction(eJediAction_Deflect)) {
		initSimSummary(simSummary, simMemory);
		return;
	}

	// if I can't see my victim, bail
	if (!simMemory.victimInView) {
		return;
	}

	// I can only do this if my victim is targetting me
	if (!(simMemory.victimState->flags & kJediAiActorStateFlag_TargetingSelf)) {
		initSimSummary(simSummary, simMemory);
		return;
	}

	// base class version
	BASECLASS::simulate(simMemory);

	// we are at least beneficial
	if (simSummary.result != eJediAiActionSimResult_Impossible && simSummary.result < eJediAiActionSimResult_Beneficial) {
		simSummary.result = eJediAiActionSimResult_Beneficial;
	}
}

CJediAiAction **CJediAiActionBlasterCounterAttack::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}


/////////////////////////////////////////////////////////////////////////////
//
// engage trandoshan infantry
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionEngageTrandoshanInfantry::CJediAiActionEngageTrandoshanInfantry() {

	// setup force tk attack
	forceTkAttack.actionTable[0] = &forceTkAttackActions.move;
	forceTkAttack.actionTable[1] = &forceTkAttackActions.forceTk;
	compileTimeAssert(forceTkAttack.eAction_Count == 2);

	// setup saber attack
	saberAttack.constraint = &lowSkillLevelConstraint;
	saberAttack.actionTable[0] = &saberAttackActions.move;
	saberAttack.actionTable[1] = &saberAttackActions.saber;
	compileTimeAssert(saberAttack.eAction_Count == 2);

	// setup special saber-kick combo attack
	saberKickCombo.constraint = &highSkillLevelConstraint;
	saberKickCombo.actionTable[0] = &saberKickComboActions.move;
	saberKickCombo.actionTable[1] = &saberKickComboActions.saber;
	saberKickCombo.actionTable[2] = &saberKickComboActions.kick;
	compileTimeAssert(saberKickCombo.eAction_Count == 3);

	// setup special attack
	specialAttack.setAction(0, &specialAttackActions.rushCounterAttack, 0.6f);
	specialAttack.setAction(1, &specialAttackActions.forcePush, 0.4f);
	specialAttack.setAction(2, &specialAttackActions.jumpAttack, 0.4f);
	specialAttack.setAction(3, &specialAttackActions.throwObject, 0.2f);
	compileTimeAssert(specialAttack.eAction_Count == 4);
	{
		// setup special rush counter attack
		specialAttackActions.rushCounterAttack.actionTable[0] = &specialAttackActions.rushCounterAttackActions.taunt;
		specialAttackActions.rushCounterAttack.actionTable[1] = &specialAttackActions.rushCounterAttackActions.counter;
		compileTimeAssert(specialAttackActions.rushCounterAttack.eAction_Count == 2);
		{
			// setup rush counter
			specialAttackActions.rushCounterAttackActions.counter.setAction(0, &specialAttackActions.rushCounterAttackActions.counterActions.kickCounter, 1.0f);
			specialAttackActions.rushCounterAttackActions.counter.setAction(1, &specialAttackActions.rushCounterAttackActions.counterActions.forcePushCounter, 1.0f);
			specialAttackActions.rushCounterAttackActions.counter.setAction(2, &specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounter, 1.0f);
			specialAttackActions.rushCounterAttackActions.counter.setAction(3, &specialAttackActions.rushCounterAttackActions.counterActions.dodgeCounter, 1.0f);
			compileTimeAssert(specialAttackActions.rushCounterAttackActions.counter.eAction_Count == 4);
			{
				// setup kick counter
				specialAttackActions.rushCounterAttackActions.counterActions.kickCounter.constraint = &highSkillLevelConstraint;
				specialAttackActions.rushCounterAttackActions.counterActions.kickCounter.actionTable[0] = &specialAttackActions.rushCounterAttackActions.counterActions.kickCounterActions.wait;
				specialAttackActions.rushCounterAttackActions.counterActions.kickCounter.actionTable[1] = &specialAttackActions.rushCounterAttackActions.counterActions.kickCounterActions.kick;
				compileTimeAssert(specialAttackActions.rushCounterAttackActions.counterActions.kickCounter.eAction_Count == 2);

				// setup force push counter
				specialAttackActions.rushCounterAttackActions.counterActions.forcePushCounter.constraint = &highSkillLevelConstraint;
				specialAttackActions.rushCounterAttackActions.counterActions.forcePushCounter.actionTable[0] = &specialAttackActions.rushCounterAttackActions.counterActions.forcePushCounterActions.wait;
				specialAttackActions.rushCounterAttackActions.counterActions.forcePushCounter.actionTable[1] = &specialAttackActions.rushCounterAttackActions.counterActions.forcePushCounterActions.forcePush;
				compileTimeAssert(specialAttackActions.rushCounterAttackActions.counterActions.forcePushCounter.eAction_Count == 2);

				// setup swing saber
				specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounter.constraint = &lowSkillLevelConstraint;
				specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounter.actionTable[0] = &specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounterActions.wait;
				specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounter.actionTable[1] = &specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounterActions.fakeSim;
				specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounterActions.fakeSim.decoratedAction = &specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounterActions.swingSaber;
				compileTimeAssert(specialAttackActions.rushCounterAttackActions.counterActions.forcePushCounter.eAction_Count == 2);

				// setup dodge counter
				specialAttackActions.rushCounterAttackActions.counterActions.dodgeCounter.actionTable[0] = &specialAttackActions.rushCounterAttackActions.counterActions.dodgeCounterActions.wait;
				specialAttackActions.rushCounterAttackActions.counterActions.dodgeCounter.actionTable[1] = &specialAttackActions.rushCounterAttackActions.counterActions.dodgeCounterActions.dodge;
				compileTimeAssert(specialAttackActions.rushCounterAttackActions.counterActions.dodgeCounter.eAction_Count == 2);
			}
		}

		// setup jump attack
		specialAttackActions.jumpAttack.constraint = &highSkillLevelConstraint;
	}

	// build my action table
	// if you hit the compileTimeAssert below, you've likely added an
	// action and need to set it up here
	memset(actionTable, 0, sizeof(actionTable));
	actionTable[eAction_ForceTkAttack] = &forceTkAttack;
	actionTable[eAction_DeflectAttack] = &deflectAttack;
	actionTable[eAction_SaberAttack] = &saberAttack;
	actionTable[eAction_SaberKickAttack] = &saberKickCombo;
	actionTable[eAction_SpecialAttack] = &specialAttack;
	compileTimeAssert(eAction_Count == 5);

	// every action is equally viable
	for (int i = 0; i < eAction_Count; ++i) {
		actionOddsTable[i] = 1.0f;
	}

	// reset my data
	reset();
}

EJediAiAction CJediAiActionEngageTrandoshanInfantry::getType() const {
	return eJediAiAction_EngageTrandoshanInfantry;
}

void CJediAiActionEngageTrandoshanInfantry::reset() {

	// base class version
	BASECLASS::reset();

	// debounce my actions
	selectorParams.debounceActions = true;

	// setup force tk attack
	forceTkAttack.name = "Force Tk Victim";
	forceTkAttack.params.allowActionFailure = true;
	forceTkAttack.minRunFrequency = 5.0f;
	{
		forceTkAttackActions.move.params.destination = eJediAiDestination_Victim;
		forceTkAttackActions.move.params.minWalkRunDistance = kJediForceSelectRange;
		forceTkAttackActions.move.params.minDashDistance = kJediForceSelectRange;
		forceTkAttackActions.move.params.facePct = 0.8f;
		forceTkAttackActions.forceTk.params.gripDuration = 1.0f;
		forceTkAttackActions.forceTk.params.gripTarget = eJediAiForceTkTarget_Victim;
		forceTkAttackActions.forceTk.params.throwTarget = eJediAiForceTkTarget_Recommended;
	}

	// setup melee attack
	saberAttack.name = "Saber Attack";
	{
		saberAttackActions.move.params.destination = eJediAiDestination_Victim;
		saberAttackActions.move.params.minWalkRunDistance = (kJediMeleeCorrectionTweak * 0.8f);
		saberAttackActions.move.params.minDashDistance = kJediAiDashActivationDistanceMin;
		saberAttackActions.move.params.facePct = 0.8f;
		saberAttackActions.saber.params.numSwings = 2;
	}

	// setup saber-kick combo
	saberKickCombo.name = "Saber-Kick Combo";
	{
		saberKickComboActions.move.params = saberAttackActions.move.params;
		saberKickComboActions.saber.params.numSwings = 2;
	}

	// setup special attacks
	specialAttack.name = "Special Attack";
	specialAttack.selectorParams.debounceActions = true;
	{
		// setup special rush counter attack
		specialAttackActions.rushCounterAttack.name = "Rush Counter";
		{
			// setup rush counter attack - taunt
			specialAttackActions.rushCounterAttackActions.taunt.params.minDistance = 20.0f;
			specialAttackActions.rushCounterAttackActions.taunt.params.enrageTargetOdds = 0.5f;
			specialAttackActions.rushCounterAttackActions.taunt.params.skipIfAlreadyRushing = true;
			specialAttackActions.rushCounterAttackActions.taunt.params.failUnlessRushing = true;
			specialAttackActions.rushCounterAttackActions.taunt.minRunFrequency = 10.0f;

			// setup rush counter attack - counter
			specialAttackActions.rushCounterAttackActions.counter.name = "Counter";
			{
				// setup rush counter attack - counter - kick
				specialAttackActions.rushCounterAttackActions.counterActions.kickCounter.name = "Kick Counter";
				{
					specialAttackActions.rushCounterAttackActions.counterActions.kickCounterActions.wait.params.duration = 5.0f;
					specialAttackActions.rushCounterAttackActions.counterActions.kickCounterActions.wait.params.threatParamTable[eJediThreatType_Rush].set(1.0f, 0.0f);
					specialAttackActions.rushCounterAttackActions.counterActions.kickCounterActions.wait.params.threatParamTable[eJediThreatType_Melee].set(1.0f, 0.0f);
				}

				// setup rush counter attack - counter - force push
				specialAttackActions.rushCounterAttackActions.counterActions.forcePushCounter.name = "Force Push Counter";
				{
					specialAttackActions.rushCounterAttackActions.counterActions.forcePushCounterActions.wait.params.duration = 5.0f;
					specialAttackActions.rushCounterAttackActions.counterActions.forcePushCounterActions.wait.params.threatParamTable[eJediThreatType_Rush].set(kJediForcePushExitDuration * 0.15f, kJediForcePushTier2Min);
					specialAttackActions.rushCounterAttackActions.counterActions.forcePushCounterActions.wait.params.threatParamTable[eJediThreatType_Melee].set(kJediForcePushExitDuration * 0.15f, kJediForcePushTier2Min);
					specialAttackActions.rushCounterAttackActions.counterActions.forcePushCounterActions.forcePush.params.chargeDuration = 0.0f;
					specialAttackActions.rushCounterAttackActions.counterActions.forcePushCounterActions.forcePush.params.skipCharge = true;
				}

				// setup rush counter attack - counter - swing saber
				specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounter.name = "Swing Saber Counter";
				{
					specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounterActions.wait.params.duration = 5.0f;
					specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounterActions.wait.params.threatParamTable[eJediThreatType_Rush].set(0.23f, 0.0f);
					specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounterActions.wait.params.threatParamTable[eJediThreatType_Melee].set(0.23f, 0.0f);
					specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounterActions.fakeSim.params.clearVictimThreats[eJediThreatType_Rush] = true;
					specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounterActions.fakeSim.params.clearVictimThreats[eJediThreatType_Melee] = true;
					specialAttackActions.rushCounterAttackActions.counterActions.swingSaberCounterActions.swingSaber.params.numSwings = 1;
				}

				// setup rush counter attack - counter - dodge
				specialAttackActions.rushCounterAttackActions.counterActions.dodgeCounter.name = "Dodge Counter";
				{
					specialAttackActions.rushCounterAttackActions.counterActions.dodgeCounterActions.wait.params.duration = 5.0f;
					specialAttackActions.rushCounterAttackActions.counterActions.dodgeCounterActions.wait.params.threatParamTable[eJediThreatType_Rush].set(0.75f, 0.0f);
					specialAttackActions.rushCounterAttackActions.counterActions.dodgeCounterActions.wait.params.threatParamTable[eJediThreatType_Melee].set(0.75f, 0.0f);
					specialAttackActions.rushCounterAttackActions.counterActions.dodgeCounterActions.dodge.params.dir = eJediDodgeDir_Right;
				}
			}
		}

		// setup force push
		specialAttackActions.forcePush.name = "Force Push";
		specialAttackActions.forcePush.params.chargeDuration = 1.0f;
		specialAttackActions.forcePush.minRunFrequency = 10.0f;

		// setup jump attack
		specialAttackActions.jumpAttack.name = "Jump Attack";
		specialAttackActions.jumpAttack.params.activationDistance = kJediCombatMaxJumpDistance;
		specialAttackActions.jumpAttack.params.attack = eJediAiJumpForwardAttack_Vertical;

		// setup special force tk attack
		specialAttackActions.throwObject.name = "Throw Object At Victim";
		specialAttackActions.throwObject.minRunFrequency = 10.0f;
		specialAttackActions.throwObject.params.gripDuration = 1.0f;
		specialAttackActions.throwObject.params.gripTarget = eJediAiForceTkTarget_Object;
		specialAttackActions.throwObject.params.throwTarget = eJediAiForceTkTarget_Victim;
	}
}

CJediAiAction **CJediAiActionEngageTrandoshanInfantry::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}

float *CJediAiActionEngageTrandoshanInfantry::getActionOddsTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionOddsTable;
}


/////////////////////////////////////////////////////////////////////////////
//
// melee counter attack
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionMeleeCounterAttack::CJediAiActionMeleeCounterAttack() {

	// setup move
	moveToMeleeRange.constraint = &moveToMeleeRangeThreatConstraint;

	// setup counter attack
	counterThreat.setAction(0, &counterThreatActions.blockAttack, 1.0f);
	counterThreat.setAction(1, &counterThreatActions.passiveBlock, 1.0f);
	counterThreat.setAction(2, &counterThreatActions.dodgeAttackLeft, 0.5f);
	counterThreat.setAction(3, &counterThreatActions.dodgeAttackRight, 0.5f);
	counterThreat.setAction(4, &counterThreatActions.crouchAttack, 1.0f);
	counterThreat.setAction(5, &counterThreatActions.jumpOverAttack, 1.0f);
	counterThreat.setAction(6, &counterThreatActions.kickSaberCombo, 1.0f);
	compileTimeAssert(counterThreat.eAction_Count == 7);
	{
		// setup close counter block attack action
		counterThreatActions.blockAttack.actionTable[0] = &counterThreatActions.blockAttackActions.block;
		counterThreatActions.blockAttack.actionTable[1] = &counterThreatActions.blockAttackActions.saberFakeSim;
		counterThreatActions.blockAttackActions.saberFakeSim.decoratedAction = &counterThreatActions.blockAttackActions.saber;
		compileTimeAssert(counterThreatActions.blockAttack.eAction_Count == 2);

		// setup close counter kick + saber combo action
		counterThreatActions.kickSaberCombo.actionTable[0] = &counterThreatActions.kickSaberComboActions.kick;
		counterThreatActions.kickSaberCombo.actionTable[1] = &counterThreatActions.kickSaberComboActions.saber;
		compileTimeAssert(counterThreatActions.kickSaberCombo.eAction_Count == 2);
	}

	// build my action table
	// if you hit the compileTimeAssert below, you've likely added an
	// action and need to set it up here
	actionTable[eAction_MoveToMeleeRange] = &moveToMeleeRange;
	actionTable[eAction_WaitForThreat] = &waitForThreat;
	actionTable[eAction_CounterThreat] = &counterThreat;
	compileTimeAssert(eAction_Count == 3);

	// reset my data
	reset();
}

EJediAiAction CJediAiActionMeleeCounterAttack::getType() const {
	return eJediAiAction_MeleeCounterAttack;
}

void CJediAiActionMeleeCounterAttack::reset() {

	// base class version
	BASECLASS::reset();

	// move to melee range
	moveToMeleeRange.params.destination = eJediAiDestination_Victim;
	moveToMeleeRange.params.facePct = 0.8f;
	moveToMeleeRange.params.minDashDistance = kJediAiDashActivationDistanceMin;
	moveToMeleeRange.params.minWalkRunDistance = kJediSwingSaberDistance + 4.0f;
	moveToMeleeRangeThreatConstraint.params.addThreatReaction(eJediThreatType_Melee, CJediAiActionConstraintThreat::eThreatReaction_SucceedIfAny);
	moveToMeleeRangeThreatConstraint.params.addThreatReaction(eJediThreatType_Rush, CJediAiActionConstraintThreat::eThreatReaction_SucceedIfAny);

	// wait for threat
	waitForThreat.params.threatParamTable[eJediThreatType_Melee].set(999.0f, 999.0f);
	waitForThreat.params.threatParamTable[eJediThreatType_Rush].set(999.0f, 999.0f);
	waitForThreat.params.duration = 10.0f;

	// counter
	counterThreat.name = "Counter";
	counterThreat.selectorParams.debounceActions = true;
	{
		// block attack
		counterThreatActions.blockAttack.name = "Block Attack";
		counterThreatActions.blockAttackActions.saberFakeSim.name = "SwingSaber Fake Sim";
		counterThreatActions.blockAttackActions.saberFakeSim.params.makeVictimStumble = true;
		counterThreatActions.blockAttackActions.saber.params.numSwings = 1;

		// passive block, dodge
		counterThreatActions.passiveBlock.name = "Passive Block";
		counterThreatActions.passiveBlock.params.dir = eJediBlockDir_Mid;

		// dodge attack left
		counterThreatActions.dodgeAttackLeft.name = "Dodge Attack (Left)";
		counterThreatActions.dodgeAttackLeft.params.dir = eJediDodgeDir_Left;
		counterThreatActions.dodgeAttackLeft.params.attack = true;
		counterThreatActions.dodgeAttackLeft.params.onlyWhenThreatened = true;

		// dodge attack right
		counterThreatActions.dodgeAttackRight.name = "Dodge Attack (Right)";
		counterThreatActions.dodgeAttackRight.params.dir = eJediDodgeDir_Right;
		counterThreatActions.dodgeAttackRight.params.attack = true;
		counterThreatActions.dodgeAttackRight.params.onlyWhenThreatened = true;

		// crouch attack
		counterThreatActions.crouchAttack.name = "Crouch Attack";
		counterThreatActions.crouchAttack.params.attack = true;

		// jump over attack
		counterThreatActions.jumpOverAttack.name = "Jump Over Attack";
		counterThreatActions.jumpOverAttack.params.attack = true;

		// setup kick + saber combo
		counterThreatActions.kickSaberCombo.name = "Kick + Saber Combo";
		{
			// setup kick
			counterThreatActions.kickSaberComboActions.kick.name = "Kick";
			counterThreatActions.kickSaberComboActions.kick.params.allowDisplacement = false;

			// setup swing saber
			counterThreatActions.kickSaberComboActions.saber.name = "Saber";
			counterThreatActions.kickSaberComboActions.saber.params.numSwings = 2;
		}
	}
}

void CJediAiActionMeleeCounterAttack::simulate(CJediAiMemory &simMemory) {

	// check constraints
	EJediAiActionResult result = checkConstraints(simMemory, true);
	if (result != eJediAiActionResult_InProgress) {
		initSimSummary(simSummary, simMemory);
		if (result == eJediAiActionResult_Success) {
			setSimSummary(simSummary, simMemory);
		}
		return;
	}

	// if I can't see my victim, bail
	if (!simMemory.victimInView) {
		return;
	}

	// if I am knocked around, fail
	if (simMemory.isSelfInState(eJediState_KnockedAround)) {
		return;
	}

	// I can only do this if my victim is targetting me
	if (!(simMemory.victimState->flags & kJediAiActorStateFlag_TargetingSelf)) {
		initSimSummary(simSummary, simMemory);
		return;
	}

	// base class version
	BASECLASS::simulate(simMemory);

	// we are at least beneficial
	if (simSummary.result < eJediAiActionSimResult_Beneficial) {
		simSummary.result = eJediAiActionSimResult_Beneficial;
	}
}

CJediAiAction **CJediAiActionMeleeCounterAttack::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}


/////////////////////////////////////////////////////////////////////////////
//
// engage trandoshan melee
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionEngageTrandoshanMelee::CJediAiActionEngageTrandoshanMelee() {

	// setup kick fail
	kickFail.constraint = &kickFailThreatConstraint;
	kickFailThreatConstraint.nextConstraint = &belowDesiredKillTimerLowSkillLevelConstraint;

	// setup special attack
	specialAttack.setAction(0, &specialAttackActions.jumpForwardAttack, 0.5f);
	specialAttack.setAction(1, &specialAttackActions.forcePushCounter, 0.25f);
	specialAttack.setAction(2, &specialAttackActions.forceTkDodgeThreat, 0.25f);
	specialAttack.setAction(3, &specialAttackActions.forceTkIgnoreThreat, 0.25f);
	compileTimeAssert(specialAttack.eAction_Count == 4);
	{
		// setup jump forward attack
		specialAttackActions.jumpForwardAttack.constraint = &highSkillLevelConstraint;

		// setup force push counter combo
		specialAttackActions.forcePushCounter.constraint = &specialAttackActions.forcePushCounterDistanceConstraint;
		specialAttackActions.forcePushCounter.actionTable[0] = &specialAttackActions.forcePushCounterActions.forcePush;
		specialAttackActions.forcePushCounter.actionTable[1] = &specialAttackActions.forcePushCounterActions.waitForRushThreat;
		specialAttackActions.forcePushCounter.actionTable[2] = &specialAttackActions.forcePushCounterActions.counter;
		compileTimeAssert(specialAttackActions.forcePushCounter.eAction_Count == 3);
		{
			// setup rush counter
			specialAttackActions.forcePushCounterActions.counter.setAction(0, &specialAttackActions.forcePushCounterActions.counterActions.kickCounter, 0.5f);
			specialAttackActions.forcePushCounterActions.counter.setAction(1, &specialAttackActions.forcePushCounterActions.counterActions.forcePushCounter, 0.5f);
			specialAttackActions.forcePushCounterActions.counter.setAction(2, &specialAttackActions.forcePushCounterActions.counterActions.failToSwingSaber, 0.5f);
			compileTimeAssert(specialAttackActions.forcePushCounterActions.counter.eAction_Count == 3);
			{
				// setup kick counter
				specialAttackActions.forcePushCounterActions.counterActions.kickCounter.actionTable[0] = &specialAttackActions.forcePushCounterActions.counterActions.kickCounterActions.wait;
				specialAttackActions.forcePushCounterActions.counterActions.kickCounter.actionTable[1] = &specialAttackActions.forcePushCounterActions.counterActions.kickCounterActions.kick;
				compileTimeAssert(specialAttackActions.forcePushCounterActions.counterActions.kickCounter.eAction_Count == 2);

				// setup force push counter
				specialAttackActions.forcePushCounterActions.counterActions.forcePushCounter.actionTable[0] = &specialAttackActions.forcePushCounterActions.counterActions.forcePushCounterActions.wait;
				specialAttackActions.forcePushCounterActions.counterActions.forcePushCounter.actionTable[1] = &specialAttackActions.forcePushCounterActions.counterActions.forcePushCounterActions.forcePush;
				compileTimeAssert(specialAttackActions.forcePushCounterActions.counterActions.forcePushCounter.eAction_Count == 2);

				// setup fail to swing saber
				specialAttackActions.forcePushCounterActions.counterActions.failToSwingSaber.constraint = &belowDesiredKillTimerConstraint;
				specialAttackActions.forcePushCounterActions.counterActions.failToSwingSaber.actionTable[0] = &specialAttackActions.forcePushCounterActions.counterActions.failToSwingSaberActions.wait;
				specialAttackActions.forcePushCounterActions.counterActions.failToSwingSaber.actionTable[1] = &specialAttackActions.forcePushCounterActions.counterActions.failToSwingSaberActions.ignoreThreat;
				specialAttackActions.forcePushCounterActions.counterActions.failToSwingSaberActions.ignoreThreat.decoratedAction = &specialAttackActions.forcePushCounterActions.counterActions.failToSwingSaberActions.swingSaber;
				compileTimeAssert(specialAttackActions.forcePushCounterActions.counterActions.failToSwingSaber.eAction_Count == 2);
			}
		}

		// setup force tk (dodge threat)
		specialAttackActions.forceTkDodgeThreat.constraint = &specialAttackActions.forceTkDodgeThreatConstraint;
		specialAttackActions.forceTkDodgeThreatConstraint.nextConstraint = &belowDesiredKillTimerHighSkillLevelConstraint;

		// setup force tk (ignore threat)
		specialAttackActions.forceTkIgnoreThreat.actionTable[0] = &specialAttackActions.forceTkIgnoreThreatActions.forceTk;
		specialAttackActions.forceTkIgnoreThreat.actionTable[1] = &specialAttackActions.forceTkIgnoreThreatActions.clearLungeThreat;
		compileTimeAssert(specialAttackActions.forceTkIgnoreThreat.eAction_Count == 2);
		specialAttackActions.forceTkIgnoreThreat.constraint = &specialAttackActions.forceTkIgnoreThreatConstraint;
		specialAttackActions.forceTkIgnoreThreatConstraint.nextConstraint = &belowDesiredKillTimerLowSkillLevelConstraint;
		{
			// setup forceTk
			specialAttackActions.forceTkIgnoreThreatActions.forceTk.constraint = &specialAttackActions.forceTkIgnoreThreatActions.succeedOnLungeThreat;

			// setup swing saber
			specialAttackActions.forceTkIgnoreThreatActions.clearLungeThreat.decoratedAction = &specialAttackActions.forceTkIgnoreThreatActions.swingSaber;
			specialAttackActions.forceTkIgnoreThreatActions.clearLungeThreat.constraint = &specialAttackActions.forceTkIgnoreThreatActions.requireLungeThreat;
		}
	}

	// setup swingSaberUntilCountered
	swingSaberUntilCounteredFakeSim.decoratedAction = &swingSaberUntilCountered;
	swingSaberUntilCounteredFakeSim.constraint = &swingSaberUntilCounteredThreatConstraint;
	swingSaberUntilCounteredFakeSim.minRunFrequency = 8.0f;
	swingSaberUntilCounteredThreatConstraint.nextConstraint = &belowDesiredKillTimerLowSkillLevelConstraint;

	// setup swingSaberOnce
	swingSaberOnceFakeSim.decoratedAction = &swingSaberOnce;
	swingSaberOnceFakeSim.constraint = &swingSaberOnceThreatConstraint;
	swingSaberOnceFakeSim.minRunFrequency = 3.0f;
	swingSaberOnceThreatConstraint.nextConstraint = &belowDesiredKillTimerHighSkillLevelConstraint;

	// build my action table
	// if you hit the compileTimeAssert below, you've likely added an
	// action and need to set it up here
	memset(actionTable, 0, sizeof(actionTable));
	actionTable[eAction_CounterAttack] = &counterAttack;
	actionTable[eAction_KickFail] = &kickFail;
	actionTable[eAction_SpecialAttack] = &specialAttack;
	actionTable[eAction_SwingSaberUntilCountered] = &swingSaberUntilCounteredFakeSim;
	actionTable[eAction_SwingSaberOnce] = &swingSaberOnceFakeSim;
	compileTimeAssert(eAction_Count == 5);

	// build my odds table
	memset(actionOddsTable, 0, sizeof(actionOddsTable));
	for (int i = 0; i < eAction_Count; ++i) {
		actionOddsTable[i] = 1.0f;
	}

	// reset my data
	reset();
}

EJediAiAction CJediAiActionEngageTrandoshanMelee::getType() const {
	return eJediAiAction_EngageTrandoshanMelee;
}

void CJediAiActionEngageTrandoshanMelee::reset() {

	// base class version
	BASECLASS::reset();

	// debounce my actions
	selectorParams.debounceActions = true;

	// setup kick fail
	kickFail.name = "Kick Fail";
	kickFail.params.allowDisplacement = true;
	kickFail.minRunFrequency = 10.0f;
	kickFailThreatConstraint.params.addThreatReaction(eJediThreatType_Melee, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
	kickFailThreatConstraint.params.addThreatReaction(eJediThreatType_Rush, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);

	// setup special attack
	specialAttack.name = "Special Attack";
	specialAttack.selectorParams.debounceActions = true;
	{
		// setup jump forward attack
		specialAttackActions.jumpForwardAttack.name = "Jump Forward Attack";
		specialAttackActions.jumpForwardAttack.params.activationDistance = kJediCombatMaxJumpDistance;
		specialAttackActions.jumpForwardAttack.params.attack = eJediAiJumpForwardAttack_Vertical;

		// setup force push counter combo
		specialAttackActions.forcePushCounterDistanceConstraint.params.destination = eJediAiDestination_Victim;
		specialAttackActions.forcePushCounterDistanceConstraint.params.maxDistance = kJediForcePushTier2Min;
		specialAttackActions.forcePushCounterDistanceConstraint.skipWhileInProgress = true;
		specialAttackActions.forcePushCounter.name = "Force Push Counter";
		specialAttackActions.forcePushCounter.params.allowActionFailure = true;
		{
			// setup force push
			specialAttackActions.forcePushCounterActions.forcePush.name = "Force Push Counter Setup";
			specialAttackActions.forcePushCounterActions.forcePush.params.chargeDuration = 1.0f;
			specialAttackActions.forcePushCounterActions.forcePush.params.maxVictimDistance = kJediForcePushTier2Min;

			// setup wait for rush threat
			specialAttackActions.forcePushCounterActions.waitForRushThreat.name = "Wait For Rush Threat";
			specialAttackActions.forcePushCounterActions.waitForRushThreat.params.duration = 3.0f;
			specialAttackActions.forcePushCounterActions.waitForRushThreat.params.threatParamTable[eJediThreatType_Rush].set(9999.0f, 9999.0f);

			// setup counter
			specialAttackActions.forcePushCounterActions.counter.name = "Counter";
			specialAttackActions.forcePushCounterActions.counter.selectorParams.debounceActions = true;
			{
				// setup kick counter
				specialAttackActions.forcePushCounterActions.counterActions.kickCounter.name = "Kick Counter";
				{
					specialAttackActions.forcePushCounterActions.counterActions.kickCounterActions.wait.params.duration = 5.0f;
					specialAttackActions.forcePushCounterActions.counterActions.kickCounterActions.wait.params.threatParamTable[eJediThreatType_Rush].set(0.9f, 0.0f);
					specialAttackActions.forcePushCounterActions.counterActions.kickCounterActions.kick.params.allowDisplacement = false;
				}

				// setup force push counter
				specialAttackActions.forcePushCounterActions.counterActions.forcePushCounter.name = "Force Push Counter";
				{
					specialAttackActions.forcePushCounterActions.counterActions.forcePushCounterActions.wait.params.duration = 5.0f;
					specialAttackActions.forcePushCounterActions.counterActions.forcePushCounterActions.wait.params.threatParamTable[eJediThreatType_Rush].set(0.0f, kJediForcePushTier2Min * 0.75f);
					specialAttackActions.forcePushCounterActions.counterActions.forcePushCounterActions.forcePush.params.skipCharge = true;
					specialAttackActions.forcePushCounterActions.counterActions.forcePushCounterActions.forcePush.params.chargeDuration = 0.0f;
				}

				// setup fail to swing saber
				specialAttackActions.forcePushCounterActions.counterActions.failToSwingSaber.name = "Fail to Swing Saber";
				{
					specialAttackActions.forcePushCounterActions.counterActions.failToSwingSaberActions.wait.params.duration = 10.0f;
					specialAttackActions.forcePushCounterActions.counterActions.failToSwingSaberActions.wait.params.threatParamTable[eJediThreatType_Rush].set(0.18f, 0.0f);
					specialAttackActions.forcePushCounterActions.counterActions.failToSwingSaberActions.ignoreThreat.params.clearVictimThreats[eJediThreatType_Rush] = true;
					specialAttackActions.forcePushCounterActions.counterActions.failToSwingSaberActions.swingSaber.params.numSwings = true;
				}
			}
		}

		// setup force tk (dodge threat)
		specialAttackActions.forceTkDodgeThreat.name = "Force Tk (Dodge Threat)";
		specialAttackActions.forceTkDodgeThreat.params.gripDuration = 6.0f;
		specialAttackActions.forceTkDodgeThreat.params.gripTarget = eJediAiForceTkTarget_Victim;
		specialAttackActions.forceTkDodgeThreat.params.throwTarget = eJediAiForceTkTarget_Recommended;
		specialAttackActions.forceTkDodgeThreat.minRunFrequency = 8.0f;
		specialAttackActions.forceTkDodgeThreatConstraint.params.addThreatReaction(eJediThreatType_Melee, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);

		// setup force tk (ignore threat)
		specialAttackActions.forceTkIgnoreThreat.name = "Force Tk (Ignore Threat)";
		specialAttackActions.forceTkIgnoreThreat.minRunFrequency = 8.0f;
		specialAttackActions.forceTkIgnoreThreatConstraint.params.addThreatReaction(eJediThreatType_Rush, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
		specialAttackActions.forceTkIgnoreThreatConstraint.params.addThreatReaction(eJediThreatType_Melee, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
		specialAttackActions.forceTkIgnoreThreatConstraint.skipWhileInProgress = true;
		{
			// setup force tk
			specialAttackActions.forceTkIgnoreThreatActions.forceTk.name = "Grip";
			specialAttackActions.forceTkIgnoreThreatActions.forceTk.params.gripDuration = 6.0f;
			specialAttackActions.forceTkIgnoreThreatActions.forceTk.params.gripTarget = eJediAiForceTkTarget_Victim;
			specialAttackActions.forceTkIgnoreThreatActions.forceTk.params.throwTarget = eJediAiForceTkTarget_Recommended;
			specialAttackActions.forceTkIgnoreThreatActions.succeedOnLungeThreat.params.addThreatReaction(eJediThreatType_Rush, CJediAiActionConstraintThreat::eThreatReaction_SucceedIfAny);

			// setup swing saber
			specialAttackActions.forceTkIgnoreThreatActions.swingSaber.name = "Swing Saber (Fail)";
			specialAttackActions.forceTkIgnoreThreatActions.swingSaber.params.numSwings = 1;
			specialAttackActions.forceTkIgnoreThreatActions.swingSaber.params.timeBeforeSwings = 0.75f;
			specialAttackActions.forceTkIgnoreThreatActions.clearLungeThreat.name = "Clear Lunge Threats";
			specialAttackActions.forceTkIgnoreThreatActions.clearLungeThreat.params.clearVictimThreats[eJediThreatType_Rush] = true;
			specialAttackActions.forceTkIgnoreThreatActions.clearLungeThreat.params.minResult = eJediAiActionSimResult_Beneficial;
			specialAttackActions.forceTkIgnoreThreatActions.requireLungeThreat.params.addThreatReaction(eJediThreatType_Rush, CJediAiActionConstraintThreat::eThreatReaction_FailIfNone);
			specialAttackActions.forceTkIgnoreThreatActions.requireLungeThreat.skipWhileSimulating = true;
		}
	}

	// setup swingSaberUntilCountered
	swingSaberUntilCounteredFakeSim.name = "Swing Saber Until Countered Fake Sim";
	swingSaberUntilCounteredFakeSim.params.makeVictimStumble = true;
	swingSaberUntilCounteredFakeSim.minRunFrequency = 4.0f;
	swingSaberUntilCountered.name = "Swing Saber Until Countered";
	swingSaberUntilCountered.params.numSwings = 3;
	swingSaberUntilCounteredThreatConstraint.params.addThreatReaction(eJediThreatType_Melee, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
	swingSaberUntilCounteredThreatConstraint.params.addThreatReaction(eJediThreatType_Rush, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
	swingSaberUntilCounteredThreatConstraint.skipWhileInProgress = true;

	// setup swingSaberOnce
	swingSaberOnceFakeSim.name = "Swing Saber Once Fake Sim";
	swingSaberOnceFakeSim.params.makeVictimStumble = true;
	swingSaberOnceFakeSim.minRunFrequency = 3.0f;
	swingSaberOnce.name = "Swing Saber Once";
	swingSaberOnce.params.numSwings = 1;
	swingSaberOnceThreatConstraint.params.addThreatReaction(eJediThreatType_Melee, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
	swingSaberOnceThreatConstraint.params.addThreatReaction(eJediThreatType_Rush, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
}

CJediAiAction **CJediAiActionEngageTrandoshanMelee::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}

float *CJediAiActionEngageTrandoshanMelee::getActionOddsTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionOddsTable;
}


/////////////////////////////////////////////////////////////////////////////
//
// engage trandoshan commando
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionEngageTrandoshanCommando::CJediAiActionEngageTrandoshanCommando() {

	// setup saber attack
	saberAttack.actionTable[0] = &saberAttackActions.move;
	saberAttack.actionTable[1] = &saberAttackActions.saber;
	compileTimeAssert(saberAttack.eAction_Count == 2);

	// setup kick + force tk combo
	kickForceTkCombo.constraint = &highSkillLevelConstraint;
	kickForceTkCombo.actionTable[0] = &kickForceTkComboActions.move;
	kickForceTkCombo.actionTable[1] = &kickForceTkComboActions.kick;
	kickForceTkCombo.actionTable[2] = &kickForceTkComboActions.forceTk;
	compileTimeAssert(kickForceTkCombo.eAction_Count == 3);

	// setup force push + force tk combo
	forcePushForceTkCombo.constraint = &highSkillLevelConstraint;
	forcePushForceTkCombo.actionTable[0] = &forcePushForceTkComboActions.move;
	forcePushForceTkCombo.actionTable[1] = &forcePushForceTkComboActions.forcePush;
	forcePushForceTkCombo.actionTable[2] = &forcePushForceTkComboActions.forceTk;
	compileTimeAssert(forcePushForceTkCombo.eAction_Count == 3);
	{
		// setup force push
		forcePushForceTkComboActions.forcePush.constraint = &forcePushForceTkComboActions.forcePushDistanceConstraint;
	}

	// setup special attack
	specialAttack.setAction(0, &specialAttackActions.pushGrenade, 0.75f);
	specialAttack.setAction(1, &specialAttackActions.throwGrenade, 0.75f);
	specialAttack.setAction(2, &specialAttackActions.jumpAttack, 0.5f);
	specialAttack.setAction(3, &specialAttackActions.kickSaberCombo, 0.25f);
	compileTimeAssert(specialAttack.eAction_Count == 4);
	{
		// setup push grenade
		specialAttackActions.pushGrenade.constraint = &specialAttackActions.onlyIfGrenadeThreat;
		specialAttackActions.onlyIfGrenadeThreat.nextConstraint = &lowSkillLevelConstraint;

		// setup force tk grenade
		specialAttackActions.throwGrenade.constraint = &highSkillLevelConstraint;

		// setup jump attack
		specialAttackActions.jumpAttack.constraint = &highSkillLevelConstraint;

		// setup special attack kick-saber combo
		specialAttackActions.kickSaberCombo.actionTable[0] = &specialAttackActions.kickSaberComboActions.move;
		specialAttackActions.kickSaberCombo.actionTable[1] = &specialAttackActions.kickSaberComboActions.kick;
		specialAttackActions.kickSaberCombo.actionTable[2] = &specialAttackActions.kickSaberComboActions.saber;
		compileTimeAssert(specialAttackActions.kickSaberCombo.eAction_Count == 3);
	}

	// setup ambient actions
	ambient.constraint = &belowDesiredKillTimerConstraint;
	ambient.actionTable[0] = &ambientActions.forceTkFakeSim;
	ambientActions.forceTkFakeSim.decoratedAction = &ambientActions.forceTk;
	ambient.actionTable[1] = &ambientActions.dashAttack;
	ambient.actionTable[2] = &ambientActions.jumpOverAttack;
	compileTimeAssert(ambient.eAction_Count == 3);

	// build my action table
	// if you hit the compileTimeAssert below, you've likely added an
	// action and need to set it up here
	memset(actionTable, 0, sizeof(actionTable));
	actionTable[eAction_SaberAttack] = &saberAttack;
	actionTable[eAction_KickForceTkCombo] = &kickForceTkCombo;
	actionTable[eAction_ForcePushForceTkCombo] = &forcePushForceTkCombo;
	actionTable[eAction_ForcePushAttack] = &forcePush;
	actionTable[eAction_SpecialAttack] = &specialAttack;
	actionTable[eAction_Ambient] = &ambient;
	compileTimeAssert(eAction_Count == 6);

	// every action is equally viable
	for (int i = 0; i < eAction_Count; ++i) {
		actionOddsTable[i] = 1.0f;
	}

	// reset my data
	reset();
}

EJediAiAction CJediAiActionEngageTrandoshanCommando::getType() const {
	return eJediAiAction_EngageTrandoshanCommando;
}

void CJediAiActionEngageTrandoshanCommando::reset() {

	// base class version
	BASECLASS::reset();

	// debounce my actions
	selectorParams.debounceActions = true;

	// setup saber attack
	saberAttack.name = "Saber Attack";
	{
		// setup move to saber range
		saberAttackActions.move.name = "Move To Saber Range";
		saberAttackActions.move.params.destination = eJediAiDestination_Victim;
		saberAttackActions.move.params.minWalkRunDistance = kJediMeleeCorrectionTweak;
		saberAttackActions.move.params.minDashDistance = kJediAiDashActivationDistanceMin;
		saberAttackActions.move.params.facePct = 0.8f;

		// setup swing saber
		saberAttackActions.saber.params.numSwings = 2;
	}

	// setup kick + force tk combo
	kickForceTkCombo.name = "Kick + Force Tk Combo";
	{
		// setup move to kick range
		kickForceTkComboActions.move.name = "Move To Kick Range";
		kickForceTkComboActions.move.params.destination = eJediAiDestination_Victim;
		kickForceTkComboActions.move.params.minWalkRunDistance = kJediMeleeCorrectionTweak;
		kickForceTkComboActions.move.params.minDashDistance = kJediMeleeCorrectionTweak;
		kickForceTkComboActions.move.params.facePct = 0.8f;

		// setup kick
		kickForceTkComboActions.kick.name = "Kick";
		kickForceTkComboActions.kick.params.allowDisplacement = true;

		// setup force tk
		kickForceTkComboActions.forceTk.name = "Force Tk";
		kickForceTkComboActions.forceTk.params.gripDuration = 0.5f;
		kickForceTkComboActions.forceTk.params.gripTarget = eJediAiForceTkTarget_Victim;
		kickForceTkComboActions.forceTk.params.throwTarget = eJediAiForceTkTarget_Recommended;
		kickForceTkComboActions.forceTk.params.failIfNotThrowable = true;
		kickForceTkComboActions.forceTk.params.minActivationDistance = 0.0f;
		kickForceTkComboActions.forceTk.params.skipEnter = true;
	}

	// setup force push + force tk combo
	forcePushForceTkCombo.name = "Force Push + Force Tk Combo";
	{
		// setup move to force push range
		forcePushForceTkComboActions.move.name = "Move To Force Push Range";
		forcePushForceTkComboActions.move.params.destination = eJediAiDestination_Victim;
		forcePushForceTkComboActions.move.params.minWalkRunDistance = ((kJediForcePushTier2Min + kJediForcePushTier3Min) / 2.0f);
		forcePushForceTkComboActions.move.params.minDashDistance = forcePushForceTkComboActions.move.params.minWalkRunDistance;
		forcePushForceTkComboActions.move.params.facePct = 0.8f;

		// setup force push
		forcePushForceTkComboActions.forcePushDistanceConstraint.params.destination = eJediAiDestination_Victim;
		forcePushForceTkComboActions.forcePushDistanceConstraint.params.minDistance = kJediForcePushTier2Min;
		forcePushForceTkComboActions.forcePushDistanceConstraint.params.maxDistance = kJediForcePushTier3Min;
		forcePushForceTkComboActions.forcePush.name = "Force Push";
		forcePushForceTkComboActions.forcePush.params.skipCharge = false;
		forcePushForceTkComboActions.forcePush.params.chargeDuration = 0.0f;

		// setup force tk
		forcePushForceTkComboActions.forceTk.name = "Force Tk";
		forcePushForceTkComboActions.forceTk.params.gripDuration = 0.5f;
		forcePushForceTkComboActions.forceTk.params.gripTarget = eJediAiForceTkTarget_Victim;
		forcePushForceTkComboActions.forceTk.params.throwTarget = eJediAiForceTkTarget_Recommended;
		forcePushForceTkComboActions.forceTk.params.failIfNotThrowable = true;
		forcePushForceTkComboActions.forceTk.params.minActivationDistance = 0.0f;
		forcePushForceTkComboActions.forceTk.params.skipEnter = true;
	}

	// setup force push
	forcePush.name = "Force Push";
	forcePush.minRunFrequency = 10.0f;
	forcePush.params.chargeDuration = 1.0f;

	// setup special attack
	specialAttack.name = "Special Attack";
	specialAttack.selectorParams.debounceActions = true;
	{
		// setup special push grenade action
		specialAttackActions.onlyIfGrenadeThreat.params.addThreatReaction(eJediThreatType_Grenade, CJediAiActionConstraintThreat::eThreatReaction_FailIfNone);
		specialAttackActions.pushGrenade.name = "Force Push Grenade";
		specialAttackActions.pushGrenade.params.chargeDuration = 0.0f;
		specialAttackActions.pushGrenade.params.mustHitGrenade = true;

		// setup special throw grenade action
		specialAttackActions.throwGrenade.name = "ForceTk Grenade";
		specialAttackActions.throwGrenade.params.gripDuration = 0.0f;
		specialAttackActions.throwGrenade.params.gripTarget = eJediAiForceTkTarget_Grenade;
		specialAttackActions.throwGrenade.params.throwTarget = eJediAiForceTkTarget_Victim;

		// setup jump attack
		specialAttackActions.jumpAttack.params.activationDistance = kJediCombatMaxJumpDistance;
		specialAttackActions.jumpAttack.params.attack = eJediAiJumpForwardAttack_Vertical;

		// setup kick-saber combo
		specialAttackActions.kickSaberCombo.name = "Kick-Saber Combo";
		{
			// setup move
			specialAttackActions.kickSaberComboActions.move.name = "Move To Kick Range";
			specialAttackActions.kickSaberComboActions.move.params.destination = eJediAiDestination_Victim;
			specialAttackActions.kickSaberComboActions.move.params.minWalkRunDistance = kJediMeleeCorrectionTweak;
			specialAttackActions.kickSaberComboActions.move.params.minDashDistance = kJediAiDashActivationDistanceMin;
			specialAttackActions.kickSaberComboActions.move.params.facePct = 0.8f;

			// setup kick
			specialAttackActions.kickSaberComboActions.kick.params.allowDisplacement = true;

			// setup swing saber
			specialAttackActions.kickSaberComboActions.saber.params.numSwings = 2;
		}
	}

	// setup ambient actions
	ambient.name = "Ambient Actions";
	ambient.selectorParams.debounceActions = true;
	{
		// setup force tk
		ambientActions.forceTk.name = "Fail Force Tk";
		ambientActions.forceTk.params.gripDuration = 0.0f;
		ambientActions.forceTk.params.gripTarget = eJediAiForceTkTarget_Victim;
		ambientActions.forceTkFakeSim.name = "Fail Force Tk Fake Sim";
		ambientActions.forceTkFakeSim.minRunFrequency = 10.0f;
		ambientActions.forceTkFakeSim.params.damageVictim = 5.0f;
		ambientActions.forceTkFakeSim.params.postSim = true;

		// setup dash attack
		ambientActions.dashAttack.name = "Fail Dash Attack";
		ambientActions.dashAttack.params.attack = true;
		ambientActions.dashAttack.params.destination = eJediAiDestination_Victim;
		ambientActions.dashAttack.params.activationDistance = kJediAiDashActivationDistanceMin;
		ambientActions.dashAttack.params.distance = kJediSwingSaberDistance;

		// setup jump over attack
		ambientActions.jumpOverAttack.name = "Fail Jump Over Attack";
		ambientActions.jumpOverAttack.params.attack = true;
	}
}

CJediAiAction **CJediAiActionEngageTrandoshanCommando::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}

float *CJediAiActionEngageTrandoshanCommando::getActionOddsTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionOddsTable;
}


/////////////////////////////////////////////////////////////////////////////
//
// engage trandoshan concussive
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionEngageTrandoshanConcussive::CJediAiActionEngageTrandoshanConcussive() {

	// setup saber attack
	saberAttack.actionTable[0] = &saberAttackActions.move;
	saberAttack.actionTable[1] = &saberAttackActions.saberSelector;
	compileTimeAssert(saberAttack.eAction_Count == 2);
	{
		// setup saber selector
		saberAttackActions.saberSelector.condition = &highSkillLevelConstraint;
		saberAttackActions.saberSelector.onSuccess = &saberAttackActions.saberSelectorActions.highSkillSaber;
		saberAttackActions.saberSelector.onFailure = &saberAttackActions.saberSelectorActions.lowSkillSaberFakeSim;
		saberAttackActions.saberSelectorActions.lowSkillSaberFakeSim.decoratedAction = &saberAttackActions.saberSelectorActions.lowSkillSaber;
	}

	// setup jump forward attack
	jumpForwardAttack.constraint = &highSkillLevelConstraint;

	// setup special attack
	specialAttack.setAction(0, &specialAttackActions.kickSaberCombo, 0.6f);
	specialAttack.setAction(1, &specialAttackActions.forcePush, 0.4f);
	specialAttack.setAction(2, &specialAttackActions.throwObject, 0.2f);
	specialAttack.setAction(3, &specialAttackActions.jumpOverAttack, 0.4f);
	compileTimeAssert(specialAttack.eAction_Count == 4);
	{
		// setup special attack kick-saber combo
		specialAttackActions.kickSaberCombo.actionTable[0] = &specialAttackActions.kickSaberComboActions.move;
		specialAttackActions.kickSaberCombo.actionTable[1] = &specialAttackActions.kickSaberComboActions.kick;
		specialAttackActions.kickSaberCombo.actionTable[2] = &specialAttackActions.kickSaberComboActions.saber;
		compileTimeAssert(specialAttackActions.kickSaberCombo.eAction_Count == 3);
	}

	// setup force tk attack
	forceTkAttack.constraint = &threatConstraint;
	forceTkAttack.actionTable[0] = &forceTkAttackActions.move;
	forceTkAttack.actionTable[1] = &forceTkAttackActions.grip;
	compileTimeAssert(forceTkAttack.eAction_Count == 2);
	{
		// setup grip
		forceTkAttackActions.grip.setAction(0, &forceTkAttackActions.gripActions.bailOnThreat, 0.5f);
		forceTkAttackActions.grip.setAction(1, &forceTkAttackActions.gripActions.ignoreThreat, 0.5f);
		forceTkAttackActions.gripActions.ignoreThreat.decoratedAction = &forceTkAttackActions.gripActions.takeDamage;
		compileTimeAssert(forceTkAttackActions.grip.eAction_Count == 2);
	}

	// build my action table
	// if you hit the compileTimeAssert below, you've likely added an
	// action and need to set it up here
	memset(actionTable, 0, sizeof(actionTable));
	actionTable[eAction_SaberAttack] = &saberAttack;
	actionTable[eAction_JumpForwardAttack] = &jumpForwardAttack;
	actionTable[eAction_SpecialAttack] = &specialAttack;
	actionTable[eAction_ForceTkAttack] = &forceTkAttack;
	compileTimeAssert(eAction_Count == 4);

	// setup my action odds table
	memset(actionOddsTable, 0, sizeof(actionOddsTable));
	for (int i = 0; i < eAction_Count; ++i) {
		actionOddsTable[i] = 1.0f;
	}

	// reset my data
	reset();
}

EJediAiAction CJediAiActionEngageTrandoshanConcussive::getType() const {
	return eJediAiAction_EngageTrandoshanConcussive;
}

void CJediAiActionEngageTrandoshanConcussive::reset() {

	// base class version
	BASECLASS::reset();

	// debounce my actions
	selectorParams.debounceActions = true;

	// setup saber attack
	saberAttack.name = "Saber Attack";
	{
		// setup move
		saberAttackActions.move.params.destination = eJediAiDestination_Victim;
		saberAttackActions.move.params.minWalkRunDistance = kJediMeleeCorrectionTweak;
		saberAttackActions.move.params.minDashDistance = kJediAiDashActivationDistanceMin;
		saberAttackActions.move.params.facePct = 0.8f;

		// setup saber selector
		saberAttackActions.saberSelector.name = "Saber Selector";
		{
			// setup high skill saber
			saberAttackActions.saberSelectorActions.highSkillSaber.name = "High Skill Saber";
			saberAttackActions.saberSelectorActions.highSkillSaber.params.numSwings = 3;

			// setup low skill saber
			saberAttackActions.saberSelectorActions.lowSkillSaberFakeSim.name = "Low Skill Saber (Fake Sim)";
			saberAttackActions.saberSelectorActions.lowSkillSaberFakeSim.params.breakVictimShield = true;
			saberAttackActions.saberSelectorActions.lowSkillSaberFakeSim.params.minResult = eJediAiActionSimResult_Beneficial;
			saberAttackActions.saberSelectorActions.lowSkillSaberFakeSim.params.postSim = true;
			saberAttackActions.saberSelectorActions.lowSkillSaber.name = "Low Skill Saber";
			saberAttackActions.saberSelectorActions.lowSkillSaber.params.numSwings = 3;
		}
	}

	// setup jump forward attack
	jumpForwardAttack.name = "Jump Forward Attack";
	jumpForwardAttack.params.activationDistance = kJediCombatMaxJumpDistance;
	jumpForwardAttack.params.attack = eJediAiJumpForwardAttack_Horizontal;

	// setup special attack
	specialAttack.name = "Special Attack";
	specialAttack.selectorParams.debounceActions = true;
	{
		// setup kick-saber combo
		specialAttackActions.kickSaberCombo.name = "Kick-Saber Combo";
		specialAttackActions.kickSaberCombo.minRunFrequency = 6.0f;
		{
			// setup move
			specialAttackActions.kickSaberComboActions.move.params.destination = eJediAiDestination_Victim;
			specialAttackActions.kickSaberComboActions.move.params.minWalkRunDistance = kJediMeleeCorrectionTweak;
			specialAttackActions.kickSaberComboActions.move.params.minDashDistance = kJediAiDashActivationDistanceMin;
			specialAttackActions.kickSaberComboActions.move.params.facePct = 0.8f;
		
			// setup swing saber
			specialAttackActions.kickSaberComboActions.saber.params.numSwings = 2;
		}

		// setup force push
		specialAttackActions.forcePush.params.chargeDuration = 1.0f;
		specialAttackActions.forcePush.minRunFrequency = 10.0f;

		// setup throw object
		specialAttackActions.throwObject.name = "Throw Object At Victim";
		specialAttackActions.throwObject.params.gripDuration = 1.0f;
		specialAttackActions.throwObject.params.gripTarget = eJediAiForceTkTarget_Object;
		specialAttackActions.throwObject.params.throwTarget = eJediAiForceTkTarget_Victim;

		// setup jump over attack
		specialAttackActions.jumpOverAttack.name = "JumpOver Attack";
		specialAttackActions.jumpOverAttack.params.attack = true;
		specialAttackActions.jumpOverAttack.minRunFrequency = 5.0f;
	}

	// setup force tk attack
	threatConstraint.skipWhileInProgress = true;
	threatConstraint.params.addThreatReaction(eJediThreatType_Explosion, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
	forceTkAttack.name = "ForceTk Attack";
	forceTkAttack.minRunFrequency = 10.0f;
	{
		// setup move
		forceTkAttackActions.move.name = "Move To ForceTk Range";
		forceTkAttackActions.move.params.destination = eJediAiDestination_Victim;
		forceTkAttackActions.move.params.minWalkRunDistance = kJediForceSelectRange;
		forceTkAttackActions.move.params.minDashDistance = kJediForceSelectRange;
		forceTkAttackActions.move.params.facePct = 0.8f;

		// setup grip
		forceTkAttackActions.grip.name = "Grip";
		forceTkAttackActions.grip.selectorParams.debounceActions = true;
		{
			// setup bailOnThreat
			forceTkAttackActions.gripActions.bailOnThreat.name = "Bail On Threat";
			forceTkAttackActions.gripActions.bailOnThreat.params.gripDuration = 2.5f;
			forceTkAttackActions.gripActions.bailOnThreat.params.gripTarget = eJediAiForceTkTarget_Victim;
			forceTkAttackActions.gripActions.bailOnThreat.params.throwTarget = eJediAiForceTkTarget_Recommended;

			// setup ignoreThreat
			forceTkAttackActions.gripActions.ignoreThreat.name = "Ignore Threat";
			forceTkAttackActions.gripActions.ignoreThreat.params.clearVictimThreats[eJediThreatType_Explosion] = true;

			// setup takeDamage
			forceTkAttackActions.gripActions.takeDamage.name = "Take Damage";
			forceTkAttackActions.gripActions.takeDamage.params.gripDuration = 6.0f;
			forceTkAttackActions.gripActions.takeDamage.params.gripTarget = eJediAiForceTkTarget_Victim;
			forceTkAttackActions.gripActions.takeDamage.params.throwTarget = eJediAiForceTkTarget_Recommended;
		}
	}
}

CJediAiAction **CJediAiActionEngageTrandoshanConcussive::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}

float *CJediAiActionEngageTrandoshanConcussive::getActionOddsTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionOddsTable;
}


/////////////////////////////////////////////////////////////////////////////
//
// engage trandoshan flutterpack
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionEngageTrandoshanFlutterpack::CJediAiActionEngageTrandoshanFlutterpack() {

	// setup engage flying actions
	engageFlying.constraint = &engageFlyingConstraint;
	engageFlying.setAction(0, &engageFlyingActions.moveCloser, 1.0f);
	engageFlying.setAction(1, &engageFlyingActions.forceTkKill, 1.0f);
	engageFlying.setAction(2, &engageFlyingActions.throwObject, 1.0f);
	engageFlying.setAction(3, &engageFlyingActions.forceTkFail, 1.0f);
	compileTimeAssert(engageFlying.eAction_Count == 4);
	{
		// setup forceTk kill
		engageFlyingActions.forceTkKill.constraint = &aboveDesiredKillTimerConstraint;

		// setup forceTk fail
		engageFlyingActions.forceTkFail.constraint = &belowDesiredKillTimerLowSkillLevelConstraint;
	}

	// setup engage ground actions
	engageGround.constraint = &engageGroundConstraint;
	engageGround.actionTable[0] = &engageGroundActions.waitForThreat;
	engageGround.actionTable[1] = &engageGroundActions.counter;
	compileTimeAssert(engageGround.eAction_Count == 2);
	{
		// setup counter
		engageGroundActions.counter.setAction(0, &engageGroundActions.counterActions.kickCounter, 1.0f);
		engageGroundActions.counter.setAction(1, &engageGroundActions.counterActions.forcePushCounter, 1.0f);
		engageGroundActions.counter.setAction(2, &engageGroundActions.counterActions.dodgeCounter, 1.0f);
		compileTimeAssert(engageGroundActions.counter.eAction_Count == 3);
		{
			// setup counter - kick
			engageGroundActions.counterActions.kickCounter.actionTable[0] = &engageGroundActions.counterActions.kickCounterActions.wait;
			engageGroundActions.counterActions.kickCounter.actionTable[1] = &engageGroundActions.counterActions.kickCounterActions.kick;
			compileTimeAssert(engageGroundActions.counterActions.kickCounter.eAction_Count == 2);

			// setup counter - force push
			engageGroundActions.counterActions.forcePushCounter.actionTable[0] = &engageGroundActions.counterActions.forcePushCounterActions.wait;
			engageGroundActions.counterActions.forcePushCounter.actionTable[1] = &engageGroundActions.counterActions.forcePushCounterActions.forcePush;
			compileTimeAssert(engageGroundActions.counterActions.forcePushCounter.eAction_Count == 2);

			// setup counter - dodge
			engageGroundActions.counterActions.dodgeCounter.actionTable[0] = &engageGroundActions.counterActions.dodgeCounterActions.wait;
			engageGroundActions.counterActions.dodgeCounter.actionTable[1] = &engageGroundActions.counterActions.dodgeCounterActions.dodge;
			compileTimeAssert(engageGroundActions.counterActions.dodgeCounter.eAction_Count == 2);
		}
	}

	// build my action table
	// if you hit the compileTimeAssert below, you've likely added an
	// action and need to set it up here
	memset(actionTable, 0, sizeof(actionTable));
	actionTable[eAction_EngageFlying] = &engageFlying;
	actionTable[eAction_EngageGround] = &engageGround;
	compileTimeAssert(eAction_Count == 2);

	// all actions are equally likely
	memset(actionOddsTable, 0, sizeof(actionOddsTable));
	for (int i = 0; i < eAction_Count; ++i) {
		actionOddsTable[i] = 1.0f;
	}

	// reset my data
	reset();
}

EJediAiAction CJediAiActionEngageTrandoshanFlutterpack::getType() const {
	return eJediAiAction_EngageTrandoshanFlutterpack;
}

void CJediAiActionEngageTrandoshanFlutterpack::reset() {

	// base class version
	BASECLASS::reset();

	// debounce my actions
	selectorParams.debounceActions = false;

	// setup engage flying
	engageFlyingConstraint.params.setAllCombatTypesDisallowed();
	engageFlyingConstraint.params.setCombatTypeDisallowed(eJediCombatType_AirUnit);
	engageFlying.name = "Engage Flying";
	{
		// setup move closer
		engageFlyingActions.moveCloser.name = "Move Closer";
		engageFlyingActions.moveCloser.params.destination = eJediAiDestination_Victim;
		engageFlyingActions.moveCloser.params.facePct = 0.08f;
		engageFlyingActions.moveCloser.params.activationDistance = 50.0f;
		engageFlyingActions.moveCloser.params.minWalkRunDistance = engageFlyingActions.moveCloser.params.activationDistance;
		engageFlyingActions.moveCloser.params.minDashDistance = engageFlyingActions.moveCloser.params.minWalkRunDistance;
		engageFlyingActions.moveCloser.params.dashActivationDistance = engageFlyingActions.moveCloser.params.minDashDistance + 30.0f;
		engageFlyingActions.moveCloser.params.isRelevant = true;
		engageFlyingActions.moveCloser.params.failIfTooClose = true;

		// force tk kill
		engageFlyingActions.forceTkKill.name = "Force Tk";
		engageFlyingActions.forceTkKill.params.gripDuration = 1.0f;
		engageFlyingActions.forceTkKill.params.gripTarget = eJediAiForceTkTarget_Victim;
		engageFlyingActions.forceTkKill.params.throwTarget = eJediAiForceTkTarget_Recommended;
		engageFlyingActions.forceTkKill.params.failIfNotThrowable = true;
		engageFlyingActions.forceTkKill.params.iThrowVelocity.set(0.0f, -50.0f, 0.0f);

		// throw object
		engageFlyingActions.throwObject.name = "Throw Object at Victim";
		engageFlyingActions.throwObject.params.gripDuration = 0.5f;
		engageFlyingActions.throwObject.params.gripTarget = eJediAiForceTkTarget_Object;
		engageFlyingActions.throwObject.params.throwTarget = eJediAiForceTkTarget_Victim;

		// force tk fail
		engageFlyingActions.forceTkFail.name = "Force Tk Fail";
		engageFlyingActions.forceTkFail.params.gripDuration = 1.0f;
		engageFlyingActions.forceTkFail.params.gripTarget = eJediAiForceTkTarget_Victim;
		engageFlyingActions.forceTkFail.params.throwTarget = eJediAiForceTkTarget_Count; // none
		engageFlyingActions.forceTkFail.params.failIfNotThrowable = true;
	}

	// setup engage ground
	engageGroundConstraint.params.setAllCombatTypesAllowed();
	engageGroundConstraint.params.setCombatTypeDisallowed(eJediCombatType_AirUnit);
	engageGround.name = "Engage Ground";
	{
		// setup wait for threat
		engageGroundActions.waitForThreat.params.duration = 10.0f;
		engageGroundActions.waitForThreat.params.threatParamTable[eJediThreatType_Rush].set(9999.0f, 9999.0f);
		engageGroundActions.waitForThreat.params.threatParamTable[eJediThreatType_Explosion].set(9999.0f, 9999.0f);

		// setup counter
		engageGroundActions.counter.name = "Counter";
		{
			// setup kick counter
			engageGroundActions.counterActions.kickCounter.name = "Kick Counter";
			{
				engageGroundActions.counterActions.kickCounterActions.wait.params.duration = 10.0f;
				engageGroundActions.counterActions.kickCounterActions.wait.params.threatParamTable[eJediThreatType_Rush].set(0.75f, kJediKickDistance);
				engageGroundActions.counterActions.kickCounterActions.wait.params.threatParamTable[eJediThreatType_Explosion].set(0.75f, kJediKickDistance);
				engageGroundActions.counterActions.kickCounterActions.kick.params.allowDisplacement = false;
			}

			// setup force push counter
			engageGroundActions.counterActions.forcePushCounter.name = "Force Push Counter";
			{
				engageGroundActions.counterActions.forcePushCounterActions.wait.params.duration = 10.0f;
				engageGroundActions.counterActions.forcePushCounterActions.wait.params.threatParamTable[eJediThreatType_Rush].set(kJediForcePushEnterDuration * 0.3f, kJediForcePushTier2Min * 0.666f);
				engageGroundActions.counterActions.forcePushCounterActions.wait.params.threatParamTable[eJediThreatType_Explosion].set(kJediForcePushEnterDuration * 0.3f, kJediForcePushTier2Min * 0.666f);
				engageGroundActions.counterActions.forcePushCounterActions.forcePush.params.chargeDuration = 0.0f;
				engageGroundActions.counterActions.forcePushCounterActions.forcePush.params.skipCharge = true;
				engageGroundActions.counterActions.forcePushCounterActions.forcePush.params.maxVictimDistance = 0.0f;
			}

			// setup dodge counter
			engageGroundActions.counterActions.dodgeCounter.name = "Dodge Counter";
			{
				engageGroundActions.counterActions.dodgeCounterActions.wait.params.duration = 10.0f;
				engageGroundActions.counterActions.dodgeCounterActions.wait.params.threatParamTable[eJediThreatType_Rush].set(0.75f, kJediSwingSaberDistance);
				engageGroundActions.counterActions.dodgeCounterActions.wait.params.threatParamTable[eJediThreatType_Explosion].set(0.75f, kJediSwingSaberDistance);
				engageGroundActions.counterActions.dodgeCounterActions.dodge.params.attack = false;
			}
		}
	}
}

CJediAiAction **CJediAiActionEngageTrandoshanFlutterpack::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}

float *CJediAiActionEngageTrandoshanFlutterpack::getActionOddsTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionOddsTable;
}


/////////////////////////////////////////////////////////////////////////////
//
// engage b1 battle droid
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionEngageB1BattleDroid::CJediAiActionEngageB1BattleDroid() {

	// setup saber attack
	saberAttack.actionTable[0] = &saberAttackActions.move;
	saberAttack.actionTable[1] = &saberAttackActions.saber;
	compileTimeAssert(saberAttack.eAction_Count == 2);

	// setup special attack
	specialAttack.setAction(0, &specialAttackActions.dashAttack, 0.6f);
	specialAttack.setAction(1, &specialAttackActions.jumpAttack, 0.6f);
	specialAttack.setAction(2, &specialAttackActions.throwObject, 0.4f);
	specialAttack.setAction(3, &specialAttackActions.pushGrenade, 0.8f);
	specialAttack.setAction(4, &specialAttackActions.throwGrenade, 0.8f);
	compileTimeAssert(specialAttack.eAction_Count == 5);
	{
		// setup dash attack
		specialAttackActions.dashAttack.constraint = &lowSkillLevelConstraint;

		// setup jump attack
		specialAttackActions.jumpAttack.constraint = &highSkillLevelConstraint;

		// setup pushGrenade
		specialAttackActions.pushGrenade.constraint = &specialAttackActions.onlyIfGrenadeThreat;
		specialAttackActions.onlyIfGrenadeThreat.nextConstraint = &highSkillLevelConstraint;
	}

	// build my action table
	// if you hit the compileTimeAssert below, you've likely added an
	// action and need to set it up here
	memset(actionTable, 0, sizeof(actionTable));
	actionTable[eAction_ForceTkAttack] = &forceTk;
	actionTable[eAction_ForcePushAttack] = &forcePush;
	actionTable[eAction_DeflectAttack] = &deflectAttack;
	actionTable[eAction_SaberAttack] = &saberAttack;
	actionTable[eAction_KickAttack] = &kick;
	actionTable[eAction_SpecialAttack] = &specialAttack;
	compileTimeAssert(eAction_Count == 6);

	// every action is equally viable
	for (int i = 0; i < eAction_Count; ++i) {
		actionOddsTable[i] = 1.0f;
	}

	// reset my data
	reset();
}

EJediAiAction CJediAiActionEngageB1BattleDroid::getType() const {
	return eJediAiAction_EngageB1BattleDroid;
}

void CJediAiActionEngageB1BattleDroid::reset() {

	// base class version
	BASECLASS::reset();

	// debounce my actions
	selectorParams.debounceActions = true;

	// setup force tk attack
	forceTk.name = "Force Tk Victim";
	forceTk.params.gripDuration = 1.0f;
	forceTk.params.gripTarget = eJediAiForceTkTarget_Victim;
	forceTk.params.throwTarget = eJediAiForceTkTarget_Recommended;

	// setup force push
	forcePush.params.chargeDuration = 0.0f;

	// setup saber attack
	saberAttack.name = "Saber Attack";
	{
		saberAttackActions.move.params.destination = eJediAiDestination_Victim;
		saberAttackActions.move.params.minWalkRunDistance = kJediMeleeCorrectionTweak;
		saberAttackActions.move.params.minDashDistance = kJediAiDashActivationDistanceMin;
		saberAttackActions.move.params.facePct = 0.8f;
		saberAttackActions.saber.params.numSwings = 2;
	}

	// setup special attacks
	specialAttack.name = "Special Attack";
	specialAttack.selectorParams.debounceActions = true;
	{
		// setup dash attack
		specialAttackActions.dashAttack.name = "Dash Attack";
		specialAttackActions.dashAttack.params.attack = true;
		specialAttackActions.dashAttack.params.destination = eJediAiDestination_Victim;
		specialAttackActions.dashAttack.params.activationDistance = kJediAiDashActivationDistanceMin;
		specialAttackActions.dashAttack.params.distance = kJediSwingSaberDistance * 0.5f;

		// setup jump attack
		specialAttackActions.jumpAttack.name = "Jump Attack";
		specialAttackActions.jumpAttack.params.activationDistance = kJediCombatMaxJumpDistance;
		specialAttackActions.jumpAttack.params.attack = eJediAiJumpForwardAttack_Horizontal;

		// setup throw object
		specialAttackActions.throwObject.name = "Throw Object At Victim";
		specialAttackActions.throwObject.params.gripDuration = 1.0f;
		specialAttackActions.throwObject.params.gripTarget = eJediAiForceTkTarget_Object;
		specialAttackActions.throwObject.params.throwTarget = eJediAiForceTkTarget_Victim;

		// setup special push grenade action
		specialAttackActions.onlyIfGrenadeThreat.params.addThreatReaction(eJediThreatType_Grenade, CJediAiActionConstraintThreat::eThreatReaction_FailIfNone);
		specialAttackActions.pushGrenade.name = "Force Push Grenade";
		specialAttackActions.pushGrenade.params.chargeDuration = 0.0f;
		specialAttackActions.pushGrenade.params.mustHitGrenade = true;

		// setup special throw grenade action
		specialAttackActions.throwGrenade.name = "ForceTk Grenade";
		specialAttackActions.throwGrenade.params.gripDuration = 0.85f;
		specialAttackActions.throwGrenade.params.gripTarget = eJediAiForceTkTarget_Grenade;
		specialAttackActions.throwGrenade.params.throwTarget = eJediAiForceTkTarget_Victim;
	}
}

CJediAiAction **CJediAiActionEngageB1BattleDroid::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}

float *CJediAiActionEngageB1BattleDroid::getActionOddsTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionOddsTable;
}


/////////////////////////////////////////////////////////////////////////////
//
// engage b1 melee droid
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionEngageB1MeleeDroid::CJediAiActionEngageB1MeleeDroid() {

	// setup special attack
	specialAttack.setAction(0, &specialAttackActions.kickSaberCombo, 0.75f);
	specialAttack.setAction(1, &specialAttackActions.jumpAttack, 0.5f);
	specialAttack.setAction(2, &specialAttackActions.kickForcePushCombo, 0.25f);
	specialAttack.setAction(3, &specialAttackActions.kickForceTkCombo, 0.25f);
	compileTimeAssert(specialAttack.eAction_Count == 4);
	{
		// setup kick - saber combo
		specialAttackActions.kickSaberCombo.actionTable[0] = &specialAttackActions.kickSaberActions.kick;
		specialAttackActions.kickSaberCombo.actionTable[1] = &specialAttackActions.kickSaberActions.swingSaber;
		compileTimeAssert(specialAttackActions.kickSaberCombo.eAction_Count == 2);

		// setup jump attack
		specialAttackActions.jumpAttack.constraint = &highSkillLevelConstraint;

		// setup kick - force push combo
		specialAttackActions.kickForcePushCombo.constraint = &highSkillLevelConstraint;
		specialAttackActions.kickForcePushCombo.actionTable[0] = &specialAttackActions.kickForcePushActions.kick;
		specialAttackActions.kickForcePushCombo.actionTable[1] = &specialAttackActions.kickForcePushActions.forcePush;
		compileTimeAssert(specialAttackActions.kickForcePushCombo.eAction_Count == 2);

		// setup kick - forceTk combo
		specialAttackActions.kickForceTkCombo.constraint = &highSkillLevelConstraint;
		specialAttackActions.kickForceTkCombo.actionTable[0] = &specialAttackActions.kickForceTkActions.kick;
		specialAttackActions.kickForceTkCombo.actionTable[1] = &specialAttackActions.kickForceTkActions.forceTk;
		compileTimeAssert(specialAttackActions.kickForceTkCombo.eAction_Count == 2);
	}

	// setup ambient actions
	ambient.setAction(0, &ambientActions.swingSaberOnceFakeSim, 1.0f);
	ambient.setAction(1, &ambientActions.forceTkIgnoreThreatFakeSim, 1.0f);
	ambient.setAction(2, &ambientActions.forceTkDodgeThreatFakeSim, 1.0f);
	compileTimeAssert(ambient.eAction_Count == 3);
	{
		// setup swingSaberOnce
		ambientActions.swingSaberOnceFakeSim.decoratedAction = &ambientActions.swingSaberOnce;

		// setup force tk (ignore threat)
		ambientActions.forceTkIgnoreThreatFakeSim.decoratedAction = &ambientActions.forceTkIgnoreThreat;
		ambientActions.forceTkIgnoreThreat.constraint = &ambientActions.forceTkIgnoreThreatConstraint;
		ambientActions.forceTkIgnoreThreatConstraint.nextConstraint = &lowSkillLevelConstraint;

		// setup force tk (dodge threat)
		ambientActions.forceTkDodgeThreatFakeSim.decoratedAction = &ambientActions.forceTkDodgeThreat;
		ambientActions.forceTkDodgeThreatFakeSim.constraint = &ambientActions.forceTkDodgeThreatConstraint;
		ambientActions.forceTkDodgeThreatConstraint.nextConstraint = &highSkillLevelConstraint;
	}

	// build my action table
	// if you hit the compileTimeAssert below, you've likely added an
	// action and need to set it up here
	memset(actionTable, 0, sizeof(actionTable));
	actionTable[eAction_CounterAttack] = &counterAttack;
	actionTable[eAction_SpecialAttack] = &specialAttack;
	actionTable[eAction_AmbientActions] = &ambient;
	compileTimeAssert(eAction_Count == 3);

	// build my odds table
	memset(actionOddsTable, 0, sizeof(actionOddsTable));
	actionOddsTable[eAction_CounterAttack] = 0.5f;
	actionOddsTable[eAction_SpecialAttack] = 0.25f;
	actionOddsTable[eAction_AmbientActions] = 0.25f;
	compileTimeAssert(eAction_Count == 3);

	// reset my data
	reset();
}

EJediAiAction CJediAiActionEngageB1MeleeDroid::getType() const {
	return eJediAiAction_EngageB1MeleeDroid;
}

void CJediAiActionEngageB1MeleeDroid::reset() {

	// base class version
	BASECLASS::reset();

	// debounce my actions
	selectorParams.debounceActions = true;

	// setup special attack
	specialAttack.name = "Special Attack";
	specialAttack.selectorParams.debounceActions = true;
	{
		// setup kick - saber combo
		specialAttackActions.kickSaberCombo.name = "Kick - Saber Combo";
		specialAttackActions.kickSaberActions.swingSaber.params.numSwings = 1;

		// setup jump attack
		specialAttackActions.jumpAttack.name = "Jump Attack";
		specialAttackActions.jumpAttack.params.activationDistance = kJediCombatMaxJumpDistance;
		specialAttackActions.jumpAttack.params.attack = eJediAiJumpForwardAttack_Vertical;

		// setup kick - force push combo
		specialAttackActions.kickForcePushCombo.name = "Kick - Force Push Combo";
		specialAttackActions.kickForcePushCombo.params.minFailureResult = eJediAiActionSimResult_Hurtful;
		specialAttackActions.kickForcePushActions.forcePush.params.chargeDuration = 0.0f;
		specialAttackActions.kickForcePushActions.forcePush.params.maxVictimDistance = kJediForcePushTier2Min;

		// setup kick - forceTk combo
		specialAttackActions.kickForceTkCombo.name = "Kick - ForceTk Combo";
		specialAttackActions.kickForceTkCombo.params.minFailureResult = eJediAiActionSimResult_Hurtful;
		specialAttackActions.kickForceTkActions.forceTk.params.gripDuration = 0.5f;
		specialAttackActions.kickForceTkActions.forceTk.params.gripTarget = eJediAiForceTkTarget_Victim;
		specialAttackActions.kickForceTkActions.forceTk.params.throwTarget = eJediAiForceTkTarget_Recommended;
		specialAttackActions.kickForceTkActions.forceTk.params.failIfNotThrowable = true;
		specialAttackActions.kickForceTkActions.forceTk.params.minActivationDistance = 0.0f;
		specialAttackActions.kickForceTkActions.forceTk.params.skipEnter = true;
	}

	// setup ambient actions
	ambient.name = "Ambient Actions";
	ambient.selectorParams.debounceActions = true;
	{
		// setup swingSaberOnce
		ambientActions.swingSaberOnceFakeSim.name = "Swing Saber Once Fake Sim";
		ambientActions.swingSaberOnceFakeSim.params.makeVictimStumble = true;
		ambientActions.swingSaberOnce.name = "Swing Saber Once";
		ambientActions.swingSaberOnce.params.numSwings = 1;

		// setup force tk (ignore threat)
		ambientActions.forceTkIgnoreThreatFakeSim.name = "Force Tk (Ignore Threat) Fake Sim";
		ambientActions.forceTkIgnoreThreatFakeSim.params.minResult = eJediAiActionSimResult_Beneficial;
		ambientActions.forceTkIgnoreThreatFakeSim.params.clearVictimThreats[eJediThreatType_Melee] = true;
		ambientActions.forceTkIgnoreThreatFakeSim.params.clearVictimThreats[eJediThreatType_Rocket] = true;
		ambientActions.forceTkIgnoreThreat.name = "Force Tk (Ignore Threat)";
		ambientActions.forceTkIgnoreThreat.params.gripDuration = 6.0f;
		ambientActions.forceTkIgnoreThreat.params.gripTarget = eJediAiForceTkTarget_Victim;
		ambientActions.forceTkIgnoreThreat.params.throwTarget = eJediAiForceTkTarget_Recommended;
		ambientActions.forceTkIgnoreThreatConstraint.params.addThreatReaction(eJediThreatType_Melee, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
		ambientActions.forceTkIgnoreThreatConstraint.params.addThreatReaction(eJediThreatType_Rocket, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
		ambientActions.forceTkIgnoreThreatConstraint.skipWhileInProgress = true;

		// setup force tk (dodge threat)
		ambientActions.forceTkDodgeThreatFakeSim.name = "Force Tk (Dodge Threat) Fake Sim";
		ambientActions.forceTkDodgeThreatFakeSim.params.minResult = eJediAiActionSimResult_Beneficial;
		ambientActions.forceTkDodgeThreat.name = "Force Tk (Dodge Threat)";
		ambientActions.forceTkDodgeThreat.params.gripDuration = 6.0f;
		ambientActions.forceTkDodgeThreat.params.gripTarget = eJediAiForceTkTarget_Victim;
		ambientActions.forceTkDodgeThreat.params.throwTarget = eJediAiForceTkTarget_Recommended;
		ambientActions.forceTkDodgeThreatConstraint.params.addThreatReaction(eJediThreatType_Melee, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
		ambientActions.forceTkDodgeThreatConstraint.params.addThreatReaction(eJediThreatType_Rocket, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
	}
}

CJediAiAction **CJediAiActionEngageB1MeleeDroid::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}

float *CJediAiActionEngageB1MeleeDroid::getActionOddsTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionOddsTable;
}


/////////////////////////////////////////////////////////////////////////////
//
// engage b1 jetpack droid
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionEngageB1JetpackDroid::CJediAiActionEngageB1JetpackDroid() {

	// setup ambient actions
	ambient.constraint = &belowDesiredKillTimerConstraint;
	ambient.setAction(0, &ambientActions.dashToCloseRangeFakeSim, 1.0f);
	ambient.setAction(1, &ambientActions.forceTkFail, 1.0f);
	compileTimeAssert(ambient.eAction_Count == 2);
	{
		// setup 'dash to close range'
		ambientActions.dashToCloseRangeFakeSim.decoratedAction = &ambientActions.dashToCloseRange;
	}

	// setup forceTkKill
	forceTkKill.constraint = &aboveDesiredKillTimerConstraint;

	// build my action table
	// if you hit the compileTimeAssert below, you've likely added an
	// action and need to set it up here
	memset(actionTable, 0, sizeof(actionTable));
	actionTable[eAction_ForceTkAttack] = &forceTkKill;
	actionTable[eAction_ForcePushAttack] = &forcePush;
	actionTable[eAction_DeflectAttack] = &deflectAttack;
	actionTable[eAction_ThrowObject] = &throwObject;
	actionTable[eAction_Ambient] = &ambient;
	compileTimeAssert(eAction_Count == 5);

	// every action is equally viable
	for (int i = 0; i < eAction_Count; ++i) {
		actionOddsTable[i] = 1.0f;
	}

	// reset my data
	reset();
}

EJediAiAction CJediAiActionEngageB1JetpackDroid::getType() const {
	return eJediAiAction_EngageB1JetpackDroid;
}

void CJediAiActionEngageB1JetpackDroid::reset() {

	// base class version
	BASECLASS::reset();

	// debounce my actions
	selectorParams.debounceActions = true;

	// setup force tk kill
	forceTkKill.name = "ForceTk Kill";
	forceTkKill.params.gripDuration = 0.5f;
	forceTkKill.params.gripTarget = eJediAiForceTkTarget_Victim;
	forceTkKill.params.throwTarget = eJediAiForceTkTarget_Recommended;
	forceTkKill.params.iThrowVelocity.set(0.0f, -50.0f, 0.0f);

	// setup force push
	forcePush.params.chargeDuration = 0.0f;

	// setup throw object
	throwObject.name = "Throw Object At Victim";
	throwObject.params.gripDuration = 0.5f;
	throwObject.params.gripTarget = eJediAiForceTkTarget_Object;
	throwObject.params.throwTarget = eJediAiForceTkTarget_Victim;

	// setup ambient
	ambient.name = "Ambient Actions";
	ambient.selectorParams.debounceActions = true;
	{
		// setup dash to close range
		ambientActions.dashToCloseRangeFakeSim.name = "Dash To Close Range Fake Sim";
		ambientActions.dashToCloseRangeFakeSim.params.minResult = eJediAiActionSimResult_Beneficial;
		ambientActions.dashToCloseRange.name = "Dash To Close Range";
		ambientActions.dashToCloseRange.params.attack = false;
		ambientActions.dashToCloseRange.params.ignoreMinDistance = true;
		ambientActions.dashToCloseRange.params.destination = eJediAiDestination_Victim;
		ambientActions.dashToCloseRange.params.distance = 5.0f;
		ambientActions.dashToCloseRange.params.activationDistance = 15.0f;
		ambientActions.dashToCloseRange.minRunFrequency = 5.0f;

		// setup forceTk fail
		ambientActions.forceTkFail.name = "ForceTk Fail";
		ambientActions.forceTkFail.params.gripDuration = 0.5f;
		ambientActions.forceTkFail.params.gripTarget = eJediAiForceTkTarget_Victim;
		ambientActions.forceTkFail.params.throwTarget = eJediAiForceTkTarget_Count; // none
	}
}

CJediAiAction **CJediAiActionEngageB1JetpackDroid::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}

float *CJediAiActionEngageB1JetpackDroid::getActionOddsTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionOddsTable;
}


/////////////////////////////////////////////////////////////////////////////
//
// engage b2 battle droid
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionEngageB2BattleDroid::CJediAiActionEngageB2BattleDroid() {

	// setup force push rocket
	forcePushRocket.constraint = &forcePushRocketThreatConstraint;
	forcePushRocket.actionTable[0] = &forcePushRocketActions.waitForThreat;
	forcePushRocket.actionTable[1] = &forcePushRocketActions.forcePush;
	compileTimeAssert(forcePushRocket.eAction_Count == 2);

	// setup saber attack
	saberAttack.constraint = &lowSkillLevelConstraint;
	saberAttack.actionTable[0] = &saberAttackActions.move;
	saberAttack.actionTable[1] = &saberAttackActions.saber;
	compileTimeAssert(saberAttack.eAction_Count == 2);

	// setup force push + saber combo
	forcePushSaberCombo.constraint = &highSkillLevelConstraint;
	forcePushSaberCombo.actionTable[0] = &forcePushSaberComboActions.move;
	forcePushSaberCombo.actionTable[1] = &forcePushSaberComboActions.forcePush;
	forcePushSaberCombo.actionTable[2] = &forcePushSaberComboActions.saber;
	compileTimeAssert(forcePushSaberCombo.eAction_Count == 3);

	// setup saber during rush
	saberDuringRush.constraint = &saberDuringRushThreatConstraint;
	saberDuringRushThreatConstraint.nextConstraint = &belowDesiredKillTimerLowSkillLevelConstraint;
	saberDuringRush.actionTable[0] = &saberDuringRushActions.waitForThreat;
	saberDuringRush.actionTable[1] = &saberDuringRushActions.fakeSim;
	saberDuringRushActions.fakeSim.decoratedAction = &saberDuringRushActions.swingSaber;
	compileTimeAssert(saberDuringRush.eAction_Count == 2);

	// setup crouch attack
	crouchAttack.constraint = &highSkillLevelConstraint;

	// setup special attack
	specialAttack.setAction(0, &specialAttackActions.dashAttack, 0.6f);
	specialAttack.setAction(1, &specialAttackActions.jumpForwardAttack, 0.4f);
	specialAttack.setAction(2, &specialAttackActions.jumpOverAttack, 0.4f);
	specialAttack.setAction(3, &specialAttackActions.throwObject, 0.2f);
	compileTimeAssert(specialAttack.eAction_Count == 4);
	{
		// setup dash attack
		specialAttackActions.dashAttack.constraint = &highSkillLevelConstraint;

		// setup jump forward attack
		specialAttackActions.jumpForwardAttack.constraint = &highSkillLevelConstraint;

		// setup jump over attack
		specialAttackActions.jumpOverAttack.constraint = &highSkillLevelConstraint;

		// setup throwObject
		specialAttackActions.throwObject.constraint = &specialAttackActions.throwObjectThreatConstraint;
	}

	// setup ambient actions
	ambient.setAction(0, &ambientActions.forceTkFakeSimResult, 1.0f);
	ambient.setAction(1, &ambientActions.jumpOverRush, 1.0f);
	ambient.setAction(2, &ambientActions.jumpOverRocket, 1.0f);
	compileTimeAssert(ambient.eAction_Count == 3);
	{
		// setup force tk
		ambientActions.forceTkFakeSimResult.decoratedAction = &ambientActions.forceTkFakeSimThreats;
		ambientActions.forceTkFakeSimResult.constraint = &ambientActions.forceTkThreatConstraint;
		ambientActions.forceTkThreatConstraint.nextConstraint = &belowDesiredKillTimerConstraint;
		ambientActions.forceTkFakeSimThreats.decoratedAction = &ambientActions.forceTk;
		ambientActions.forceTkFakeSimThreats.modifySimConstraint = &padawanSkillLevelConstraint;

		// setup jump over rush
		ambientActions.jumpOverRush.constraint = &ambientActions.jumpOverRushThreatConstraint;
		ambientActions.jumpOverRushThreatConstraint.nextConstraint = &highSkillLevelConstraint;
		ambientActions.jumpOverRush.actionTable[0] = &ambientActions.jumpOverRushActions.waitForThreat;
		ambientActions.jumpOverRush.actionTable[1] = &ambientActions.jumpOverRushActions.jumpOver;
		compileTimeAssert(ambientActions.jumpOverRush.eAction_Count == 2);

		// setup jump over rocket
		ambientActions.jumpOverRocket.constraint = &ambientActions.jumpOverRocketConstraint;
		ambientActions.jumpOverRocketConstraint.nextConstraint = &highSkillLevelConstraint;
	}

	// build my action table
	// if you hit the compileTimeAssert below, you've likely added an
	// action and need to set it up here
	memset(actionTable, 0, sizeof(actionTable));
	actionTable[eAction_DeflectAttack] = &deflectAttack;
	actionTable[eAction_ForcePushRocket] = &forcePushRocket;
	actionTable[eAction_SaberAttack] = &saberAttack;
	actionTable[eAction_ForcePushSaber] = &forcePushSaberCombo;
	actionTable[eAction_SaberDuringRush] = &saberDuringRush;
	actionTable[eAction_CrouchAttack] = &crouchAttack;
	actionTable[eAction_SpecialAttack] = &specialAttack;
	actionTable[eAction_Ambient] = &ambient;
	compileTimeAssert(eAction_Count == 8);

	// every action is equally viable
	for (int i = 0; i < eAction_Count; ++i) {
		actionOddsTable[i] = 1.0f;
	}

	// reset my data
	reset();
}

EJediAiAction CJediAiActionEngageB2BattleDroid::getType() const {
	return eJediAiAction_EngageB2BattleDroid;
}

void CJediAiActionEngageB2BattleDroid::reset() {

	// base class version
	BASECLASS::reset();

	// debounce my actions
	selectorParams.debounceActions = true;
	selectorParams.selectFrequency = 0.0f;

	// setup deflect counter attack
	deflectAttack.name = "Deflect at Victim";
	deflectAttack.params.deflectAtEnemies = true;

	// setup force push rocket
	forcePushRocket.name = "Force Push Rocket";
	forcePushRocketThreatConstraint.params.addThreatReaction(eJediThreatType_Rocket, CJediAiActionConstraintThreat::eThreatReaction_FailIfNone);
	{
		// setup wait for threat
		forcePushRocketActions.waitForThreat.name = "Wait for Rocket Threat";
		forcePushRocketActions.waitForThreat.params.duration = 5.0f;
		forcePushRocketActions.waitForThreat.params.threatParamTable[eJediThreatType_Rocket].set(0.2f, 20.0f);

		// setup force push
		forcePushRocketActions.forcePush.name = "Force Push";
		forcePushRocketActions.forcePush.params.mustHitRocket = true;
		forcePushRocketActions.forcePush.params.skipCharge = true;
	}

	// setup saber attack
	saberAttack.name = "Saber Attack";
	{
		// setup move
		saberAttackActions.move.name = "Move";
		saberAttackActions.move.params.destination = eJediAiDestination_Victim;
		saberAttackActions.move.params.minWalkRunDistance = kJediMeleeCorrectionTweak;
		saberAttackActions.move.params.minDashDistance = kJediAiDashActivationDistanceMin;
		saberAttackActions.move.params.facePct = 0.8f;

		// setup saber
		saberAttackActions.saber.name = "Saber";
		saberAttackActions.saber.params.numSwings = 3;
	}

	// setup force push + saber combo
	forcePushSaberCombo.name = "Force Push + Saber Combo";
	forcePushSaberCombo.minRunFrequency = 8.0f;
	{
		// setup move
		forcePushSaberComboActions.move.name = "Move";
		forcePushSaberComboActions.move.params.destination = eJediAiDestination_Victim;
		forcePushSaberComboActions.move.params.minWalkRunDistance = kJediForcePushTier2Min;
		forcePushSaberComboActions.move.params.minDashDistance = kJediAiDashActivationDistanceMin;
		forcePushSaberComboActions.move.params.facePct = 0.8f;

		// setup force push
		forcePushSaberComboActions.forcePush.name = "Force Push";
		forcePushSaberComboActions.forcePush.params.chargeDuration = 0.0f;
		forcePushSaberComboActions.forcePush.params.maxVictimDistance = kJediForcePushTier2Min;

		// setup saber
		forcePushSaberComboActions.saber.name = "Saber";
		forcePushSaberComboActions.saber.params.numSwings = 2;
	}

	// setup saber during rush
	saberDuringRush.name = "Saber During Rush";
	saberDuringRushThreatConstraint.params.addThreatReaction(eJediThreatType_Rush, CJediAiActionConstraintThreat::eThreatReaction_FailIfNone);
	saberDuringRushThreatConstraint.params.addThreatReaction(eJediThreatType_Melee, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
	{
		// setup wait for threat
		saberDuringRushActions.waitForThreat.name = "Wait for Rush Threat";
		saberDuringRushActions.waitForThreat.params.duration = 10.0f;
		saberDuringRushActions.waitForThreat.params.threatParamTable[eJediThreatType_Rush].set(kJediSwingSaberEnterDuration, kJediSwingSaberDistance * 2.0f);

		// setup fake sim
		saberDuringRushActions.fakeSim.name = "Saber Fake Sim";
		saberDuringRushActions.fakeSim.params.clearVictimThreats[eJediThreatType_Rush] = true;
		saberDuringRushActions.fakeSim.params.clearVictimThreats[eJediThreatType_Melee] = true;
		saberDuringRushActions.fakeSim.params.ignoreDamage = true;

		// setup saber
		saberDuringRushActions.swingSaber.name = "Saber";
		saberDuringRushActions.swingSaber.params.numSwings = 1;
	}

	// setup crouch attack
	crouchAttack.name = "Crouch Attack";
	crouchAttack.params.attack = true;
	crouchAttack.minRunFrequency = 6.0f;

	// setup special attacks
	specialAttack.name = "Special Attack";
	specialAttack.selectorParams.debounceActions = true;
	{
		// setup dash attack
		specialAttackActions.dashAttack.name = "Dash Attack";
		specialAttackActions.dashAttack.params.attack = true;
		specialAttackActions.dashAttack.params.destination = eJediAiDestination_Victim;
		specialAttackActions.dashAttack.params.activationDistance = kJediAiDashActivationDistanceMin;
		specialAttackActions.dashAttack.params.distance = kJediSwingSaberDistance;

		// setup jump forward attack
		specialAttackActions.jumpForwardAttack.name = "Jump Forward Attack";
		specialAttackActions.jumpForwardAttack.params.activationDistance = kJediCombatMaxJumpDistance;
		specialAttackActions.jumpForwardAttack.params.attack = eJediAiJumpForwardAttack_Vertical;

		// setup jump over attack
		specialAttackActions.jumpOverAttack.name = "Jump Over Attack";
		specialAttackActions.jumpOverAttack.params.attack = true;
		specialAttackActions.jumpOverAttack.minRunFrequency = 10.0f;

		// setup throw object
		specialAttackActions.throwObject.name = "Throw Object At Victim";
		specialAttackActions.throwObject.params.gripDuration = 0.0f;
		specialAttackActions.throwObject.params.gripTarget = eJediAiForceTkTarget_Object;
		specialAttackActions.throwObject.params.throwTarget = eJediAiForceTkTarget_Victim;
		specialAttackActions.throwObjectThreatConstraint.params.addThreatReaction(eJediThreatType_Rush, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
		specialAttackActions.throwObjectThreatConstraint.params.addThreatReaction(eJediThreatType_Melee, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
	}

	// setup ambient actions
	ambient.name = "Ambient Actions";
	ambient.selectorParams.debounceActions = true;
	{
		// setup force tk
		ambientActions.forceTk.name = "Force Tk";
		ambientActions.forceTk.minRunFrequency = 10.0f;
		ambientActions.forceTk.params.gripDuration = 10.0f;
		ambientActions.forceTk.params.gripTarget = eJediAiForceTkTarget_Victim;
		ambientActions.forceTk.params.throwTarget = eJediAiForceTkTarget_Count;
		ambientActions.forceTk.params.minActivationDistance = 30.0f;
		ambientActions.forceTkThreatConstraint.params.addThreatReaction(eJediThreatType_Rush, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
		ambientActions.forceTkThreatConstraint.params.addThreatReaction(eJediThreatType_Rocket, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
		ambientActions.forceTkThreatConstraint.skipWhileInProgress = true;
		ambientActions.forceTkFakeSimResult.name = "Force Tk Fake Sim Result";
		ambientActions.forceTkFakeSimResult.params.minResult = eJediAiActionSimResult_Beneficial;
		ambientActions.forceTkFakeSimThreats.name = "Force Tk Fake Sim Threats";
		ambientActions.forceTkFakeSimThreats.params.clearVictimThreats[eJediThreatType_Rush] = true;
		ambientActions.forceTkFakeSimThreats.params.clearVictimThreats[eJediThreatType_Rocket] = true;

		// setup jump over rush
		ambientActions.jumpOverRush.name = "Jump Over Rush";
		ambientActions.jumpOverRushThreatConstraint.params.addThreatReaction(eJediThreatType_Rush, CJediAiActionConstraintThreat::eThreatReaction_FailIfNone);
		ambientActions.jumpOverRushThreatConstraint.params.addThreatReaction(eJediThreatType_Melee, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
		{
			// setup wait for threat
			ambientActions.jumpOverRushActions.waitForThreat.name = "Wait for Rush Threat";
			ambientActions.jumpOverRushActions.waitForThreat.params.duration = 10.0f;
			ambientActions.jumpOverRushActions.waitForThreat.params.threatParamTable[eJediThreatType_Rush].set(0.5f, 0.0f);

			// setup saber
			ambientActions.jumpOverRushActions.jumpOver.name = "Jump Over";
			ambientActions.jumpOverRushActions.jumpOver.params.attack = false;
		}

		// setup jump over rocket
		ambientActions.jumpOverRocket.name = "Jump Over Rocket";
		ambientActions.jumpOverRocket.params.attack = eJediAiJumpForwardAttack_None;
		ambientActions.jumpOverRocket.params.distance = kJediMeleeCorrectionTweak;
		ambientActions.jumpOverRocket.params.activationDistance = kJediForwardJumpMaxDistance;
		ambientActions.jumpOverRocketConstraint.params.addThreatReaction(eJediThreatType_Rocket, CJediAiActionConstraintThreat::eThreatReaction_FailIfNone);
	}
}

CJediAiAction **CJediAiActionEngageB2BattleDroid::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}

float *CJediAiActionEngageB2BattleDroid::getActionOddsTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionOddsTable;
}


/////////////////////////////////////////////////////////////////////////////
//
// engage droideka
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionEngageDroideka::CJediAiActionEngageDroideka() {

	// setup jump forward attack
	jumpForwardAttack.constraint = &highSkillLevelConstraint;

	// setup force push
	forcePush.actionTable[0] = &forcePushActions.move;
	forcePush.actionTable[1] = &forcePushActions.forcePush;
	compileTimeAssert(forcePush.eAction_Count == 2);

	// setup kick
	kick.actionTable[0] = &kickActions.move;
	kick.actionTable[1] = &kickActions.kick;
	compileTimeAssert(kick.eAction_Count == 2);

	// setup saber attack
	saberAttack.actionTable[0] = &saberAttackActions.move;
	saberAttack.actionTable[1] = &saberAttackActions.saberFakeSim;
	saberAttackActions.saberFakeSim.decoratedAction = &saberAttackActions.saber;
	saberAttackActions.saberFakeSim.modifySimConstraint = &padawanSkillLevelConstraint;
	compileTimeAssert(saberAttack.eAction_Count == 2);

	// setup force tk
	forceTk.condition = &forceTkActions.condition;
	forceTk.onFailure = &forceTkActions.forceTkNoShield;
	forceTk.onSuccess = &forceTkActions.forceTkShieldFakeSim;
	{
		// setup forceTk no shield
		forceTkActions.forceTkShieldFakeSim.constraint = &forceTkActions.forceTkShieldThreatConstraint;
		forceTkActions.forceTkShieldThreatConstraint.nextConstraint = &lowSkillLevelConstraint;
		forceTkActions.forceTkShieldFakeSim.decoratedAction = &forceTkActions.forceTkShield;

		// setup forceTk shield
		forceTkActions.forceTkShield.constraint = &highSkillLevelConstraint;
	}

	// setup dodge + dash
	dodgeDash.actionTable[0] = &dodgeDashActions.dodge;
	dodgeDash.actionTable[1] = &dodgeDashActions.dashFakeSim;
	compileTimeAssert(dodgeDash.eAction_Count == 2);
	{
		dodgeDashActions.dashFakeSim.decoratedAction = &dodgeDashActions.dash;
	}

	// build my action table
	// if you hit the compileTimeAssert below, you've likely added an
	// action and need to set it up here
	memset(actionTable, 0, sizeof(actionTable));
	actionTable[eAction_DeflectAttack] = &deflectAttack;
	actionTable[eAction_JumpOverAttack] = &jumpOverAttack;
	actionTable[eAction_JumpForwardAttack] = &jumpForwardAttack;
	actionTable[eAction_ForcePush] = &forcePush;
	actionTable[eAction_Kick] = &kick;
	actionTable[eAction_SaberAttack] = &saberAttack;
	actionTable[eAction_ThrowObject] = &throwObject;
	actionTable[eAction_ForceTk] = &forceTk;
	actionTable[eAction_DodgeDash] = &dodgeDash;
	compileTimeAssert(eAction_Count == 9);

	// every action is equally viable
	for (int i = 0; i < eAction_Count; ++i) {
		actionOddsTable[i] = 1.0f;
	}

	// reset my data
	reset();
}

EJediAiAction CJediAiActionEngageDroideka::getType() const {
	return eJediAiAction_EngageDroideka;
}

void CJediAiActionEngageDroideka::reset() {

	// base class version
	BASECLASS::reset();

	// debounce my actions
	selectorParams.debounceActions = true;

	// deflect attack
	deflectAttack.name = "Deflect Attack";

	// setup jump over attack
	jumpOverAttack.name = "Jump Over Attack";
	jumpOverAttack.params.attack = true;
	jumpOverAttack.minRunFrequency = 3.0f;

	// setup jump forward attack
	jumpForwardAttack.name = "Jump Forward Attack";
	jumpForwardAttack.params.activationDistance = kJediCombatMaxJumpDistance;
	jumpForwardAttack.params.attack = eJediAiJumpForwardAttack_Horizontal;

	// setup force push
	forcePush.name = "Force Push";
	forcePush.minRunFrequency = 4.0f;
	{
		// setup move
		forcePushActions.move.name = "Move";
		forcePushActions.move.params.destination = eJediAiDestination_Victim;
		forcePushActions.move.params.minWalkRunDistance = kJediForcePushTier2Min * 0.75f;
		forcePushActions.move.params.minDashDistance = kJediAiDashActivationDistanceMin;
		forcePushActions.move.params.facePct = 0.8f;

		// setup force push
		forcePushActions.forcePush.name = "Force Push";
		forcePushActions.forcePush.params.chargeDuration = 0.0f;
		forcePushActions.forcePush.params.skipCharge = true;
		forcePushActions.forcePush.params.maxVictimDistance = kJediForcePushTier2Min;
	}

	// setup kick
	kick.name = "Kick";
	kick.minRunFrequency = 4.0f;
	{
		// setup move
		kickActions.move.name = "Move";
		kickActions.move.params.destination = eJediAiDestination_Victim;
		kickActions.move.params.minWalkRunDistance = kJediMeleeCorrectionTweak;
		kickActions.move.params.minDashDistance = kJediAiDashActivationDistanceMin;
		kickActions.move.params.facePct = 0.8f;

		// setup kick
		kickActions.kick.name = "Kick";
		kickActions.kick.params.allowDisplacement = true;
	}

	// setup saber attack
	saberAttack.name = "Saber Attack";
	{
		// setup move
		saberAttackActions.move.name = "Move";
		saberAttackActions.move.params.destination = eJediAiDestination_Victim;
		saberAttackActions.move.params.minWalkRunDistance = kJediMeleeCorrectionTweak;
		saberAttackActions.move.params.minDashDistance = kJediAiDashActivationDistanceMin;
		saberAttackActions.move.params.facePct = 0.8f;

		// setup saber
		saberAttackActions.saber.name = "Saber";
		saberAttackActions.saber.params.numSwings = 2;
		saberAttackActions.saberFakeSim.params.postSim = true;
		saberAttackActions.saberFakeSim.params.breakVictimShield = true;
	}

	// setup throw object
	throwObject.name = "Throw Object At Victim";
	throwObject.params.gripDuration = 0.0f;
	throwObject.params.gripTarget = eJediAiForceTkTarget_Object;
	throwObject.params.throwTarget = eJediAiForceTkTarget_Victim;

	// setup force tk
	forceTk.name = "ForceTk";
	forceTk.minRunFrequency = 4.0f;
	{
		// setup condition
		forceTkActions.condition.params.setAllFlagsDisallowed();
		forceTkActions.condition.params.setFlagsAllowed(kJediAiActorStateFlag_Shielded);

		// setup forceTkNoShield
		forceTkActions.forceTkNoShield.name = "ForceTk No Shield";
		forceTkActions.forceTkNoShield.params.gripDuration = 0.5f;
		forceTkActions.forceTkNoShield.params.gripTarget = eJediAiForceTkTarget_Victim;
		forceTkActions.forceTkNoShield.params.throwTarget = eJediAiForceTkTarget_Count; // none
		forceTkActions.forceTkNoShield.params.failIfNotThrowable = true;

		// setup forceTkShield
		forceTkActions.forceTkShieldFakeSim.name = "ForceTk Shield Fake Sim";
		forceTkActions.forceTkShieldFakeSim.params.breakVictimShield = true;
		forceTkActions.forceTkShieldThreatConstraint.params.addThreatReaction(eJediThreatType_Blaster, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
		forceTkActions.forceTkShieldThreatConstraint.params.addThreatReaction(eJediThreatType_Rush, CJediAiActionConstraintThreat::eThreatReaction_FailIfAny);
		forceTkActions.forceTkShield.name = "ForceTk Shield";
		forceTkActions.forceTkShield.params.gripDuration = 0.5f;
		forceTkActions.forceTkShield.params.gripTarget = eJediAiForceTkTarget_Victim;
		forceTkActions.forceTkShield.params.throwTarget = eJediAiForceTkTarget_Count; // none
		forceTkActions.forceTkShield.minRunFrequency = 8.0f;
	}

	// setup dodge + dash
	dodgeDash.name = "Dodge + Dash";
	dodgeDashThreatConstraint.params.addThreatReaction(eJediThreatType_Blaster, CJediAiActionConstraintThreat::eThreatReaction_FailIfNone);
	{
		// setup dodge
		dodgeDashActions.dodge.name = "Dodge";
		dodgeDashActions.dodge.params.attack = false;
		dodgeDashActions.dodge.params.onlyWhenThreatened = true;

		// setup dash
		dodgeDashActions.dashFakeSim.name = "Dash Fake Sim";
		dodgeDashActions.dashFakeSim.params.breakVictimShield = true;
		dodgeDashActions.dashFakeSim.params.clearVictimThreats[eJediThreatType_Blaster] = true;
		dodgeDashActions.dash.name = "Dash";
		dodgeDashActions.dash.params.attack = false;
		dodgeDashActions.dash.params.destination = eJediAiDestination_Victim;
		dodgeDashActions.dash.params.distance = kJediMeleeCorrectionTweak;
		dodgeDashActions.dash.params.activationDistance = kJediAiDashActivationDistanceMin;
	}
}

CJediAiAction **CJediAiActionEngageDroideka::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}

float *CJediAiActionEngageDroideka::getActionOddsTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionOddsTable;
}


/////////////////////////////////////////////////////////////////////////////
//
// engage
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionEngage::CJediAiActionEngage() {

	// build my action table
	// if you hit the compileTimeAssert below, you've likely added an
	// action and need to set it up here
	memset(actionTable, 0, sizeof(actionTable));
	actionTable[eAction_EngageTrandoshanInfantry] = &engageTrandoshanInfantry;
	actionTable[eAction_EngageTrandoshanMelee] = &engageTrandoshanMelee;
	actionTable[eAction_EngageTrandoshanCommando] = &engageTrandoshanCommando;
	actionTable[eAction_EngageTrandoshanConcussive] = &engageTrandoshanConcussive;
	actionTable[eAction_EngageTrandoshanFlutterpack] = &engageTrandoshanFlutterpack;
	actionTable[eAction_EngageB1BattleDroid] = &engageB1BattleDroid;
	actionTable[eAction_EngageB1MeleeDroid] = &engageB1MeleeDroid;
	actionTable[eAction_EngageB1JetpackDroid] = &engageB1JetpackDroid;
	actionTable[eAction_EngageB2BattleDroid] = &engageB2BattleDroid;
	actionTable[eAction_EngageDroideka] = &engageDroideka;
	compileTimeAssert(eAction_Count == 10);

	// reset my data
	reset();
}

CJediAiActionEngage::~CJediAiActionEngage() {
}

void CJediAiActionEngage::init(CJediAiMemory *newWorldState) {

	// base class version
	BASECLASS::init(newWorldState);
}

EJediAiAction CJediAiActionEngage::getType() const {
	return eJediAiAction_Engage;
}

bool CJediAiActionEngage::isNotSelectable() const {
	return CJediAiAction::isNotSelectable();
}

CJediAiAction **CJediAiActionEngage::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}

CJediAiAction *CJediAiActionEngage::selectAction(CJediAiMemory *simMemory) const {

	// if I have no victim, I can't engage
	SJediAiActorState *victimState = (simMemory != NULL ? simMemory->victimState : memory->victimState);
	if (victimState->actor == NULL || victimState->hitPoints <= 0.0f) {
		return NULL;
	}

	// if my victim is incapacitated, bail
	if (victimState->flags & kJediAiActorStateFlag_Incapacitated) {
		return NULL;
	}

	// get the action for our victim
	EAction actionType = getActionForVictim(*victimState);
	CJediAiAction *action = (actionType == eAction_Count ? NULL : actionTable[actionType]);

	// if we have an action, simulate it
	if (action != NULL) {
		if (simMemory != NULL) {
			action->simulate(*simMemory);
		} else {
			CJediAiMemory actionSimMemory(*memory);
			action->simulate(actionSimMemory);
		}
	}

	// return our action
	return action;
}

EJediAiActionResult CJediAiActionEngage::setCurrentAction(CJediAiAction *action) {

	// base class version
	bool actionChanging = (selectorData.currentAction != action);
	EJediAiActionResult result = BASECLASS::setCurrentAction(action);

	// done
	return result;
}

CJediAiActionEngage::EAction CJediAiActionEngage::getActionForVictim(const SJediAiActorState &victimState) {

	// select an action specific to my victim's type
	EAction action = eAction_Count;
	switch (victimState.enemyType) {

		// trandoshan infantry
		case eJediEnemyType_TrandoshanInfantry:
			action = eAction_EngageTrandoshanInfantry;
			break;

		// trandoshan melee
		case eJediEnemyType_TrandoshanMelee:
			action = eAction_EngageTrandoshanMelee;
			break;

		// trandoshan commando
		case eJediEnemyType_TrandoshanCommando:
			action = eAction_EngageTrandoshanCommando;
			break;

		// trandoshan concussive
		case eJediEnemyType_TrandoshanConcussive:
			action = eAction_EngageTrandoshanConcussive;
			break;

		// trandoshan flutterpack
		case eJediEnemyType_TrandoshanFlutterpack:
			action = eAction_EngageTrandoshanFlutterpack;
			break;

		// b1 battle and grenade droids
		case eJediEnemyType_B1BattleDroid:
		case eJediEnemyType_B1GrenadeDroid:
			action = eAction_EngageB1BattleDroid;
			break;

		// b1 melee droid
		case eJediEnemyType_B1MeleeDroid:
			action = eAction_EngageB1MeleeDroid;
			break;

		// b1 jetpack droid
		case eJediEnemyType_B1JetpackDroid:
			action = eAction_EngageB1JetpackDroid;
			break;

		// b2 battle droid
		case eJediEnemyType_B2BattleDroid:
		case eJediEnemyType_B2RocketDroid:
			action = eAction_EngageB2BattleDroid;
			break;

		// droideka
		case eJediEnemyType_Droideka:
			action = eAction_EngageDroideka;
			break;
	}
	compileTimeAssert(eJediEnemyType_Count == 13);
	return action;
}


/////////////////////////////////////////////////////////////////////////////
//
// combat
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionCombat::CJediAiActionCombat() {

	// setup 'give other jedi space'
	giveOtherJediSpace.constraint = &tooCloseToOtherJediConstraint;
	giveOtherJediSpace.actionTable[0] = &giveOtherJediSpaceActions.meleeJumpOver;
	giveOtherJediSpace.actionTable[1] = &giveOtherJediSpaceActions.dodgeLateral;
	giveOtherJediSpace.actionTable[2] = &giveOtherJediSpaceActions.dodgeBack;
	giveOtherJediSpace.actionTable[3] = &giveOtherJediSpaceActions.nonMeleeJumpOver;
	compileTimeAssert(giveOtherJediSpace.eAction_Count == 4);
	{
		// setup melee jump over
		giveOtherJediSpaceActions.meleeJumpOver.constraint = &giveOtherJediSpaceActions.meleeEnemyTypeConstraint;

		// setup dodge lateral
		giveOtherJediSpaceActions.dodgeLateral.setAction(0, &giveOtherJediSpaceActions.dodgeLateralActions.dodgeLeft, 1.0f);
		giveOtherJediSpaceActions.dodgeLateral.setAction(1, &giveOtherJediSpaceActions.dodgeLateralActions.dodgeRight, 1.0f);
		compileTimeAssert(giveOtherJediSpaceActions.dodgeLateral.eAction_Count == 2);

		// setup non-melee jump over
		giveOtherJediSpaceActions.nonMeleeJumpOver.constraint = &giveOtherJediSpaceActions.nonMeleeEnemyTypeConstraint;
	}

	// build my action table
	memset(actionTable, 0, sizeof(actionTable));
	actionTable[eAction_GiveOtherJediSpace] = &giveOtherJediSpace;
	actionTable[eAction_Engage] = &engage;
	actionTable[eAction_Defend] = &defend;
	actionTable[eAction_Idle] = &idle;
	compileTimeAssert(eAction_Count == 4);

	// reset my data
	reset();
}

CJediAiActionCombat::~CJediAiActionCombat() {
}

EJediAiAction CJediAiActionCombat::getType() const {
	return eJediAiAction_Combat;
}

void CJediAiActionCombat::reset() {

	// base class version
	BASECLASS::reset();

	// select every 1/8th of a second
	selectorParams.selectFrequency = 0.0f;
	selectorParams.ifEqualUseCurrentAction = false;

	// setup 'give other jedi space'
	giveOtherJediSpace.name = "Give Other Jedi Space";
	tooCloseToOtherJediConstraint.params.desiredValue = true;
	{
		// setup melee jump over
		giveOtherJediSpaceActions.meleeJumpOver.name = "Melee Jump Over";
		giveOtherJediSpaceActions.meleeEnemyTypeConstraint.params.setAllEnemyTypesDisallowed();
		giveOtherJediSpaceActions.meleeEnemyTypeConstraint.params.setEnemyTypeAllowed(eJediEnemyType_Unknown);
		giveOtherJediSpaceActions.meleeEnemyTypeConstraint.params.setEnemyTypeAllowed(eJediEnemyType_TrandoshanMelee);
		giveOtherJediSpaceActions.meleeEnemyTypeConstraint.params.setEnemyTypeAllowed(eJediEnemyType_B1MeleeDroid);

		// setup dodge lateral
		giveOtherJediSpaceActions.dodgeLateral.name = "Dodge Lateral";
		{
			giveOtherJediSpaceActions.dodgeLateralActions.dodgeLeft.name = "Dodge Left";
			giveOtherJediSpaceActions.dodgeLateralActions.dodgeLeft.params.dir = eJediDodgeDir_Left;
			giveOtherJediSpaceActions.dodgeLateralActions.dodgeRight.name = "Dodge Right";
			giveOtherJediSpaceActions.dodgeLateralActions.dodgeRight.params.dir = eJediDodgeDir_Right;
		}

		// setup dodge back
		giveOtherJediSpaceActions.dodgeBack.name = "Dodge Back";
		giveOtherJediSpaceActions.dodgeBack.params.dir = eJediDodgeDir_Back;

		// setup non-melee jump over
		giveOtherJediSpaceActions.nonMeleeJumpOver.name = "Non-Melee Jump Over";
		giveOtherJediSpaceActions.nonMeleeEnemyTypeConstraint.params.setAllEnemyTypesAllowed();
		giveOtherJediSpaceActions.nonMeleeEnemyTypeConstraint.params.setEnemyTypeAllowed(eJediEnemyType_Unknown);
		giveOtherJediSpaceActions.nonMeleeEnemyTypeConstraint.params.setEnemyTypeDisallowed(eJediEnemyType_TrandoshanMelee);
		giveOtherJediSpaceActions.nonMeleeEnemyTypeConstraint.params.setEnemyTypeDisallowed(eJediEnemyType_B1MeleeDroid);
	}

	// setup 'engage'
	engage.name = "Engage";
	notTooCloseToOtherJediConstraint.params.desiredValue = false;

	// setup 'defend'
	defend.name = "Defend";

	// setup 'idle'
	idle.name = "Idle";
}

EJediAiActionResult CJediAiActionCombat::checkConstraints(const CJediAiMemory &simMemory, bool simulating) const {

	// base class version
	EJediAiActionResult result = BASECLASS::checkConstraints(simMemory, simulating);
	return result;
}

EJediAiActionResult CJediAiActionCombat::onBegin() {

	// base class version
	BASECLASS::onBegin();

	// make sure that I have nothing going on
	setCurrentAction(NULL);

	// in progress
	return eJediAiActionResult_InProgress;
}

void CJediAiActionCombat::onEnd() {

	// base class version
	BASECLASS::onEnd();
}

void CJediAiActionCombat::updateTimers(float dt) {

	// we need a self to operate
	if (memory->selfState.jedi == NULL || !memory->selfState.isAiControlled) {
		return;
	}

	// base class version
	BASECLASS::updateTimers(dt);

	// if I'm incapacitated or my victim changed, drop any current action
	if (memory->selfState.hitPoints <= 0.0f || memory->victimChanged) {
		setCurrentAction(NULL);
	}
}

EJediAiActionResult CJediAiActionCombat::update(float dt) {

	// we need a self to operate
	if (memory->selfState.jedi == NULL || !memory->selfState.isAiControlled) {
		return eJediAiActionResult_InProgress;
	}

	// update our current action
	EJediAiActionResult result = BASECLASS::update(dt);

	// if our current action is no longer in progress, choose another one
	if (result != eJediAiActionResult_InProgress) {
		setCurrentAction(NULL);
	}

	// we are always in progress
	return eJediAiActionResult_InProgress;
}

CJediAiAction **CJediAiActionCombat::getActionTable(int *actionCount) {
	if (actionCount != NULL) {
		*actionCount = eAction_Count;
	}
	return actionTable;
}
