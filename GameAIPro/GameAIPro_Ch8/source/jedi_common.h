#ifndef __JEDI_COMMON__
#define __JEDI_COMMON__

#ifndef __VECTOR__
	#include "vector.h"
#endif

#ifndef __MATH__
	#include "math.h"
#endif


/////////////////////////////////////////////////////////////////////////////
//
// jedi globals and constants
//
/////////////////////////////////////////////////////////////////////////////

#pragma region forward decls

// forward decls
class CActor;
class CJedi;
class CJediAiMemory;
class CJediAiAction;
struct SJediAiActorState;
struct SJediAiThreatState;
struct SJediAiActionSimSummary;

#pragma endregion

#pragma region jedi physical constants (pulled from animation data)

// jedi constants
extern float kJediMoveSpeedMin;
extern float kJediMoveSpeedMax;
extern float kJediStrafeSpeed;
extern float kJediSwingSaberDistance;
extern float kJediSwingSaberEnterDuration;
extern float kJediSwingSaberDuration;
extern float kJediSwingSaberExitDuration;
extern float kJediDodgeDuration;
extern float kJediDodgeDistance;
extern float kJediDodgeFlipDuration;
extern float kJediDodgeFlipDistance;
extern float kJediDashEnterDuration;
extern float kJediDashEnterDistance;
extern float kJediDashExitDuration;
extern float kJediDashExitDistance;
extern float kJediDashAttackDuration;
extern float kJediDashAttackDistance;
extern float kJediForcePushEnterDuration;
extern float kJediForcePushExitDuration;
extern float kJediForceTkOneHandEnterDuration;
extern float kJediForceTkOneHandExitDuration;
extern float kJediForceTkTwoHandEnterDuration;
extern float kJediForceTkTwoHandExitDuration;
extern float kJediJumpOverDuration;
extern float kJediJumpOverCrouchDuration;
extern float kJediJumpOverAttackDuration;
extern float kJediJumpForwardEnterDuration;
extern float kJediJumpForwardEnterInFlightDuration;
extern float kJediJumpForwardExitDuration;
extern float kJediJumpForwardExitInFlightDuration;
extern float kJediJumpForwardAttackEnterDuration;
extern float kJediJumpForwardAttackVerticalExitDuration;
extern float kJediJumpForwardAttackHorizontalExitDuration;
extern float kJediJumpOntoDuration;
extern float kJediCrouchEnterDuration;
extern float kJediCrouchAttackDuration;
extern float kJediKickDistance;
extern float kJediKickDuration;
extern float kJediDeflectOnceDuration;
extern float kJediDeflectionEnterDuration;
extern float kJediDeflectionExitDuration;
extern float kJediTauntDuration;
extern float kJediAiDashActivationDistanceMin;
extern float kJediAiForceTkActivationDistanceMin;
extern float kJediAiForceTkFacePctMin;

#pragma endregion

#pragma region jedi gameplay constants

// jedi tweakable globals
extern float kJediMeleeCorrectionTweak;
extern float kJediDashSpeed;
extern float kJediForwardJumpSpeed;
extern float kJediSaberEffectSlamVicinityRadius;
extern float kJediSaberEffectSlashVicinityRadius;
extern float kJediSaberEffectMinConeLength;
extern float kJediSaberEffectConeRadius;
extern float kJediSaberEffectDamageStrength;
extern float kJediCombatMaxJumpDistance;
extern float kJediForwardJumpMaxDistance;
extern float kJediForceSelectRange;
extern float kJediThrowRange;
extern float kJediForcePushTier2Min;
extern float kJediForcePushTier3Min;
extern float kJediMeleeCorrectionTweak;
extern float kJediForceSelectRange;
extern float kJediThrowRange;
extern float kJediTKThrowSpeed;

#pragma endregion


/////////////////////////////////////////////////////////////////////////////
//
// engine system stubs
//
/////////////////////////////////////////////////////////////////////////////

#pragma region determinePathFindValidity

// is there a valid path between two points?
inline bool determinePathFindValidity(const CVector &a, const CVector &b) {
	return true; // assume we can get there for the example
}

#pragma endregion

#pragma region navMeshCollideRay

