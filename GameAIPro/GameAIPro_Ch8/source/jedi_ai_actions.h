#ifndef __JEDI_AI_ACTIONS__
#define __JEDI_AI_ACTIONS__

#ifndef __JEDI_COMMON__
	#include "jedi_common.h"
#endif

#ifndef __JEDI_AI_CONSTRAINTS__
	#include "jedi_ai_constraints.h"
#endif

#ifndef __JEDI_AI_MEMORY__
	#include "jedi_ai_memory.h"
#endif


/////////////////////////////////////////////////////////////////////////////
//
// jedi action base class
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiAction {
public:

	// construction
	CJediAiAction();

	// initialize
	virtual void init(CJediAiMemory *memory);

	// has this action been initialized?
	// if not, error
	virtual bool ensureInitialization() const;

	// reset all data
	virtual void reset();

	// get my action type
	virtual EJediAiAction getType() const = 0;

	// check my constraints
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;

	// called when this action begins
	virtual EJediAiActionResult onBegin();

	// called when this action ends
	virtual void onEnd();

	// simulate the results of this action on the specified memory
	virtual void simulate(CJediAiMemory &simMemory) = 0;

	// update any timers I may have
	virtual void updateTimers(float dt);

	// update this action
	// returns whether or not this action is complete
	virtual EJediAiActionResult update(float dt) = 0;

	// a name for easier tracking in the debugger
	// this defaults to the type name, so you only have to set it if you care
	const char *name;
	const char *getName() const;

	// world state
	CJediAiMemory *memory;

	// constraint linked list
	CJediAiActionConstraint *constraint;

	// simulation results
	SJediAiActionSimSummary simSummary;

	// what was the current time when this action was last run?
	float lastRunTime;

	// how often can this action be begun?
	// defaults to zero seconds
	float minRunFrequency;

	// flags
	enum {

		// this action is in progress
		kFlag_InProgress = 1,

		// this action may not be selected
		kFlag_IsNotSelectable = (kFlag_InProgress << 1),

		// this flag allows you to find this action more easily in the debugger
		kFlag_DebugMe = (kFlag_IsNotSelectable << 1),

		// insert new CJediAiAction flags above
		// this constant denotes the next flag value available to subclasses
		// begin you subclass' flags list like this: kFlag_XXX = BASECLASS::kFlag_NextAvailable,
		kFlag_NextAvailable = (kFlag_DebugMe << 1)

	};
	unsigned int flags;

	// this action is in progress
	bool isInProgress() const;

	// can this action be selected?
	virtual bool isNotSelectable() const;

	// set whether or not this action is selectable
	virtual void setIsNotSelectable(bool notSelectable);
};


/////////////////////////////////////////////////////////////////////////////
//
// base class for all action composed of other actions
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionComposite : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// CJediAiAction methods
	virtual void init(CJediAiMemory *newWorldState);
	virtual void reset();
	virtual bool isNotSelectable() const;

	// get my action table
	virtual CJediAiAction **getActionTable(int *actionCount) = 0;
	CJediAiAction *const *getActionTable(int *actionCount) const;

	// get a specific action from my action table
	CJediAiAction *getAction(int index);
	const CJediAiAction *getAction(int index) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// base class for all actions running sub-actions in parallel
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionParallelBase : public CJediAiActionComposite {
public:
	typedef CJediAiActionComposite BASECLASS;

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);

	// get my action result table
	// I'll store sub-action update results here so I know the results of my sub-actions
	virtual EJediAiActionResult *getActionResultTable(int *actionResultCount) = 0;
	virtual EJediAiActionResult const *getActionResultTable(int *actionResultCount) const;

	// does the specified action loop?
	virtual bool doesActionLoop(int actionIndex) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// parallel
//
/////////////////////////////////////////////////////////////////////////////

template <int kActionCount>
class CJediAiActionParallel : public CJediAiActionParallelBase {
public:
	typedef CJediAiActionParallelBase BASECLASS;
	enum { eAction_Count = kActionCount };

	// action table
	CJediAiAction *actionTable[eAction_Count];

	// action result table
	EJediAiActionResult actionResultTable[eAction_Count];

	// action result table
	struct {
		bool actionLoopTable[eAction_Count];
	} params;

	// construction
	CJediAiActionParallel() {
		memset(actionTable, 0, sizeof(actionTable));
		reset();
	}

	// CJediAiAction methods
	virtual EJediAiAction getType() const {
		return eJediAiAction_Parallel;
	}
	virtual void reset() {
		memset(actionResultTable, 0, sizeof(actionResultTable));
		memset(params.actionLoopTable, 0, sizeof(params.actionLoopTable));
		BASECLASS::reset();
	}

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount) {
		if (actionCount) *actionCount = eAction_Count;
		return actionTable;
	}

	// CJediAiActionParallel methods
	virtual EJediAiActionResult *getActionResultTable(int *actionResultCount) {
		if (actionResultCount) *actionResultCount = eAction_Count;
		return actionResultTable;
	}
	virtual bool doesActionLoop(int actionIndex) const {
		return (actionIndex > -1 && actionIndex < eAction_Count ? params.actionLoopTable[actionIndex] : false);
	}
};


/////////////////////////////////////////////////////////////////////////////
//
// base class for all actions running sub-action sequences
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionSequenceBase : public CJediAiActionComposite {
public:
	typedef CJediAiActionComposite BASECLASS;

	// parameters
	struct {
		float timeBetweenActions;
		bool loop;
		bool allowActionFailure;
		EJediAiActionSimResult minFailureResult;
	} params;

	// update data
	struct {
		float timer;
		CJediAiAction *currentAction;
		int nextActionIndex;
	} data;

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);
	virtual bool isNotSelectable() const;

	// get the next available action in the sequence starting with the specified index
	virtual CJediAiAction *getNextAction(int &nextActionIndex) const;

	// begin the next available action in the sequence
	virtual EJediAiActionResult beginNextAction();
};


/////////////////////////////////////////////////////////////////////////////
//
// sequence
//
/////////////////////////////////////////////////////////////////////////////

template <int kActionCount>
class CJediAiActionSequence : public CJediAiActionSequenceBase {
public:
	typedef CJediAiActionSequenceBase BASECLASS;
	enum { eAction_Count = kActionCount };

	// action table
	CJediAiAction *actionTable[eAction_Count];

	// construction
	CJediAiActionSequence() {
		memset(actionTable, 0, sizeof(actionTable));
		reset();
	}

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount) {
		if (actionCount) *actionCount = eAction_Count;
		return actionTable;
	}
};


