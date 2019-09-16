#ifndef __JEDI_AI_MEMORY__
#define __JEDI_AI_MEMORY__

#ifndef __JEDI_COMMON__
	#include "jedi_common.h"
#endif


/////////////////////////////////////////////////////////////////////////////
//
// jedi world state knowledge container
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiMemory {
public:

	//---------------------------------
	// maintenance
	//---------------------------------

	// construction
	CJediAiMemory();

	// copy construction
	CJediAiMemory(const CJediAiMemory &copyMe);

	// reset world state information
	void reset();

	// copy world state information
	void copy(const CJediAiMemory &copyMe);

	// setup this memory for the specified jedi
	void setup(CJedi *jedi);

	// update world state
	void update(float dt);

	// the current 'time' (polled each frame from 'getTime()')
	float currentTime;


	//---------------------------------
	// simulation
	//---------------------------------

	// simulation parameters
	struct SSimulateParams {
		SSimulateParams() { memset(this, 0, sizeof(SSimulateParams)); }

		// positional data
		CVector *wSelfPos;
		CVector *iSelfFrontDir;

		// defense
		EJediBlockDir blockDir;
	};

	// keep track of how long we've simulated so far
	float simulationDuration;

	// did any actors die during the current sim step?
	int numActorsDiedDuringSimulation;

	// did any actors engaged with another jedi die during the simulation?
	int numActorsEngagedWithOtherJediDiedDuringSimulation;

	// were any actors that a player was targeting disturbed during the simulation?
	bool playerTargetDisturbedDuringSimulation;

	// simulate a set of actions over a given timestep
	void simulate(float dt, const SSimulateParams &params);

	// simulate damage to an actor
	void simulateDamage(float damage, SJediAiActorState &actorState);


	//---------------------------------
	// self state
	//---------------------------------

	// collision check directions
	enum ECollisionDir {
		eCollisionDir_Left,
		eCollisionDir_Right,
		eCollisionDir_Forward,
		eCollisionDir_Backward,
		eCollisionDir_Victim,
		eCollisionDir_Count
	};

	// data about my self's current state
	struct SSelfState {

		// self gameplay data
		CJedi *jedi;
		float skillLevel;
		float hitPoints;
		float collisionRadius;
		float maxJumpForwardDistance;
		int currentStateBitfield;
		int disabledActionBitfield;
		bool isAiControlled;
		bool defensiveModeEnabled;
		bool isTooCloseToAnotherJedi;

		// self positional data
		CVector wPos;
		CVector wPrevPos;
		CVector wBoundsCenterPos;
		CVector iFrontDir;
		CVector iRightDir;

		// self damage data
		float saberDamage;
		float kickDamage;
		float forcePushMaxDist;
		float forcePushDamageRadius;
		float forcePushDamage;

		// collision checks
		struct { CActor *actor; float distance; } nearestCollisionTable[eCollisionDir_Count];
		int nearestCollisionCheckIndex;

	} selfState;

	// self state query
	void querySelfState();

	// query self collisions
	void querySelfCollisions();

	// is my self in a given state?
	bool isSelfInState(EJediState state) const;

	// set my self into a given state
	void setSelfInState(EJediState state, bool inState);

	// query disabled actions
	bool canSelfDoAction(EJediAction action) const;


	//---------------------------------
	// generic state updates
	//---------------------------------

	// update an entity state relative to the self state
	void updateEntityToSelfState(SJediAiEntityState &updateMe);

	// fill an actor state with the data from an actor
	void queryActorState(CActor *actor, SJediAiActorState &state);


	//---------------------------------
	// victim state
	//---------------------------------

	// my victim
	CActor *victim;
	bool victimChanged;
	bool victimInView;
	bool victimCanBeNavigatedTo;

	// how long should it take and how long has it taken to kill this victim?
	float victimTimer;
	float victimDesiredKillTime;

	// how high is my victim off the ground?
	float victimFloorHeight;

	// victim state
	SJediAiActorState *victimState;

	// victim state query
	void queryVictimState();

	// update the victim's state relative to the current self state
	void updateVictimToSelfState();


	//---------------------------------
	// force tk actors
	//---------------------------------

	// current force tk target state
	SJediAiActorState *forceTkTargetState;

	// best potential force tk target
	// if we have a valid force tk target, this will be it as well
	SJediAiActorState *forceTkBestTargetState;

	// best force tk throw target given our best force tk target
	SJediAiActorState *forceTkBestThrowTargetState;

	// force tk target state query
	void queryForceTkTargetStates();
	void queryForceTkBestTargetState();
	void queryForceTkBestThrowTargetState();

	// update the force tk target's state relative to the current self state
	void updateForceTkTargetToSelfStates();


	//---------------------------------
	// nearby actors of interest to my self
	//---------------------------------

	// partner jedi state list
	enum { kPartnerJediStateListSize = 2 };
	int partnerJediStateCount;
	SJediAiActorState partnerJediStates[kPartnerJediStateListSize];

	// enemy state list
	enum { kEnemyStateListSize = 8 };
	int enemyStateCount;
	SJediAiActorState enemyStates[kEnemyStateListSize];

	// find an enemy state
	int findEnemyStateIndex(CActor *enemy) const;
	SJediAiActorState *findEnemyState(CActor *enemy);

	// force tk object states
	enum { kForceTkObjectStateListSize = 8 };
	int forceTkObjectStateCount;
	SJediAiActorState forceTkObjectStates[kForceTkObjectStateListSize];

	// each frame, I check whether or not the forceTkObjectState at this
	// index can hit my victim if thrown
	int canForceTkObjectHitVictimCheckIndex;

	// clear whether or not forceTkObjects can hit my victim
	void clearCanForceTkObjectsHitVictim();

	// update whether or not forceTkObjects can hit my victim
	void updateCanForceTkObjectsHitVictim();

	// find a force tk object state
	int findForceTkObjectStateIndex(CActor *object) const;
	SJediAiActorState *findForceTkObjectState(CActor *object);

	// actor state query
	void queryActorStates();

	// update the 'throwable' flag for a given actor state
	void updateActorStateThrowableFlag(SJediAiActorState &actorState) const;

	// update the actor states relative to the current self state
	void updateActorToSelfStates();

	// simulate our actors
	void simulateActor(float dt, const SSimulateParams &params, SJediAiActorState &actorState);
	void simulateActors(float dt, const SSimulateParams &params);


	//---------------------------------
	// nearby threats
	//---------------------------------

	// aggregate of all threatLevels (0 to 1)
	float threatLevel;

	// threat state list
	enum { kThreatStateListSize = 8 };
	int threatStateCount;
	SJediAiThreatState threatStates[kThreatStateListSize];

	// aggregate data for each threat type
	struct SJediThreatTypeData {
		int count;
		const SJediAiThreatState *shortestDurationThreat;
		const SJediAiThreatState *shortestDistanceThreat;
	};
	SJediThreatTypeData threatTypeDataTable[eJediThreatType_Count];

	// if I block in a given direction, how much threat do I eliminate?
	float highestBlockableThreatLevel;
	struct { float threatLevel, maxDuration; } blockDirThreatInfoTable[eJediBlockDir_Count];

	// if I dodge in a given direction, how much threat do I eliminate?
	float highestDodgeableThreatLevel;
	float dodgeDirThreatLevelTable[eJediDodgeDir_Count];

	// how long do I need to deflect to handle all incoming blaster threats?
	float recommendedDeflectionDuration;

	// how long do I need to crouch to handle all incoming horizontal threats?
	float recommendedCrouchDuration;

	// how long do I have until the next tackle threat damages me?
	float nextRushThreatDuration;

	// threat state query
	void queryThreatStates();

	// simulate threats
	void simulateThreats(float dt, const SSimulateParams &params);

	// update an entity state relative to the self state
	void updateThreatToSelfState(SJediAiThreatState &updateMe);

	// compute a threat state's threat level
	float computeThreatStateLevel(SJediAiThreatState &state);
};

#endif // __JEDI_AI_MEMORY__