inline bool navMeshCollideRay(const CVector &wStartPos, const CVector &wTargetPos, CVector *wClosestNavigablePos) {
	if (wClosestNavigablePos != NULL)
		wClosestNavigablePos->zero();
	return false; // assume that we hit nothing for the example
}

#pragma endregion

#pragma region collideWorldWithMovingSphere

// perform a world collision test against the specified moving sphere
inline bool collideWorldWithMovingSphere(CVector wPos, float radius, CVector iDelta, CVector *wCollisionPos, CActor **collisionActor = NULL) {
	if (wCollisionPos != NULL)
		wCollisionPos->zero();
	if (collisionActor != NULL)
		*collisionActor = NULL;
	return false; // assume that we hit nothing for the example
}

#pragma endregion

#pragma region findActorsInVicinity

// find all actors in the vicinity of the specified position
inline int findActorsInVicinity(const CVector &wPos, float radius, CActor *actorList[], int actorListSize, bool(*callback)(CActor*,void*), void *context) {
	if (actorList != NULL)
		memset(actorList, 0, sizeof(CActor*) * actorListSize);
	return 0; // stubbed for this example
}

#pragma endregion

#pragma region EAttackLevel

enum EAttackLevel {
	eAttackLevel_Light = 0,
	eAttackLevel_Medium,
	eAttackLevel_Heavy,
	eAttackLevel_Count
};

#pragma endregion

#pragma region Gravity

// gravity constant
inline float getGravity() {
	return 9.8f;
}

#pragma endregion


/////////////////////////////////////////////////////////////////////////////
//
// random number utils
//
/////////////////////////////////////////////////////////////////////////////

#pragma region fRand

extern float fRand(float rangeMin, float rangeMax);

#pragma endregion

#pragma region randBool

extern bool randBool(float odds);

#pragma endregion

#pragma region randChoice

extern int randChoice(int oddsTableSize, float oddsTable[]);

#pragma endregion


/////////////////////////////////////////////////////////////////////////////
//
// jedi types and enumerations
//
/////////////////////////////////////////////////////////////////////////////

#pragma region jedi actions

// jedi actions
enum EJediAction {
	eJediAction_WalkRun,
	eJediAction_SwingSaber,
	eJediAction_Crouch,
	eJediAction_Dodge,
	eJediAction_Dash,
	eJediAction_Jump,
	eJediAction_ForcePush,
	eJediAction_ForceTk,
	eJediAction_Kick,
	eJediAction_Block,
	eJediAction_Deflect,
	eJediAction_Taunt,
	eJediAction_TankAttack,
	eJediAction_Count
};

// lookup a jedi action name
// returns '<unknown>' on failure, not NULL
extern const char *lookupJediActionName(EJediAction action);

#pragma endregion

#pragma region jedi state

// jedi state
enum EJediState {
	eJediState_SwingingSaber,
	eJediState_Crouching,
	eJediState_Dodging,
	eJediState_Dashing,
	eJediState_Jumping,
	eJediState_Flipping,
	eJediState_ForcePushing,
	eJediState_UsingForceTk,
	eJediState_ForceTkAllowed,
	eJediState_Kicking,
	eJediState_Blocking,
	eJediState_KnockedAround,
	eJediState_Deflecting,
	eJediState_Reflecting,
	eJediState_AiBlocked,
	eJediState_Turning,
	eJediState_DodgingLeft,
	eJediState_DodgingRight,
	eJediState_Count
};

// lookup a jedi state name
// returns '<unknown>' on failure, not NULL
extern const char *lookupJediStateName(EJediState state);

#pragma endregion

#pragma region jedi combat type

// jedi combat type
enum EJediCombatType {
	eJediCombatType_Unknown = 0,
	eJediCombatType_Infantry,
	eJediCombatType_Brawler,
	eJediCombatType_Concussive,
	eJediCombatType_AirUnit,
	eJediCombatType_Count
};

#pragma endregion

#pragma region jedi enemy type