/////////////////////////////////////////////////////////////////////////////
//
// base class for all action selectors
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionSelectorBase : public CJediAiActionComposite {
public:
	typedef CJediAiActionComposite BASECLASS;

	// parameters
	struct SSelectorParams {
		float selectFrequency;
		bool debounceActions;
		bool allowNegativeActions;
		bool ifEqualUseCurrentAction; // default is true
	} selectorParams;

	// update data
	struct SSelectorData {
		float selectTimer;
		CJediAiAction *debouncedAction;
		CJediAiAction *bestAction;
		CJediAiAction *currentAction;
		EJediAiActionResult currentActionResult;
	} selectorData;

	// construction
	CJediAiActionSelectorBase();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);

	// set my current action
	EJediAiActionResult setCurrentAction(CJediAiAction *action);

	// simulate each action and select which one is best
	virtual CJediAiAction *selectAction(CJediAiMemory *simMemory) const;

	// compare action simulation summaries and select one
	virtual int compareAndSelectAction(int actionCount, CJediAiAction *const actionTable[], EJediAiActionSimResult bestResult) const;

	// can I select the specified action
	virtual bool canSelectAction(int actionIndex) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// selector
//
/////////////////////////////////////////////////////////////////////////////

template <int kActionCount>
class CJediAiActionSelector : public CJediAiActionSelectorBase {
public:
	typedef CJediAiActionSelectorBase BASECLASS;
	enum { eAction_Count = kActionCount };

	// action table
	CJediAiAction *actionTable[eAction_Count];

	// construction
	CJediAiActionSelector() {
		memset(actionTable, 0, sizeof(actionTable));
		reset();
	}

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount) {
		if (actionCount) *actionCount = eAction_Count;
		return actionTable;
	}
};


/////////////////////////////////////////////////////////////////////////////
//
// binary selector
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionSelectorBinary : public CJediAiActionSelectorBase {
public:
	typedef CJediAiActionSelectorBase BASECLASS;
	enum {
		eAction_OnFailure = 0,
		eAction_OnSuccess,
		eAction_Count
	};

	// action table
	union {
		CJediAiAction *actionTable[eAction_Count];
		struct {
			CJediAiAction *onFailure;
			CJediAiAction *onSuccess;
		};
	};

	// condition
	// if this fails, we use the onFailure action
	// otherwise, we use the onSuccess action
	CJediAiActionConstraint *condition;

	// construction
	CJediAiActionSelectorBinary();

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);
	virtual CJediAiAction *selectAction(CJediAiMemory *simMemory) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// base class for all random action selectors
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionRandomBase : public CJediAiActionSelectorBase {
public:
	typedef CJediAiActionSelectorBase BASECLASS;

	// construction
	CJediAiActionRandomBase();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;

	// CJediAiActionSelector methods
	virtual int compareAndSelectAction(int actionCount, CJediAiAction *const actionTable[], EJediAiActionSimResult bestResult) const;
	virtual bool canSelectAction(int actionIndex) const;

	// get my action odds table
	virtual float *getActionOddsTable(int *actionCount) = 0;
	const float *getActionOddsTable(int *actionCount) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// random
//
/////////////////////////////////////////////////////////////////////////////

template <int kActionCount>
class CJediAiActionRandom : public CJediAiActionRandomBase {
public:
	typedef CJediAiActionRandomBase BASECLASS;
	enum { eAction_Count = kActionCount };

	// action table
	CJediAiAction *actionTable[eAction_Count];

	// action odds table
	float actionOddsTable[eAction_Count];

	// construction
	CJediAiActionRandom() {
		memset(actionTable, 0, sizeof(actionTable));
		memset(actionOddsTable, 0, sizeof(actionOddsTable));
		reset();
	}

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount) {
		if (actionCount) *actionCount = eAction_Count;
		return actionTable;
	}

	// CJediAiActionRandomBase methods
	virtual float *getActionOddsTable(int *actionCount) {
		if (actionCount) *actionCount = eAction_Count;
		return actionOddsTable;
	}

	// set an action into our table
	void setAction(int index, CJediAiAction *action, float odds) {
		if (index >= 0 && index < eAction_Count) {
			actionTable[index] = action;
			actionOddsTable[index] = odds;
		}
	}
};


/////////////////////////////////////////////////////////////////////////////
//
// decorator
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionDecorator : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// decorated action
	CJediAiAction *decoratedAction;

	// construction
	CJediAiActionDecorator();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void init(CJediAiMemory *newWorldState);
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);
	virtual bool isNotSelectable() const;
};


/////////////////////////////////////////////////////////////////////////////
//
// insert fake data into the simulation
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionFakeSim : public CJediAiActionDecorator {
public:
	typedef CJediAiActionDecorator BASECLASS;

	// set this value to the damageVictim value below to kill the victim
	static const float kKillVictim;

	// parameters
	struct {
		EJediAiActionSimResult minResult;
		float duration;
		float damageVictim;
		bool clearVictimThreats[eJediThreatType_Count];
		bool breakVictimShield;
		bool makeVictimStumble;
		bool ignoreDamage;
		bool postSim;
	} params;

	// if this constraint fails, don't modify the simulation, just run it for our decorated action
	CJediAiActionConstraint *modifySimConstraint;

	// construction
	CJediAiActionFakeSim();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual EJediAiActionResult update(float dt);
};


/////////////////////////////////////////////////////////////////////////////
//
// walk/run
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionWalkRun : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct {
		EJediAiDestination destination;
		float distance;
		float minRunDistance;
		float facePct;
	} params;

	// construction
	CJediAiActionWalkRun();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual EJediAiActionResult update(float dt);

	// compute my target position
	CVector computeTargetPos(CJediAiMemory &memory);
};


/////////////////////////////////////////////////////////////////////////////
//
// dash
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionDash : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct {
		EJediAiDestination destination;
		float distance;
		float activationDistance;
		bool attack;
		bool ignoreMinDistance;
	} params;

	// update data
	struct {
		bool wasDashing;
	} data;

	// construction
	CJediAiActionDash();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual EJediAiActionResult update(float dt);
};


/////////////////////////////////////////////////////////////////////////////
//
// strafe
//
/////////////////////////////////////////////////////////////////////////////

// strafe direction
enum EStrafeDir {
	eStrafeDir_Left,
	eStrafeDir_Right,
	eStrafeDir_Forward,
	eStrafeDir_Backward,
	eStrafeDir_Count
};

class CJediAiActionStrafe : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct {
		EStrafeDir dir; // defaults to 'count'
		float moveDistanceMin; // defaults to 1.0f
		float moveDistanceMax; // defaults to 10.0f
		float distanceFromVictimMin; // defaults to 0.0f
		float distanceFromVictimMax; // defaults to 'infinity'
		bool requireVictim; // defaults to true
	} params;

	// update data
	struct {
		CVector wSelfPrevPos;
		CVector wSelfStartPos;
		CVector wSelfEndPos;
		float desiredMoveDistance;
		float moveDistance;
	} data;

	// construction
	CJediAiActionStrafe();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual EJediAiActionResult update(float dt);

	// randomly choose a strafe direction
	static EStrafeDir chooseRandomDir();

	// calculate the strafe direction
	static CVector computeStrafeDir(EStrafeDir dir, const CJediAiMemory &memory);

	// calculate the target position
	static CVector computeTargetPos(EStrafeDir dir, float distance, const CJediAiMemory &memory);
};


