#ifndef __JEDI__
#define __JEDI__

#ifndef __ACTOR__
	#include "actor.h"
#endif

#ifndef __JEDI_AI_ACTIONS__
	#include "jedi_ai_actions.h"
#endif

#ifndef __JEDI_AI_MEMORY__
	#include "jedi_ai_memory.h"
#endif


///////////////////////////////////////////////////////////////////////////////
//
// CJedi
//
///////////////////////////////////////////////////////////////////////////////

class CJedi : public CActor {
public:

	// construction
	CJedi();
	virtual ~CJedi();

	// setup this jedi
	virtual bool setup();

	// process this jedi
	virtual void process(float dt);

	// is this actor a jedi?
	// in the real game, we would use RTTI to do this, but this will work for this example
	virtual bool isJedi() const { return true; }


	//------------------------------------
	// attack settings (stubbed for this example)
	//------------------------------------

	// is the saber active?
	bool isSaberActive() const { return true; }

	// how much damage does my saber do?
	float getSaberDamage() const { return 25.0f; }

	// how much damage does my kick do?
	float getKickDamage() const { return 25.0f; }

	// how far can my force push go?
	float getForcePushMaxDist() const { return 100.0f; }

	// what is the radius of the force push blast?
	float getForcePushRadius() const { return 10.0f; }

	// get the actor that this jedi is gripping (NULL if none)
	CActor *getForceSelectedActor() const { return NULL; }


	//------------------------------------
	// ai section
	//------------------------------------

	// is this jedi under ai control?
	virtual bool isAiControlled() const { return true; }

	// AI data
	CJediAiMemory aiMemory;
	CJediAiActionCombat aiCombatAction;

	// is this jedi a padawan?
	bool isPadawan() const { return false; }

	// AI settings
	float getSkillLevel() { return 1.0f; }

	// stop my current action
	// pass the name of the calling function so we know where we were stopped
	void stop(const char *callingFunctionName) {}

	// walk toward the specified point
	void passNearPoint(const CVector &point, float distanceFromTarget, bool run, const char *callingFunctionName) {}

	// set desired movement speed
	void setDesiredMoveSpeed(float desMoveSpeed) {}

	// has my kill timer elapsed for the specified enemy?
	// returns false if the specified enemy isn't my target or if I am not AI controlled
	bool hasEnemyKillTimerExpired(const CActor *enemy) const { return false; }


	//---------------------------------
	// Ai defensive mode
	// defensive mode prevents the Ai from dealing any damage to an enemy
	// if 'disableOnPlayerAttack' is true, the Ai will automatically disable
	// defensive mode when a player performs an aggressive action
	// aggressive actions include:
	//   - Walk/Run
	//   - Dash
	//   - Jump
	//   - Causing any Damage
	//---------------------------------

	// defensive mode data
	struct {
		bool enabled;
		bool disableOnPlayerAttack;
	} defensiveModeData;

	// is defensive mode enabled?
	bool isDefensiveModeEnabled() const;

	// enable defensive mode
	void enableDefensiveMode(bool disableOnPlayerAttack);

	// disable defensive mode
	void disableDefensiveMode();

	// called when the Jedi does something aggressive
	void onPlayerAggressive();


	//---------------------------------
	// Actions: things the jedi is currently doing
	//---------------------------------

	// is this jedi's model in the specified animation range?
	bool isModelInAnimationRange(const char *animRangeName);

	// evaluate the animation channels to determine which actions we are currently doing
	void updateActionBitfield();
	int currentStateBitfield;
	int disabledActionBitfield;
	int disabledActionAiBitfield;

	// allow all actions
	void setCanDoAllActions(bool allow);

	// can the AI do all actions?
	void setCanAiDoAllActions(bool allow);

	// return whether or not we can do a particular action
	bool canDoAction(EJediAction action) const;

	// set whether or not we can do a particular action
	void setCanDoAction(EJediAction action, bool allow, bool aiOnly);


	//---------------------------------
	// Commands: things we can tell the Jedi to do
	//---------------------------------