// jedi enemy type
enum EJediEnemyType {
	eJediEnemyType_Unknown = 0,
	eJediEnemyType_B1BattleDroid,
	eJediEnemyType_B1GrenadeDroid,
	eJediEnemyType_B1MeleeDroid,
	eJediEnemyType_B1JetpackDroid,
	eJediEnemyType_B2BattleDroid,
	eJediEnemyType_B2RocketDroid,
	eJediEnemyType_Droideka,
	eJediEnemyType_TrandoshanInfantry,
	eJediEnemyType_TrandoshanMelee,
	eJediEnemyType_TrandoshanConcussive,
	eJediEnemyType_TrandoshanCommando,
	eJediEnemyType_TrandoshanFlutterpack,
	eJediEnemyType_Count
};

#pragma endregion

#pragma region jedi threat types

// jedi threat types
enum EJediThreatType {
	eJediThreatType_Blaster,
	eJediThreatType_Melee,
	eJediThreatType_Rush,
	eJediThreatType_Grenade,
	eJediThreatType_Rocket,
	eJediThreatType_Explosion,
	eJediThreatType_Count
};

#pragma endregion

#pragma region SJediThreatInfo

struct SJediThreatInfo {
	CVector wPos;             // threat position
	CVector wEndPos;          // threat end position
	CVector iDir;             // threat direction
	CActor *creator;          // who created this threat
	CActor *object;           // threat object (eg, grenade or rocket)
	CActor *intendedVictim;   // who is this threat intended for (can be NULL)
	EJediThreatType type;     // threat type
	EAttackLevel attackLevel; // attack level
	float strength;           // amount of damage this threat will cause
	float speed;              // how fast this threat is travelling
	float delayToAttackTime;  // how long until this threat applies damage
	float damageRadius;       // how far from wEndPos does this threat apply damage?
	bool isMelee360;          // is this a 360 degree melee attack?
};

// threat list
extern SJediThreatInfo gThreatList[];
extern int gThreatCount;

#pragma endregion

#pragma region swing saber direction

// swing saber direction
enum EJediSwingSaberDir {
	eJediSwingSaberDir_Auto,  // let the jedi choose the best direction
	eJediSwingSaberDir_Left,  // right to left
	eJediSwingSaberDir_Right, // left to right
	eJediSwingSaberDir_Down,  // top to bottom
	eJediSwingSaberDir_Up,    // bottom to top
	eJediSwingSaberDir_Count, // direction count
};

// look an swing saber dir's name
// returns "<unknown>" on error, won't return NULL
extern const char *lookupJediSwingSaberDirName(EJediSwingSaberDir swingSaberDir);

#pragma endregion

#pragma region jedi attack displacement

// jedi attack displacement
enum EJediAttackDisplacement {
	eJediAttackDisplacement_None = 0,
	eJediAttackDisplacement_Mid,
	eJediAttackDisplacement_Long,
	eJediAttackDisplacement_Count
};

#pragma endregion

#pragma region jedi dodge direction

// jedi dodge direction
enum EJediDodgeDir {
	eJediDodgeDir_None,  // no dodge
	eJediDodgeDir_Left,  // dodge left
	eJediDodgeDir_Right, // dodge right
	eJediDodgeDir_Back,  // dodge back
	eJediDodgeDir_Count, // dodge count
};

// look an dodge dir's name
// returns "<unknown>" on error, won't return NULL
extern const char *lookupJediDodgeDirName(EJediDodgeDir dodgeDir);

#pragma endregion

#pragma region jedi block direction

// jedi block direction
enum EJediBlockDir {
	eJediBlockDir_None,  // no block
	eJediBlockDir_High,  // block high
	eJediBlockDir_Left,  // block left
	eJediBlockDir_Right, // block right
	eJediBlockDir_Mid,   // block mid
	eJediBlockDir_Count, // block count
};

// look an block dir's name
// returns "<unknown>" on error, won't return NULL
extern const char *lookupJediBlockDirName(EJediBlockDir blockDir);

#pragma endregion

#pragma region jedi jump forward attack types

// jedi jump forward attack types
enum EJediAiJumpForwardAttack {
	eJediAiJumpForwardAttack_None,       // no jump attack
	eJediAiJumpForwardAttack_Vertical,   // vertical jump attack
	eJediAiJumpForwardAttack_Horizontal, // horizontal jump attack
	eJediAiJumpForwardAttack_Count,      // jump attack count
};