/////////////////////////////////////////////////////////////////////////////
//
// jump
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionJumpForward : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct {
		float distance;
		float activationDistance;
		EJediAiJumpForwardAttack attack;
	} params;

	// update data
	struct {
		bool isJumping;
	} data;

	// construction
	CJediAiActionJumpForward();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);

	// perform a collision check
	static bool checkCollision(const CJediAiMemory &memory, float distance);

	// compute my target position
	static CVector computeTargetPos(const CJediAiMemory &memory, float distance);
};


/////////////////////////////////////////////////////////////////////////////
//
// jump over enemy
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionJumpOver : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct {
		bool attack;
	} params;

	// update data
	struct {
		bool isJumping;
	} data;

	// construction
	CJediAiActionJumpOver();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual EJediAiActionResult update(float dt);

	// perform a collision check
	static bool checkCollision(const CJediAiMemory &memory);
};


/////////////////////////////////////////////////////////////////////////////
//
// dodge
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionDodge : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct SDodgeParams {
		EJediDodgeDir dir;
		bool attack;
		bool onlyWhenThreatened;
	} params;

	// update data
	struct {
		EJediDodgeDir dir;
	} data;

	// construction
	CJediAiActionDodge();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual EJediAiActionResult update(float dt);

	// choose the best parameters based on the specified memory state
	static EJediDodgeDir chooseBestDir(const CJediAiMemory &simMemory);

	// randomly choose a dodge direction
	static EJediDodgeDir chooseRandomDir();

	// should we perform a flip dodge or a normal dodge?
	static bool shouldFlipDodge(const CJediAiMemory &memory);

	// check collision
	static float checkCollision(EJediDodgeDir dir, const CJediAiMemory &memory);

	// calculate the dodge direction
	static CVector computeDodgeDir(EJediDodgeDir dir, const CJediAiMemory &memory);

	// calculate the target position
	static CVector computeTargetPos(EJediDodgeDir dir, bool attack, const CJediAiMemory &memory);
};


/////////////////////////////////////////////////////////////////////////////
//
// crouch
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionCrouch : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct SCrouchParams {
		float duration;
		bool attack;
	} params;

	// update data
	struct {
		float timer;
		float duration;
		bool attacked;
	} data;

	// construction
	CJediAiActionCrouch();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);
};


/////////////////////////////////////////////////////////////////////////////
//
// deflect
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionDeflect : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct {
		float duration;
		bool deflectAtEnemies;
	} params;

	// update data
	struct {
		float duration;
		float timer;
	} data;

	// construction
	CJediAiActionDeflect();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);
};


/////////////////////////////////////////////////////////////////////////////
//
// block
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionBlock : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct SBlockParams {
		EJediBlockDir dir;
		float duration;
	} params;

	// update data
	struct {
		SBlockParams params;
		float timer;
	} data;

	// construction
	CJediAiActionBlock();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);

	// choose best parameters for the specified memory
	void chooseBestParams(CJediAiMemory &simMemory, SBlockParams &bestParams) const;
	EJediBlockDir chooseBestDir(CJediAiMemory &simMemory) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// swing saber
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionSwingSaber : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct {
		int numSwings;
		float timeBeforeSwings;
		float timeBetweenSwings;
		bool onlyFromBehind;
	} params;

	// update data
	struct {
		int counter;
		float timer;
	} data;

	// construction
	CJediAiActionSwingSaber();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);

	// enqueue a saber swing
	void enqueueSaberSwing();
};


/////////////////////////////////////////////////////////////////////////////
//
// kick
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionKick : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct {
		bool allowDisplacement; // default is true
	} params;

	// construction
	CJediAiActionKick();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual EJediAiActionResult update(float dt);
};


/////////////////////////////////////////////////////////////////////////////
//
// force push
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionForcePush : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct {
		float chargeDuration;
		float maxVictimDistance;
		bool mustHitGrenade;
		bool mustHitRocket;
		bool skipCharge;
	} params;

	// update data
	struct {
		float chargeTimer;
		bool forcePushed;
	} data;

	// construction
	CJediAiActionForcePush();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);
};


/////////////////////////////////////////////////////////////////////////////
//
// force tk
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionForceTk : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct {
		CVector iThrowVelocity;
		EJediAiForceTkTarget gripTarget;
		EJediAiForceTkTarget throwTarget;
		float gripDuration;
		float minActivationDistance;
		bool failIfNotThrowable;
		bool skipEnter;
	} params;

	// update data
	struct {
		float gripTimer;
		bool twoHanded;
		bool throwing;
		bool failIfNotThrowable;
	} data;

	// construction
	CJediAiActionForceTk();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);
};


/////////////////////////////////////////////////////////////////////////////
//
// defensive stance
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionDefensiveStance : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct {
		bool exitOnThreat[eJediThreatType_Count];
		float duration;
	} params;

	// update data
	struct {
		float timer;
	} data;

	// construction
	CJediAiActionDefensiveStance();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);
};


/////////////////////////////////////////////////////////////////////////////
//
// wait for threat
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionWaitForThreat : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// threat parameters
	struct SThreatParams {
		float distance;
		float durationOffset;
		bool inUse;
		void set(float durationOffset, float distance) {
			this->distance = max(0.0f, distance);
			this->durationOffset = max(0.0f, durationOffset);
			this->inUse = true;
		}
	};

	// parameters
	struct {
		SThreatParams threatParamTable[eJediThreatType_Count];
		float duration;
	} params;

	// update data
	struct {
		float timer;
	} data;

	// construction
	CJediAiActionWaitForThreat();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);

	// compute our wait duration for a given threat
	float computeWaitDurationForThreat(const CJediAiMemory::SJediThreatTypeData &threatTypeData, const SThreatParams &threatParams) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// taunt
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionTaunt : public CJediAiAction {
public:
	typedef CJediAiAction BASECLASS;

	// parameters
	struct {
		float minDistance;
		float enrageTargetOdds;
		bool skipIfAlreadyRushing;
		bool failUnlessRushing;
	} params;

	// update data
	struct {
		float timer;
		bool enragedEnemy;
	} data;

	// construction
	CJediAiActionTaunt();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);
};


/////////////////////////////////////////////////////////////////////////////
//
// defend
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionDefend : public CJediAiActionRandomBase {
public:
	typedef CJediAiActionRandomBase BASECLASS;

	// action table
	enum {
		eAction_DodgeLeft,
		eAction_DodgeRight,
		eAction_DodgeBack,
		eAction_Crouch,
		eAction_Deflect,
		eAction_Block,
		eAction_JumpForward,
		eAction_JumpOver,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// odds table
	float actionOddsTable[eAction_Count];

	// actions to choose from
	CJediAiActionDodge dodgeLeft;
	CJediAiActionDodge dodgeRight;
	CJediAiActionDodge dodgeBack;
	CJediAiActionCrouch crouch;
	CJediAiActionDeflect deflect;
	CJediAiActionBlock block;
	CJediAiActionJumpForward jumpForward;
	CJediAiActionConstraintThreat jumpForwardThreatConstraint;
	CJediAiActionJumpOver jumpOver;
	CJediAiActionConstraintThreat jumpOverThreatConstraint;

	// construction
	CJediAiActionDefend();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual void simulate(CJediAiMemory &simMemory);

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);

	// CJediAiActionRandomBase methods
	virtual float *getActionOddsTable(int *actionCount);
};