	// commands
	enum ECommand {
		eCommandStandDefensive,
		eCommandStrafe,
		eCommandSwingSaber,
		eCommandDodge,
		eCommandDash,
		eCommandForcePushCharge,
		eCommandForcePushThrow,
		eCommandForceTkGrip,
		eCommandForceTkThrow,
		eCommandJumpOver,
		eCommandJumpForward,
		eCommandCrouch,
		eCommandCrouchAttack,
		eCommandKick,
		eCommandDeflect,
		eCommandBlock,
		eCommandBlockImpact,
		eCommandTaunt,
		eCommandHurt,
		eCommandCount
	};
	compileTimeAssert(eCommandCount <= 32);
	unsigned int commands;

	// commands
	bool hasCommand(ECommand command) const;
	void setCommand(ECommand command);

	// clear all  command params
	void clearJediCommandParams();

	// stand defensive
	void setCommandJediStandDefensive();

	// strafe
	void setCommandJediStrafe();

	// swing saber command
	void setCommandJediSwingSaber(EJediSwingSaberDir dir = eJediSwingSaberDir_Auto);
	struct SJediSwingSaberParams {
		EJediSwingSaberDir dir;
		EJediAttackDisplacement displacement;
		CVector wStartPos;
	} commandSwingSaberParams;

	// dodge command
	void setCommandJediDodge(EJediDodgeDir dir, bool attack);

	// dodge flip command
	void setCommandJediDodgeFlip(EJediDodgeDir dir);
	struct SJediDodgeParams {
		EJediDodgeDir dir;
		bool attack;
		bool flip;
	} commandDodgeParams;

	// dash command
	void setCommandJediDash(CActor *target, float distanceToTarget, bool attack);
	struct SJediDashParams {
		CActor *target;
		float distanceToTarget;
		bool attack;
	} commandDashParams;

	// force push charge command
	void setCommandJediForcePushCharge();

	// force push throw command
	void setCommandJediForcePushThrow();

	// force tk grip command
	void setCommandJediForceTkGrip(CActor *target, bool twoHand, float holdHeight = 1.0f, bool skipEnter = false);
	struct SJediForceTkGripParams {
		CActor *target;
		bool twoHand;
		bool skipEnter;
		float holdHeight;
	} commandForceTkGripParams;

	// force tk throw command
	void setCommandJediForceTkThrow(CVector iThrowVelocity);

	// force tk throw at target command
	void setCommandJediForceTkThrowAtTarget(CActor *target);
	bool advAiComputeThrowAtTargetVelocity(CActor *target, CVector &iThrowVelocity) const;
	struct SJediForceTkThrowParams {
		CVector iThrowVelocity;
		CActor *target;
		float throwRightPct;
		float throwUpPct;
	} commandForceTkThrowParams;

	// jump over command
	void setCommandJediJumpOver(CActor *target, bool attack);
	struct SJediJumpOverParams {
		CActor *target;
		bool attack;
	} commandJumpOverParams;

	// jump forward command
	void setCommandJediJumpForward(CActor *target, EJediAiJumpForwardAttack attack, float distance = 0.0f);
	void initJumpForwardPath();
	struct SJediJumpForwardParams {
		CActor *target;
		EJediAiJumpForwardAttack attack;
		float distance;
	} commandJumpForwardParams;
	struct SJediJumpForwardData {
		SJediJumpForwardParams params;
		float pitchAnimVar;
		float exitPosT;
		bool turnedOffSaber;
		bool attacking;
	} commandJumpForwardData;

	// crouch command
	void setCommandJediCrouch();

	// un-crouch command
	void setCommandJediUnCrouch();

	// crouch attack command
	void setCommandJediCrouchAttack();

	// kick command
	void setCommandJediKick(bool allowDisplacement = true);
	struct {
		EJediAttackDisplacement displacement;
		CVector wStartPos;
		bool allowDisplacement;
	} commandKickParams;

	// deflect continuously command (you must send this command every frame that you want the Jedi to deflect)
	void setCommandJediDeflect(bool deflectAtEnemies);
	struct SJediDeflectParams {
		bool deflectAtEnemies;
	} commandDeflectParams;

	// block command
	void setCommandJediBlock(EJediBlockDir dir);
	struct SJediBlockParams {
		EJediBlockDir dir;
	} commandBlockParams;

	// block impact command
	void setCommandJediBlockImpact();

	// taunt command
	void setCommandJediTaunt();
};

#endif // __JEDI__