// look up a block dir's name
// returns "<unknown>" on error, won't return NULL
extern const char *lookupJediAiJumpForwardAttackName(EJediAiJumpForwardAttack jumpAttack);

#pragma endregion

#pragma region jedi force push damage tiers

// jedi jump forward attack types
enum EForcePushDamageTier {
	eForcePushDamageTier_One,
	eForcePushDamageTier_Two,
	eForcePushDamageTier_Three,
	eForcePushDamageTier_Count,
};

// compute the force push damage tier
extern EForcePushDamageTier computeForcePushDamageTier(const CVector &wJediPos, const CVector &wTargetPos);

#pragma endregion

#pragma region computeTimeToKill

// how long should it take a given jedi to kill a given enemy type
extern float computeTimeToKill(CJedi *jedi, EJediEnemyType enemyType, float skillLevel);

#pragma endregion


/////////////////////////////////////////////////////////////////////////////
//
// jedi ai types and enumerations
//
/////////////////////////////////////////////////////////////////////////////

#pragma region jedi ai action types

// jedi ai action types
enum EJediAiAction {

	// composite types
	eJediAiAction_Parallel,                      // run multiple actions in parallel
	eJediAiAction_Sequence,                      // run multiple actions in sequence
	eJediAiAction_Selector,                      // select one action at a time from a prioritized list
	eJediAiAction_Random,                        // select one action at a time from a list at random
	eJediAiAction_Decorator,                     // add functionality to a given node

	// utility actions
	eJediAiAction_FakeSim,                       // insert fake data into the simulation

	// move
	eJediAiAction_WalkRun,                       // walk/run toward a specified actor
	eJediAiAction_Dash,                          // dash toward a specified actor
	eJediAiAction_Strafe,                        // strafe in a specified direction
	eJediAiAction_Move,                          // move to the specified actor
	eJediAiAction_JumpForward,                   // jump forward to an actor
	eJediAiAction_JumpOver,                      // jump over an actor
	eJediAiAction_JumpOnto,                      // jump on top of an actor

	// defense
	eJediAiAction_Dodge,                         // dodge in a specified direction
	eJediAiAction_Crouch,                        // crouch for a specified duration
	eJediAiAction_Deflect,                       // deflect for a specified duration
	eJediAiAction_Block,                         // block melee attack
	eJediAiAction_Defend,                        // do whatever defensive action is necessary, if any

	// offense
	eJediAiAction_SwingSaber,                    // swing light saber at enemy
	eJediAiAction_Kick,                          // kick enemy
	eJediAiAction_ForcePush,                     // force push enemy
	eJediAiAction_ForceTk,                       // force tk enemy
	eJediAiAction_BlasterCounterAttack,          // blaster counter attack
	eJediAiAction_MeleeCounterAttack,            // melee counter attack

	// engage targets
	eJediAiAction_Engage,                        // engage our target
	eJediAiAction_EngageTrandoshanInfantry,      // engage a trandoshan infantry target
	eJediAiAction_EngageTrandoshanMelee,         // engage a trandoshan melee target
	eJediAiAction_EngageTrandoshanCommando,      // engage a trandoshan commando target
	eJediAiAction_EngageTrandoshanConcussive,    // engage a trandoshan concussive target
	eJediAiAction_EngageTrandoshanFlutterpack,   // engage a trandoshan flutterpack target
	eJediAiAction_EngageB1BattleDroid,           // engage a b1 battle droid target
	eJediAiAction_EngageB1GrenadeDroid,          // engage a b1 grenade droid target
	eJediAiAction_EngageB1MeleeDroid,            // engage a b1 melee droid target
	eJediAiAction_EngageB1JetpackDroid,          // engage a b1 battle droid target
	eJediAiAction_EngageB2BattleDroid,           // engage a b2 battle droid target
	eJediAiAction_EngageB2RocketDroid,           // engage a b2 rocket droid target
	eJediAiAction_EngageDroideka,                // engage a droideka
	eJediAiAction_EngageCrabDroid,               // engage a crab droid

	// idle
	eJediAiAction_DefensiveStance,               // hold up my saber defensively
	eJediAiAction_WaitForThreat,                 // wait defensively for a given threat scenario to occur
	eJediAiAction_Taunt,                         // taunt my target
	eJediAiAction_Idle,                          // choose an idle action to make the ai look alive