/////////////////////////////////////////////////////////////////////////////
//
// idle
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionIdle : public CJediAiActionSequenceBase {
public:
	typedef CJediAiActionSequenceBase BASECLASS;

	// action table
	enum {
		eAction_DefensiveStance,
		eAction_Ambient,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// defensive stance
	CJediAiActionDefensiveStance defensiveStance;

	// ambient actions
	CJediAiActionRandom<5> ambient;
	struct {

		// strafes
		CJediAiActionStrafe strafeLeft;
		CJediAiActionStrafe strafeRight;
		CJediAiActionStrafe strafeForward;
		CJediAiActionStrafe strafeBackward;

		// flourish
		CJediAiActionDeflect flourish;

	} ambientActions;

	// construction
	CJediAiActionIdle();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual EJediAiActionResult update(float dt);

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);
};


/////////////////////////////////////////////////////////////////////////////
//
// move
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionMove : public CJediAiActionParallelBase {
public:
	typedef CJediAiActionParallelBase BASECLASS;

	// parameters
	struct {
		EJediAiDestination destination;
		float activationDistance;
		float dashActivationDistance;
		float minDashDistance;
		float minWalkRunDistance;
		float facePct;
		bool isRelevant;
		bool failIfTooClose;
	} params;

	// action table
	enum {
		eAction_Deflect,
		eAction_Move,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// action result table
	EJediAiActionResult actionResultTable[eAction_Count];

	// move actions
	CJediAiActionSequence<2> move;
	struct {
		CJediAiActionDash dash;
		CJediAiActionWalkRun walkRun;
	} moveActions;

	// deflect
	CJediAiActionDeflect deflect;

	// construction
	CJediAiActionMove();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void simulate(CJediAiMemory &simMemory);
	virtual EJediAiActionResult update(float dt);

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);

	// CJediAiActionParallelBase methods
	virtual EJediAiActionResult *getActionResultTable(int *actionResultCount);
	virtual bool doesActionLoop(int actionIndex) const;

	// pass my parameters on to my children
	virtual void updateParams();
};


/////////////////////////////////////////////////////////////////////////////
//
// counter a blaster attack
// 1) move back
// 2) wait in defensive stance for a blaster attack
// 3) counter the blaster attack by deflecting it back at the attacker
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionBlasterCounterAttack : public CJediAiActionSequenceBase {
public:
	typedef CJediAiActionSequenceBase BASECLASS;

	// action table
	enum {
		eAction_MoveBack,
		eAction_WaitForThreat,
		eAction_CounterThreat,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// move back
	CJediAiActionConstraintDistance moveBackDistanceConstraint;
	CJediAiActionParallel<2> moveBack;
	struct {
		CJediAiActionDodge dodgeBack;
		CJediAiActionDeflect deflect;
	} moveBackActions;

	// wait for the threat
	CJediAiActionConstraintDistance waitForThreatDistanceConstraint;
	CJediAiActionConstraintThreat waitForThreatBlasterThreatConstraint;
	CJediAiActionConstraintKillTimer waitForThreatKillTimerConstraint;
	CJediAiActionWaitForThreat waitForThreat;

	// counter threat
	CJediAiActionDeflect deflectAttack;

	// construction
	CJediAiActionBlasterCounterAttack();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual void simulate(CJediAiMemory &simMemory);

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);
};


/////////////////////////////////////////////////////////////////////////////
//
// choose an appropriate action against trandoshan infantry
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionEngageTrandoshanInfantry : public CJediAiActionRandomBase {
public:
	typedef CJediAiActionRandomBase BASECLASS;

	// action table
	enum {
		eAction_ForceTkAttack,
		eAction_DeflectAttack,
		eAction_SaberAttack,
		eAction_SaberKickAttack,
		eAction_SpecialAttack,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// odds table
	float actionOddsTable[eAction_Count];

	// force tk attack action sequence
	CJediAiActionSequence<2> forceTkAttack;
	struct {
		CJediAiActionMove move;
		CJediAiActionForceTk forceTk;
	} forceTkAttackActions;

	// deflect attack
	CJediAiActionBlasterCounterAttack deflectAttack;

	// melee attack action sequence
	CJediAiActionSequence<2> saberAttack;
	struct {
		CJediAiActionMove move;
		CJediAiActionSwingSaber saber;
	} saberAttackActions;

	// saber kick combo
	CJediAiActionSequence<3> saberKickCombo;
	struct {
		CJediAiActionMove move;
		CJediAiActionSwingSaber saber;
		CJediAiActionKick kick;
	} saberKickComboActions;

	// special attacks
	CJediAiActionRandom<4> specialAttack;
	struct {

		// rush counter attack
		CJediAiActionSequence<2> rushCounterAttack;
		struct {
			CJediAiActionTaunt taunt;
			CJediAiActionRandom<4> counter;
			struct {
				CJediAiActionSequence<2> kickCounter;
				struct {
					CJediAiActionWaitForThreat wait;
					CJediAiActionKick kick;
				} kickCounterActions;
				CJediAiActionSequence<2> forcePushCounter;
				struct {
					CJediAiActionWaitForThreat wait;
					CJediAiActionForcePush forcePush;
				} forcePushCounterActions;
				CJediAiActionSequence<2> swingSaberCounter;
				struct {
					CJediAiActionWaitForThreat wait;
					CJediAiActionFakeSim fakeSim;
					CJediAiActionSwingSaber swingSaber;
				} swingSaberCounterActions;
				CJediAiActionSequence<2> dodgeCounter;
				struct {
					CJediAiActionWaitForThreat wait;
					CJediAiActionDodge dodge;
				} dodgeCounterActions;
			} counterActions;
		} rushCounterAttackActions;

		// force push
		CJediAiActionForcePush forcePush;

		// jump attack
		CJediAiActionJumpForward jumpAttack;

		// throw object at victim
		CJediAiActionForceTk throwObject;

	} specialAttackActions;

	// construction
	CJediAiActionEngageTrandoshanInfantry();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);

	// CJediAiActionRandomBase methods
	virtual float *getActionOddsTable(int *actionCount);
};