	// combat
	eJediAiAction_Combat,                        // engage enemies in combat

	// number of actions
	eJediAiAction_Count
};

// look an action's name
// returns "<unknown>" on error, won't return NULL
extern const char *lookupJediAiActionName(EJediAiAction action);

#pragma endregion

#pragma region jedi ai action results

// jedi ai action results
enum EJediAiActionResult {
	eJediAiActionResult_Success = 0,
	eJediAiActionResult_InProgress,
	eJediAiActionResult_Failure,
	eJediAiActionResult_Count
};

#pragma endregion

#pragma region jedi ai action simulation result

// jedi ai action simulation result
enum EJediAiActionSimResult {
	eJediAiActionSimResult_Impossible,
	eJediAiActionSimResult_Deadly,
	eJediAiActionSimResult_Hurtful,
	eJediAiActionSimResult_Irrelevant,
	eJediAiActionSimResult_Cosmetic,
	eJediAiActionSimResult_Beneficial,
	eJediAiActionSimResult_Safe,
	eJediAiActionSimResult_Urgent,
	eJediAiActionSimResult_Count
};

// look an action sim results name
// returns "<unknown>" on error, won't return NULL
extern const char *lookupJediAiActionSimResultName(EJediAiActionSimResult simResult);

// determine the result of a simulation by comparing a summary of the initial state to the post-simulation memory
extern EJediAiActionSimResult computeSimResult(SJediAiActionSimSummary &initialSummary, const CJediAiMemory &simMemory);

#pragma endregion

#pragma region jedi ai action simulation summary data

// jedi ai action simulation summary data
struct SJediAiActionSimSummary {
	EJediAiActionSimResult result;    // result of the simulation
	float selfHitPoints;              // how many hitpoints do I have after the sim?
	float victimHitPoints;            // how many hitpoints does my victim have after the sim?
	float threatLevel;                // how threatening is the resulting world state?
	unsigned int victimFlags;         // what is my victim's state after the sim?
	bool selfIsTooCloseToAnotherJedi; // am I too close to another Jedi?
	bool ignoreTooCloseToAnotherJedi; // do I care if I'm too close to another Jedi?
};

// initialize a jedi ai action simulation summary
extern void setSimSummaryMemoryData(SJediAiActionSimSummary &simSummary, const CJediAiMemory &memory);

// initialize a jedi ai action simulation summary
extern void initSimSummary(SJediAiActionSimSummary &simSummary, const CJediAiMemory &memory);

// set a jedi ai action simulation summary
extern void setSimSummary(SJediAiActionSimSummary &simSummary, const CJediAiMemory &memory);

#pragma endregion

#pragma region jedi ai destination types

// jedi ai destination types
enum EJediAiDestination {
	eJediAiDestination_None = 0,
	eJediAiDestination_Victim,
	eJediAiDestination_ForceTkTarget,
	eJediAiDestination_Count
};

// look a destination's name
// returns "<unknown>" on error, won't return NULL
extern const char *lookupJediAiDestinationName(EJediAiDestination destination);

// lookup the actor state of a given destination actor
extern SJediAiActorState *lookupJediAiDestinationActorState(EJediAiDestination destination, const CJediAiMemory &memory, float *minDistance, float *maxDistance);

#pragma endregion

#pragma region jedi ai force tk target types

// jedi force tk target types
enum EJediAiForceTkTarget {
	eJediAiForceTkTarget_Recommended,
	eJediAiForceTkTarget_Victim,
	eJediAiForceTkTarget_Object,
	eJediAiForceTkTarget_Grenade,
	eJediAiForceTkTarget_Count
};

// look a force tk grip target's name
// returns "<unknown>" on error, won't return NULL
extern const char *lookupJediAiForceTkTargetName(EJediAiForceTkTarget gripTarget);

// lookup the actor state of a given forceTk target
extern SJediAiActorState *lookupJediAiForceTkTargetActorState(bool throwing, EJediAiForceTkTarget gripTarget, const CJediAiMemory &memory);

#pragma endregion

#pragma region jedi ai entity state

// knowledge container for world entities
struct SJediAiEntityState {
	CVector wPos;
	CVector wBoundsCenterPos;
	CVector iVelocity;
	CVector iFrontDir;
	CVector iRightDir;
	CVector iToSelfDir;
	float distanceToSelf;
	float selfFacePct;
	float faceSelfPct;
};

#pragma endregion

#pragma region jedi ai actor state

// jedi ai actor state bitflags
const unsigned int kJediAiActorStateFlag_Grippable = (1 << 0);
const unsigned int kJediAiActorStateFlag_Gripped = (1 << 1);
const unsigned int kJediAiActorStateFlag_GrippedBySelf = (1 << 2);
const unsigned int kJediAiActorStateFlag_GripWithTwoHands = (1 << 3);
const unsigned int kJediAiActorStateFlag_GripWithTwoPlayers = (1 << 4);
const unsigned int kJediAiActorStateFlag_Throwable = (1 << 5);
const unsigned int kJediAiActorStateFlag_InRushAttack = (1 << 6);
const unsigned int kJediAiActorStateFlag_Stumbling = (1 << 7);
const unsigned int kJediAiActorStateFlag_Incapacitated = (1 << 8);
const unsigned int kJediAiActorStateFlag_Dead = (1 << 9);
const unsigned int kJediAiActorStateFlag_TargetingSelf = (1 << 10);
const unsigned int kJediAiActorStateFlag_Shielded = (1 << 11);
const unsigned int kJediAiActorStateFlag_CrabCanJumpOnto = (1 << 12);
const unsigned int kJediAiActorStateFlag_TargetedByPlayer = (1 << 13);
const unsigned int kJediAiActorStateFlag_EngagedWithOtherJedi = (1 << 14); // other jedi and actor are targeting each other
const unsigned int kJediAiActorStateFlag_ForceTkObjectCanHitVictim = (1 << 15);
const unsigned int kJediAiActorStateFlag_IsPlayer = (1 << 16);
const unsigned int kJediAiActorStateFlag_CanBeTaunted = (1 << 17);

// knowledge container for actors
struct SJediAiActorState : SJediAiEntityState {
	CActor *actor;
	CActor *victim;
	SJediAiThreatState *threatState;
	EJediCombatType combatType;
	EJediEnemyType enemyType;
	unsigned int flags;
	float hitPoints;
	float collisionRadius;
};
extern SJediAiActorState *gEmptyJediAiActorState;

#pragma endregion

#pragma region jedi ai threat state

// jedi ai threat state bitflags
static const unsigned char kJediAiThreatStateFlag_Horizontal = (1 << 0);
static const unsigned char kJediAiThreatStateFlag_Melee360 = (1 << 1);

// threat dodge direction bitmask values
static const unsigned char kJediAiThreatDodgeDirMask_None = 0;
static const unsigned char kJediAiThreatDodgeDirMask_Left = (1 << ((char)eJediDodgeDir_Left - 1));
static const unsigned char kJediAiThreatDodgeDirMask_Right = (1 << ((char)eJediDodgeDir_Right - 1));
static const unsigned char kJediAiThreatDodgeDirMask_Back = (1 << ((char)eJediDodgeDir_Back - 1));
static const unsigned char kJediAiThreatDodgeDirMask_Lateral = kJediAiThreatDodgeDirMask_Left | kJediAiThreatDodgeDirMask_Right;
static const unsigned char kJediAiThreatDodgeDirMask_All = kJediAiThreatDodgeDirMask_Lateral | kJediAiThreatDodgeDirMask_Back;

// knowledge container for threats
struct SJediAiThreatState : SJediAiEntityState {
	CVector wEndPos;
	SJediAiActorState *attackerState;
	SJediAiActorState *objectState;
	SJediThreatInfo *threat;
	float duration;
	float strength;
	float damageRadius;
	EJediThreatType type;
	EAttackLevel attackLevel;
	EJediBlockDir blockDir;
	unsigned char flags;
	unsigned char dodgeDirMask;

	bool doesDodgeDirWork(EJediDodgeDir dir) const {
		return ((dodgeDirMask & (1 << ((char)dir - 1))) != 0);
	}
};

#pragma endregion


#endif // __JEDI_COMMON__