/////////////////////////////////////////////////////////////////////////////
//
// counter a melee attack
// 1) move into melee range
// 2) wait in defensive stance for a melee attack
// 3) counter the melee attack by:
//    a) block, crouch, or dodge left/right, then attacking with the saber, or
//    b) dodge back, then jump attack
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionMeleeCounterAttack : public CJediAiActionSequenceBase {
public:
	typedef CJediAiActionSequenceBase BASECLASS;

	// action table
	enum {
		eAction_MoveToMeleeRange,
		eAction_WaitForThreat,
		eAction_CounterThreat,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// move to melee range
	CJediAiActionConstraintThreat moveToMeleeRangeThreatConstraint;
	CJediAiActionMove moveToMeleeRange;

	// wait for threat
	CJediAiActionWaitForThreat waitForThreat;

	// counter threat
	CJediAiActionRandom<7> counterThreat;
	struct {

		// block attack
		CJediAiActionSequence<2> blockAttack;
		struct {
			CJediAiActionBlock block;
			CJediAiActionFakeSim saberFakeSim;
			CJediAiActionSwingSaber saber;
		} blockAttackActions;

		// passive block
		CJediAiActionBlock passiveBlock;

		// dodge attacks
		CJediAiActionDodge dodgeAttackLeft;
		CJediAiActionDodge dodgeAttackRight;

		// crouch attacks
		CJediAiActionCrouch crouchAttack;

		// jump over attack
		CJediAiActionJumpOver jumpOverAttack;

		// kick + saber combo
		CJediAiActionSequence<2> kickSaberCombo;
		struct {
			CJediAiActionKick kick;
			CJediAiActionSwingSaber saber;
		} kickSaberComboActions;

	} counterThreatActions;

	// construction
	CJediAiActionMeleeCounterAttack();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual void simulate(CJediAiMemory &simMemory);

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);
};


/////////////////////////////////////////////////////////////////////////////
//
// choose an appropriate action against trandoshan melees
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionEngageTrandoshanMelee : public CJediAiActionRandomBase {
public:
	typedef CJediAiActionRandomBase BASECLASS;

	// action table
	enum {
		eAction_CounterAttack,
		eAction_KickFail,
		eAction_SpecialAttack,
		eAction_SwingSaberUntilCountered,
		eAction_SwingSaberOnce,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// odds table
	float actionOddsTable[eAction_Count];

	// counter attack
	CJediAiActionMeleeCounterAttack counterAttack;

	// kick to trigger block
	CJediAiActionConstraintThreat kickFailThreatConstraint;
	CJediAiActionKick kickFail;

	// special attacks
	CJediAiActionRandom<4> specialAttack;
	struct {

		// jump forward attack
		CJediAiActionJumpForward jumpForwardAttack;

		// force push counter
		CJediAiActionConstraintDistance forcePushCounterDistanceConstraint;
		CJediAiActionSequence<3> forcePushCounter;
		struct {

			// force push to cause lunge attack
			CJediAiActionForcePush forcePush;

			// wait for the next rush threat
			CJediAiActionWaitForThreat waitForRushThreat;

			// counter the lunge attack
			CJediAiActionRandom<3> counter;
			struct {

				// kick counter
				CJediAiActionSequence<2> kickCounter;
				struct {
					CJediAiActionWaitForThreat wait;
					CJediAiActionKick kick;
				} kickCounterActions;

				// force push counter
				CJediAiActionSequence<2> forcePushCounter;
				struct {
					CJediAiActionWaitForThreat wait;
					CJediAiActionForcePush forcePush;
				} forcePushCounterActions;

				// fail to swing saber
				CJediAiActionSequence<2> failToSwingSaber;
				struct {
					CJediAiActionWaitForThreat wait;
					CJediAiActionFakeSim ignoreThreat;
					CJediAiActionSwingSaber swingSaber;
				} failToSwingSaberActions;

			} counterActions;

		} forcePushCounterActions;

		// force tk to cause lunge attack, then dodge the resulting counter attack
		CJediAiActionForceTk forceTkDodgeThreat;
		CJediAiActionConstraintThreat forceTkDodgeThreatConstraint;

		// force tk to cause lunge attack, then ignore the resulting counter attack
		CJediAiActionSequence<2> forceTkIgnoreThreat;
		CJediAiActionConstraintThreat forceTkIgnoreThreatConstraint;
		struct {

			// grip the enemy, triggering it's lunge attack
			CJediAiActionForceTk forceTk;
			CJediAiActionConstraintThreat succeedOnLungeThreat;

			// attempt to counter-attack during the lunge, which will fail
			CJediAiActionSwingSaber swingSaber;
			CJediAiActionConstraintThreat requireLungeThreat;
			CJediAiActionFakeSim clearLungeThreat;

		} forceTkIgnoreThreatActions;

	} specialAttackActions;

	// swing saber until countered
	CJediAiActionConstraintThreat swingSaberUntilCounteredThreatConstraint;
	CJediAiActionFakeSim swingSaberUntilCounteredFakeSim;
	CJediAiActionSwingSaber swingSaberUntilCountered;

	// swing saber once
	CJediAiActionConstraintThreat swingSaberOnceThreatConstraint;
	CJediAiActionFakeSim swingSaberOnceFakeSim;
	CJediAiActionSwingSaber swingSaberOnce;

	// construction
	CJediAiActionEngageTrandoshanMelee();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);

	// CJediAiActionRandomBase methods
	virtual float *getActionOddsTable(int *actionCount);
};


/////////////////////////////////////////////////////////////////////////////
//
// choose an appropriate action against trandoshan commandos
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionEngageTrandoshanCommando : public CJediAiActionRandomBase {
public:
	typedef CJediAiActionRandomBase BASECLASS;

	// action table
	enum {
		eAction_SaberAttack,
		eAction_KickForceTkCombo,
		eAction_ForcePushForceTkCombo,
		eAction_ForcePushAttack,
		eAction_SpecialAttack,
		eAction_Ambient,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// odds table
	float actionOddsTable[eAction_Count];

	// saber attack action sequence
	CJediAiActionSequence<2> saberAttack;
	struct {
		CJediAiActionMove move;
		CJediAiActionSwingSaber saber;
	} saberAttackActions;

	// kick + force tk combo
	CJediAiActionSequence<3> kickForceTkCombo;
	struct {
		CJediAiActionMove move;
		CJediAiActionKick kick;
		CJediAiActionForceTk forceTk;
	} kickForceTkComboActions;

	// force push + force tk combo
	CJediAiActionSequence<3> forcePushForceTkCombo;
	struct {
		CJediAiActionMove move;
		CJediAiActionConstraintDistance forcePushDistanceConstraint;
		CJediAiActionForcePush forcePush;
		CJediAiActionForceTk forceTk;
	} forcePushForceTkComboActions;

	// force push
	CJediAiActionForcePush forcePush;

	// special attacks
	CJediAiActionRandom<4> specialAttack;
	struct {

		// force push a grenade
		CJediAiActionConstraintThreat onlyIfGrenadeThreat;
		CJediAiActionForcePush pushGrenade;

		// throw a grenade
		CJediAiActionForceTk throwGrenade;

		// jump forward attack
		CJediAiActionJumpForward jumpAttack;

		// kick-saber combo
		CJediAiActionSequence<3> kickSaberCombo;
		struct {
			CJediAiActionMove move;
			CJediAiActionKick kick;
			CJediAiActionSwingSaber saber;
		} kickSaberComboActions;

	} specialAttackActions;

	// ambient actions
	CJediAiActionSelector<3> ambient;
	struct {

		// dash attack
		CJediAiActionForceTk forceTk;
		CJediAiActionFakeSim forceTkFakeSim;

		// dash attack
		CJediAiActionDash dashAttack;

		// jump over attack
		CJediAiActionJumpOver jumpOverAttack;

	} ambientActions;

	// construction
	CJediAiActionEngageTrandoshanCommando();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);

	// CJediAiActionRandomBase methods
	virtual float *getActionOddsTable(int *actionCount);
};


/////////////////////////////////////////////////////////////////////////////
//
// choose an appropriate action against trandoshan concussives
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionEngageTrandoshanConcussive : public CJediAiActionRandomBase {
public:
	typedef CJediAiActionRandomBase BASECLASS;

	// action table
	enum {
		eAction_SaberAttack,
		eAction_JumpForwardAttack,
		eAction_SpecialAttack,
		eAction_ForceTkAttack,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// odds table
	float actionOddsTable[eAction_Count];

	// saber attack action sequence
	CJediAiActionSequence<2> saberAttack;
	struct {
		CJediAiActionMove move;
		CJediAiActionSelectorBinary saberSelector;
		struct {
			CJediAiActionSwingSaber highSkillSaber;
			CJediAiActionSwingSaber lowSkillSaber;
			CJediAiActionFakeSim lowSkillSaberFakeSim;
		} saberSelectorActions;
	} saberAttackActions;

	// jump forward attack
	CJediAiActionJumpForward jumpForwardAttack;

	// special attacks
	CJediAiActionRandom<4> specialAttack;
	struct {

		// saber-kick combo
		CJediAiActionSequence<3> kickSaberCombo;
		struct {
			CJediAiActionMove move;
			CJediAiActionKick kick;
			CJediAiActionSwingSaber saber;
		} kickSaberComboActions;

		// force push
		CJediAiActionForcePush forcePush;

		// throw object
		CJediAiActionForceTk throwObject;

		// jump over attack
		CJediAiActionJumpOver jumpOverAttack;

	} specialAttackActions;

	// force tk attack action sequence
	CJediAiActionConstraintThreat threatConstraint;
	CJediAiActionSequence<2> forceTkAttack;
	struct {

		// move into range
		CJediAiActionMove move;

		// grip the victim (we can't throw him)
		CJediAiActionRandom<2> grip;
		struct {

			// when he counters with a concussion blast, let him go and dodge out of the way
			CJediAiActionForceTk bailOnThreat;

			// when he counters with a concussion blast, ignore it and take damage
			CJediAiActionFakeSim ignoreThreat;
			CJediAiActionForceTk takeDamage;

		} gripActions;

	} forceTkAttackActions;

	// construction
	CJediAiActionEngageTrandoshanConcussive();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);

	// CJediAiActionRandomBase methods
	virtual float *getActionOddsTable(int *actionCount);
};


/////////////////////////////////////////////////////////////////////////////
//
// choose an appropriate action against trandoshan flutterpacks
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionEngageTrandoshanFlutterpack : public CJediAiActionRandomBase {
public:
	typedef CJediAiActionSelectorBase BASECLASS;

	// action table
	enum {
		eAction_EngageFlying,
		eAction_EngageGround,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// odds table
	float actionOddsTable[eAction_Count];

	// actions
	CJediAiActionConstraintVictimCombatType engageFlyingConstraint;
	CJediAiActionRandom<4> engageFlying;
	struct {

		// move into tk range
		CJediAiActionMove moveCloser;

		// force tk kill
		CJediAiActionForceTk forceTkKill;

		// throw object
		CJediAiActionForceTk throwObject;

		// force tk fail
		CJediAiActionForceTk forceTkFail;

	} engageFlyingActions;

	// ground actions
	CJediAiActionConstraintVictimCombatType engageGroundConstraint;
	CJediAiActionSequence<2> engageGround;
	struct {

		// wait for threat
		CJediAiActionWaitForThreat waitForThreat;

		// counter actions
		CJediAiActionRandom<3> counter;
		struct {

			// kick
			CJediAiActionSequence<2> kickCounter;
			struct {
				CJediAiActionWaitForThreat wait;
				CJediAiActionKick kick;
			} kickCounterActions;

			// force push
			CJediAiActionSequence<2> forcePushCounter;
			struct {
				CJediAiActionWaitForThreat wait;
				CJediAiActionForcePush forcePush;
			} forcePushCounterActions;

			// dodge
			CJediAiActionSequence<2> dodgeCounter;
			struct {
				CJediAiActionWaitForThreat wait;
				CJediAiActionDodge dodge;
			} dodgeCounterActions;

		} counterActions;

	} engageGroundActions;

	// construction
	CJediAiActionEngageTrandoshanFlutterpack();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);

	// CJediAiActionRandomBase methods
	virtual float *getActionOddsTable(int *actionCount);
};


/////////////////////////////////////////////////////////////////////////////
//
// choose an appropriate action against b1 battle droids
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionEngageB1BattleDroid : public CJediAiActionRandomBase {
public:
	typedef CJediAiActionRandomBase BASECLASS;

	// action table
	enum {
		eAction_ForceTkAttack,
		eAction_ForcePushAttack,
		eAction_DeflectAttack,
		eAction_SaberAttack,
		eAction_KickAttack,
		eAction_SpecialAttack,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// odds table
	float actionOddsTable[eAction_Count];

	// force tk attack
	CJediAiActionForceTk forceTk;

	// force push attack
	CJediAiActionForcePush forcePush;

	// deflect attack
	CJediAiActionBlasterCounterAttack deflectAttack;

	// saber attack action sequence
	CJediAiActionSequence<2> saberAttack;
	struct {
		CJediAiActionMove move;
		CJediAiActionSwingSaber saber;
	} saberAttackActions;

	// kick attack action sequence
	CJediAiActionKick kick;

	// special attacks
	CJediAiActionRandom<5> specialAttack;
	struct {

		// dash attack
		CJediAiActionDash dashAttack;

		// jump attack
		CJediAiActionJumpForward jumpAttack;

		// throw object at victim
		CJediAiActionForceTk throwObject;

		// force push a grenade
		CJediAiActionConstraintThreat onlyIfGrenadeThreat;
		CJediAiActionForcePush pushGrenade;

		// throw a grenade
		CJediAiActionForceTk throwGrenade;

	} specialAttackActions;

	// construction
	CJediAiActionEngageB1BattleDroid();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);

	// CJediAiActionRandomBase methods
	virtual float *getActionOddsTable(int *actionCount);
};


/////////////////////////////////////////////////////////////////////////////
//
// choose an appropriate action against b1 melee droids
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionEngageB1MeleeDroid : public CJediAiActionRandomBase {
public:
	typedef CJediAiActionRandomBase BASECLASS;

	// action table
	enum {
		eAction_CounterAttack,
		eAction_SpecialAttack,
		eAction_AmbientActions,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// odds table
	float actionOddsTable[eAction_Count];

	// counter attack
	CJediAiActionMeleeCounterAttack counterAttack;

	// special attacks
	CJediAiActionRandom<4> specialAttack;
	struct {

		// kick saber combo
		CJediAiActionSequence<2> kickSaberCombo;
		struct {
			CJediAiActionKick kick;
			CJediAiActionSwingSaber swingSaber;
		} kickSaberActions;

		// jump forward attack
		CJediAiActionJumpForward jumpAttack;

		// kick force push combo
		CJediAiActionSequence<2> kickForcePushCombo;
		struct {
			CJediAiActionKick kick;
			CJediAiActionForcePush forcePush;
		} kickForcePushActions;

		// kick force tk combo
		CJediAiActionSequence<2> kickForceTkCombo;
		struct {
			CJediAiActionKick kick;
			CJediAiActionForceTk forceTk;
		} kickForceTkActions;

	} specialAttackActions;

	// ambient actions
	CJediAiActionRandom<3> ambient;
	struct {

		// swing saber once
		CJediAiActionFakeSim swingSaberOnceFakeSim;
		CJediAiActionSwingSaber swingSaberOnce;

		// force tk to cause counter attack, then ignore the resulting counter attack
		CJediAiActionForceTk forceTkIgnoreThreat;
		CJediAiActionFakeSim forceTkIgnoreThreatFakeSim;
		CJediAiActionConstraintThreat forceTkIgnoreThreatConstraint;

		// force tk to cause counter attack, then dodge the resulting counter attack
		CJediAiActionForceTk forceTkDodgeThreat;
		CJediAiActionFakeSim forceTkDodgeThreatFakeSim;
		CJediAiActionConstraintThreat forceTkDodgeThreatConstraint;

	} ambientActions;

	// construction
	CJediAiActionEngageB1MeleeDroid();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);

	// CJediAiActionRandomBase methods
	virtual float *getActionOddsTable(int *actionCount);
};


/////////////////////////////////////////////////////////////////////////////
//
// choose an appropriate action against b1 jetpack droids
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionEngageB1JetpackDroid : public CJediAiActionRandomBase {
public:
	typedef CJediAiActionRandomBase BASECLASS;

	// action table
	enum {
		eAction_ForceTkAttack,
		eAction_ForcePushAttack,
		eAction_DeflectAttack,
		eAction_ThrowObject,
		eAction_Ambient,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// odds table
	float actionOddsTable[eAction_Count];

	// force tk kill
	CJediAiActionForceTk forceTkKill;

	// force push attack
	CJediAiActionForcePush forcePush;

	// deflect attack
	CJediAiActionBlasterCounterAttack deflectAttack;

	// throw object at victim
	CJediAiActionForceTk throwObject;

	// ambient actions
	CJediAiActionRandom<2> ambient;
	struct {

		// dash to close range such that the droid must fly to a new spot
		CJediAiActionFakeSim dashToCloseRangeFakeSim;
		CJediAiActionDash dashToCloseRange;

		// force tk fail
		CJediAiActionForceTk forceTkFail;

	} ambientActions;

	// construction
	CJediAiActionEngageB1JetpackDroid();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);

	// CJediAiActionRandomBase methods
	virtual float *getActionOddsTable(int *actionCount);
};


/////////////////////////////////////////////////////////////////////////////
//
// choose an appropriate action against b2 battle droids
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionEngageB2BattleDroid : public CJediAiActionRandomBase {
public:
	typedef CJediAiActionRandomBase BASECLASS;

	// action table
	enum {
		eAction_DeflectAttack,
		eAction_ForcePushRocket,
		eAction_SaberAttack,
		eAction_ForcePushSaber,
		eAction_SaberDuringRush,
		eAction_CrouchAttack,
		eAction_SpecialAttack,
		eAction_Ambient,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// odds table
	float actionOddsTable[eAction_Count];

	// deflect attack
	CJediAiActionDeflect deflectAttack;

	// force push rocket
	CJediAiActionConstraintThreat forcePushRocketThreatConstraint;
	CJediAiActionSequence<2> forcePushRocket;
	struct {
		CJediAiActionWaitForThreat waitForThreat;
		CJediAiActionForcePush forcePush;
	} forcePushRocketActions;

	// saber attack action sequence
	CJediAiActionSequence<2> saberAttack;
	struct {
		CJediAiActionMove move;
		CJediAiActionSwingSaber saber;
	} saberAttackActions;

	// force push + saber combo
	CJediAiActionSequence<3> forcePushSaberCombo;
	struct {
		CJediAiActionMove move;
		CJediAiActionForcePush forcePush;
		CJediAiActionSwingSaber saber;
	} forcePushSaberComboActions;

	// saber during rush
	CJediAiActionConstraintThreat saberDuringRushThreatConstraint;
	CJediAiActionSequence<2> saberDuringRush;
	struct {
		CJediAiActionWaitForThreat waitForThreat;
		CJediAiActionFakeSim fakeSim;
		CJediAiActionSwingSaber swingSaber;
	} saberDuringRushActions;

	// crouch
	CJediAiActionCrouch crouchAttack;

	// special attacks
	CJediAiActionRandom<4> specialAttack;
	struct {

		// dash attack
		CJediAiActionDash dashAttack;

		// jump forward attack
		CJediAiActionJumpForward jumpForwardAttack;

		// jump over attack
		CJediAiActionJumpOver jumpOverAttack;

		// throw object at victim
		CJediAiActionConstraintThreat throwObjectThreatConstraint;
		CJediAiActionForceTk throwObject;

	} specialAttackActions;

	// ambient actions
	CJediAiActionRandom<3> ambient;
	struct {

		// force tk
		CJediAiActionConstraintThreat forceTkThreatConstraint;
		CJediAiActionFakeSim forceTkFakeSimResult;
		CJediAiActionFakeSim forceTkFakeSimThreats;
		CJediAiActionForceTk forceTk;

		// jump over rush
		CJediAiActionConstraintThreat jumpOverRushThreatConstraint;
		CJediAiActionSequence<2> jumpOverRush;
		struct {
			CJediAiActionWaitForThreat waitForThreat;
			CJediAiActionJumpOver jumpOver;
		} jumpOverRushActions;

		// jump over rocket
		CJediAiActionConstraintThreat jumpOverRocketConstraint;
		CJediAiActionJumpForward jumpOverRocket;

	} ambientActions;

	// construction
	CJediAiActionEngageB2BattleDroid();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);

	// CJediAiActionRandomBase methods
	virtual float *getActionOddsTable(int *actionCount);
};


/////////////////////////////////////////////////////////////////////////////
//
// choose an appropriate action against droideka
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionEngageDroideka : public CJediAiActionRandomBase {
public:
	typedef CJediAiActionRandomBase BASECLASS;

	// action table
	enum {
		eAction_DeflectAttack,
		eAction_JumpOverAttack,
		eAction_JumpForwardAttack,
		eAction_ForcePush,
		eAction_Kick,
		eAction_SaberAttack,
		eAction_ThrowObject,
		eAction_ForceTk,
		eAction_DodgeDash,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// odds table
	float actionOddsTable[eAction_Count];

	// deflect attack
	CJediAiActionBlasterCounterAttack deflectAttack;

	// jump over attack
	CJediAiActionJumpOver jumpOverAttack;

	// jump forward attack
	CJediAiActionJumpForward jumpForwardAttack;

	// force push
	CJediAiActionSequence<2> forcePush;
	struct {
		CJediAiActionMove move;
		CJediAiActionForcePush forcePush;
	} forcePushActions;

	// kick
	CJediAiActionSequence<2> kick;
	struct {
		CJediAiActionMove move;
		CJediAiActionKick kick;
	} kickActions;

	// saber attack action sequence
	CJediAiActionSequence<2> saberAttack;
	struct {
		CJediAiActionMove move;
		CJediAiActionSwingSaber saber;
		CJediAiActionFakeSim saberFakeSim;
	} saberAttackActions;

	// throw object at victim
	CJediAiActionForceTk throwObject;

	// force tk
	CJediAiActionSelectorBinary forceTk;
	struct {

		// condition
		CJediAiActionConstraintFlags condition;

		// forceTk while shield is down (high skill)
		CJediAiActionForceTk forceTkNoShield;

		// forceTk while shield is up (low skill)
		CJediAiActionForceTk forceTkShield;
		CJediAiActionFakeSim forceTkShieldFakeSim;
		CJediAiActionConstraintThreat forceTkShieldThreatConstraint;

	} forceTkActions;

	// jump over
	CJediAiActionConstraintThreat jumpOverThreatConstraint;
	CJediAiActionJumpOver jumpOver;

	// dodge + dash
	CJediAiActionConstraintThreat dodgeDashThreatConstraint;
	CJediAiActionSequence<2> dodgeDash;
	struct {
		CJediAiActionDodge dodge;
		CJediAiActionDash dash;
		CJediAiActionFakeSim dashFakeSim;
	} dodgeDashActions;

	// construction
	CJediAiActionEngageDroideka();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);

	// CJediAiActionRandomBase methods
	virtual float *getActionOddsTable(int *actionCount);
};


/////////////////////////////////////////////////////////////////////////////
//
// choose an appropriate way to engage a particular type of enemy
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionEngage : public CJediAiActionSelectorBase {
public:
	typedef CJediAiActionSelectorBase BASECLASS;

	// action table
	enum EAction {
		eAction_EngageTrandoshanInfantry,
		eAction_EngageTrandoshanMelee,
		eAction_EngageTrandoshanCommando,
		eAction_EngageTrandoshanConcussive,
		eAction_EngageTrandoshanFlutterpack,
		eAction_EngageB1BattleDroid,
		eAction_EngageB1MeleeDroid,
		eAction_EngageB1JetpackDroid,
		eAction_EngageB2BattleDroid,
		eAction_EngageDroideka,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// actions
	CJediAiActionEngageTrandoshanInfantry engageTrandoshanInfantry;
	CJediAiActionEngageTrandoshanMelee engageTrandoshanMelee;
	CJediAiActionEngageTrandoshanCommando engageTrandoshanCommando;
	CJediAiActionEngageTrandoshanConcussive engageTrandoshanConcussive;
	CJediAiActionEngageTrandoshanFlutterpack engageTrandoshanFlutterpack;
	CJediAiActionEngageB1BattleDroid engageB1BattleDroid;
	CJediAiActionEngageB1MeleeDroid engageB1MeleeDroid;
	CJediAiActionEngageB1JetpackDroid engageB1JetpackDroid;
	CJediAiActionEngageB2BattleDroid engageB2BattleDroid;
	CJediAiActionEngageDroideka engageDroideka;

	// construction
	CJediAiActionEngage();
	virtual ~CJediAiActionEngage();

	// CJediAiAction methods
	virtual void init(CJediAiMemory *memory);
	virtual EJediAiAction getType() const;
	virtual bool isNotSelectable() const;

	// CJediAiActionComposite methods
	virtual CJediAiAction **getActionTable(int *actionCount);

	// CJediAiActionSelector methods
	virtual CJediAiAction *selectAction(CJediAiMemory *simMemory) const;
	virtual EJediAiActionResult setCurrentAction(CJediAiAction *action);

	// get the action for a given victim
	static EAction getActionForVictim(const SJediAiActorState &victimState);
};


/////////////////////////////////////////////////////////////////////////////
//
// combat
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionCombat : public CJediAiActionSelectorBase {
public:
	typedef CJediAiActionSelectorBase BASECLASS;

	// action table
	enum {
		eAction_GiveOtherJediSpace,
		eAction_Engage,
		eAction_Defend,
		eAction_Idle,
		eAction_Count
	};
	CJediAiAction *actionTable[eAction_Count];

	// give other jedi space
	CJediAiActionSelector<4> giveOtherJediSpace;
	CJediAiActionConstraintSelfIsTooCloseToOtherJedi tooCloseToOtherJediConstraint;
	struct {

		// if we are engaged with a melee enemy, try to jump over first
		CJediAiActionConstraintVictimEnemyType meleeEnemyTypeConstraint;
		CJediAiActionJumpOver meleeJumpOver;

		// try to dodge laterally
		CJediAiActionRandom<2> dodgeLateral;
		struct {
			CJediAiActionDodge dodgeLeft;
			CJediAiActionDodge dodgeRight;
		} dodgeLateralActions;

		// try to dodge backward
		CJediAiActionDodge dodgeBack;

		// if we are engaged with a melee enemy, try to jump over first
		CJediAiActionConstraintVictimEnemyType nonMeleeEnemyTypeConstraint;
		CJediAiActionJumpOver nonMeleeJumpOver;

	} giveOtherJediSpaceActions;

	// engage
	CJediAiActionConstraintSelfIsTooCloseToOtherJedi notTooCloseToOtherJediConstraint;
	CJediAiActionEngage engage;

	// defend
	CJediAiActionDefend defend;

	// idle
	CJediAiActionIdle idle;

	// construction
	CJediAiActionCombat();
	virtual ~CJediAiActionCombat();

	// CJediAiAction methods
	virtual EJediAiAction getType() const;
	virtual void reset();
	virtual EJediAiActionResult checkConstraints(const CJediAiMemory &simMemory, bool simulating) const;
	virtual EJediAiActionResult onBegin();
	virtual void onEnd();
	virtual void updateTimers(float dt);
	virtual EJediAiActionResult update(float dt);

	// CJediAiActionSelector methods
	virtual CJediAiAction **getActionTable(int *actionCount);
};

#endif // __JEDI_AI_ACTIONS__
