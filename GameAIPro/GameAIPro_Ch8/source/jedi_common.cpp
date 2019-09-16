#include "pch.h"
#include "jedi_common.h"
#include "jedi_ai_memory.h"


/////////////////////////////////////////////////////////////////////////////
//
// globals and constants
//
/////////////////////////////////////////////////////////////////////////////

// Jedi constants
float kJediMoveSpeedMax = 0.0f;
float kJediDashExitDistance = 0.0f;
float kJediStrafeSpeed = 0.0f;
float kJediJumpForwardAttackHorizontalExitDuration = 0.0f;
float kJediJumpForwardAttackVerticalExitDuration = 0.0f;
float kJediJumpForwardAttackEnterDuration = 0.0f;
float kJediJumpForwardExitInFlightDuration = 0.0f;
float kJediJumpForwardExitDuration = 0.0f;
float kJediJumpForwardEnterInFlightDuration = 0.0f;
float kJediJumpForwardEnterDuration = 0.0f;
float kJediJumpOverCrouchDuration = 0.0f;
float kJediJumpOverDuration = 0.0f;
float kJediDodgeDistance = 0.0f;
float kJediDodgeFlipDistance = 0.0f;
float kJediDodgeDuration = 0.0f;
float kJediDodgeFlipDuration = 0.0f;
float kJediCrouchAttackDuration = 0.0f;
float kJediSwingSaberEnterDuration = 0.0f;
float kJediSwingSaberDuration = 0.0f;
float kJediKickDistance = 0.0f;
float kJediKickDuration = 0.0f;
float kJediForcePushExitDuration = 0.0f;
float kJediForcePushEnterDuration = 0.0f;
float kJediForceTkOneHandExitDuration = 0.0f;
float kJediForceTkTwoHandExitDuration = 0.0f;
float kJediForceTkOneHandEnterDuration = 0.0f;
float kJediForceTkTwoHandEnterDuration = 0.0f;
float kJediTauntDuration = 0.0f;
float kJediDashEnterDistance = 0.0f;
float kJediDashSpeed = 0.0f;
float kJediMeleeCorrectionTweak = 0.0f;
float kJediSaberEffectDamageStrength = 0.0f;
float kJediSaberEffectSlamVicinityRadius = 0.0f;
float kJediSaberEffectSlashVicinityRadius = 0.0f;
float kJediSaberEffectConeRadius = 0.0f;
float kJediForwardJumpSpeed = 0.0f;
float kJediForceSelectRange = 0.0f;
float kJediThrowRange = 0.0f;
float kJediForcePushTier2Min = 0.0f;
float kJediForcePushTier3Min = 0.0f;
float kJediCombatMaxJumpDistance = 0.0f;
float kJediForwardJumpMaxDistance = 0.0f;

// jedi physical constants
float kJediSwingSaberDistance = 5.0f;
float kJediAiDashActivationDistanceMin = kJediDashEnterDistance + 20.0f + kJediDashExitDistance;
float kJediAiForceTkActivationDistanceMin = 15.0f;
float kJediAiForceTkFacePctMin = 0.25f;

// constants
const float kThreatRangedAwareDistance = 100.0f;
const float kThreatMeleeAwareDistance = 15.0f;
const float kThreatRushAwareDistance = 100.0f;
const float kThreatRushAwareDuration = 10.0f;
const float kThreatMaxAwareDistance = 100.0f;
const float kThreatMaxAwareDuration = 10.0f; // seconds
const float kCollisionCheckDistance = 20.0f;


/////////////////////////////////////////////////////////////////////////////
//
// random number utils
//
/////////////////////////////////////////////////////////////////////////////

float fRand(float rangeMin, float rangeMax) {
	return (((float)rand() / (float)RAND_MAX) * (rangeMax + rangeMax));
}

bool randBool(float odds) {
	return (fRand(0.0f, odds) < odds);
}

int randChoice(int oddsTableSize, float oddsTable[]) {

	// calculate the odds total
	float oddsTotal = 0.0f;
	for (int i = 0; i < oddsTableSize; ++i) {
		oddsTotal += oddsTable[i];
	}

	// compute a random value in the range [0, oddsTotal]
	float value = fRand(0.0f, oddsTotal);

	// determine which odds element the value falls into
	for (int i = 0; i < oddsTableSize; ++i) {
		value -= oddsTable[i];
		if (value <= 0.0f) {
			return i;
		}
	}

	// we didn't find a value
	return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
// jedi ai types and enumerations
//
/////////////////////////////////////////////////////////////////////////////

const char *lookupJediActionName(EJediAction action) {
	switch (action) {
		case eJediAction_WalkRun: return "WalkRun";
		case eJediAction_SwingSaber: return "SwingSaber";
		case eJediAction_Crouch: return "Crouch";
		case eJediAction_Dodge: return "Dodge";
		case eJediAction_Dash: return "Dash";
		case eJediAction_Jump: return "Jump";
		case eJediAction_ForcePush: return "ForcePush";
		case eJediAction_ForceTk: return "ForceTk";
		case eJediAction_Kick: return "Kick";
		case eJediAction_Block: return "Block";
		case eJediAction_Deflect: return "Deflect";
		case eJediAction_Taunt: return "Taunt";
		case eJediAction_TankAttack: return "Tank Attack";
	}
	return "<unknown>";
}

const char *lookupJediStateName(EJediState state) {
	switch (state) {
		case eJediState_SwingingSaber: return "SwingingSaber";
		case eJediState_Crouching: return "Crouching";
		case eJediState_Dodging: return "Dodging";
		case eJediState_Dashing: return "Dashing";
		case eJediState_Jumping: return "Jumping";
		case eJediState_Flipping: return "Flipping";
		case eJediState_ForcePushing: return "ForcePushing";
		case eJediState_UsingForceTk: return "UsingForceTk";
		case eJediState_ForceTkAllowed: return "ForceTkAllowed";
		case eJediState_Kicking: return "Kicking";
		case eJediState_Blocking: return "Blocking";
		case eJediState_KnockedAround: return "KnockedAround";
		case eJediState_Deflecting: return "DeflectingActively";
		case eJediState_Reflecting: return "Reflecting";
		case eJediState_AiBlocked: return "AiBlocked";
		case eJediState_Turning: return "Turning";
		case eJediState_DodgingLeft: return "DodgingLeft";
		case eJediState_DodgingRight: return "DodgingRight";
	}
	return "<unknown>";
}

// list of active threats
SJediThreatInfo gThreatList[32] = {};
int gThreatCount = 0;

const char *lookupJediSwingSaberDirName(EJediSwingSaberDir swingSaberDir) {
	switch (swingSaberDir) {
		case eJediSwingSaberDir_Auto: return "eJediSwingSaberDir_Auto";
		case eJediSwingSaberDir_Left: return "eJediSwingSaberDir_Left";
		case eJediSwingSaberDir_Right: return "eJediSwingSaberDir_Right";
		case eJediSwingSaberDir_Down: return "eJediSwingSaberDir_Down";
		case eJediSwingSaberDir_Up: return "eJediSwingSaberDir_Up";
		default: return "<unknown>";
	}
}

const char *lookupJediDodgeDirName(EJediDodgeDir dodgeDir) {
	switch (dodgeDir) {
		case eJediDodgeDir_None: return "eJediDodgeDir_None";
		case eJediDodgeDir_Left: return "eJediDodgeDir_Left";
		case eJediDodgeDir_Right: return "eJediDodgeDir_Right";
		case eJediDodgeDir_Back: return "eJediDodgeDir_Back";
		default: return "<unknown>";
	}
}

const char *lookupJediBlockDirName(EJediBlockDir blockDir) {
	switch (blockDir) {
		case eJediBlockDir_None: return "eJediBlockDir_None";
		case eJediBlockDir_High: return "eJediBlockDir_High";
		case eJediBlockDir_Left: return "eJediBlockDir_Left";
		case eJediBlockDir_Right: return "eJediBlockDir_Right";
		case eJediBlockDir_Mid: return "eJediBlockDir_Mid";
		default: return "<unknown>";
	}
}

const char *lookupJediAiJumpForwardAttackName(EJediAiJumpForwardAttack jumpAttack) {
	switch (jumpAttack) {
		case eJediAiJumpForwardAttack_None: return "eJediAiJumpForwardAttack_None";
		case eJediAiJumpForwardAttack_Vertical: return "eJediAiJumpForwardAttack_Vertical";
		case eJediAiJumpForwardAttack_Horizontal: return "eJediAiJumpForwardAttack_Horizontal";
		default: return "<unknown>";
	}
}

EForcePushDamageTier computeForcePushDamageTier(const CVector &wJediPos, const CVector &wTargetPos) {
	float distanceSq = wJediPos.distanceSqTo(wTargetPos);
	if (distanceSq > SQ(15.0f)) {
		return eForcePushDamageTier_Three;
	} else if (distanceSq > SQ(7.5f)) {
		return eForcePushDamageTier_Two;
	} else {
		return eForcePushDamageTier_One;
	}
}

float computeTimeToKill(CJedi *jedi, EJediEnemyType enemyType, float skillLevel) {
	return 5.0f; // 5 seconds for this example
}


/////////////////////////////////////////////////////////////////////////////
//
// jedi ai types and enumerations
//
/////////////////////////////////////////////////////////////////////////////

const char *lookupJediAiActionName(EJediAiAction action) {
	switch (action) {

		// composite types
		case eJediAiAction_Parallel: return "eJediAiAction_Parallel";
		case eJediAiAction_Sequence: return "eJediAiAction_Sequence";
		case eJediAiAction_Selector: return "eJediAiAction_Selector";
		case eJediAiAction_Random: return "eJediAiAction_Random";
		case eJediAiAction_Decorator: return "eJediAiAction_Decorator";

		// utility actions
		case eJediAiAction_FakeSim: return "eJediAiAction_FakeSim";

		// move
		case eJediAiAction_WalkRun: return "eJediAiAction_WalkRun";
		case eJediAiAction_Dash: return "eJediAiAction_Dash";
		case eJediAiAction_Strafe: return "eJediAiAction_Strafe";
		case eJediAiAction_Move: return "eJediAiAction_Move";
		case eJediAiAction_JumpForward: return "eJediAiAction_JumpForward";
		case eJediAiAction_JumpOver: return "eJediAiAction_JumpOver";
		case eJediAiAction_JumpOnto: return "eJediAiAction_JumpOnto";

		// defense
		case eJediAiAction_Dodge: return "eJediAiAction_Dodge";
		case eJediAiAction_Crouch: return "eJediAiAction_Crouch";
		case eJediAiAction_Deflect: return "eJediAiAction_Deflect";
		case eJediAiAction_Block: return "eJediAiAction_Block";
		case eJediAiAction_Defend: return "eJediAiAction_Defend";

		// offense
		case eJediAiAction_SwingSaber: return "eJediAiAction_SwingSaber";
		case eJediAiAction_Kick: return "eJediAiAction_Kick";
		case eJediAiAction_ForcePush: return "eJediAiAction_ForcePush";
		case eJediAiAction_ForceTk: return "eJediAiAction_ForceTk";
		case eJediAiAction_BlasterCounterAttack: return "eJediAiAction_BlasterCounterAttack";
		case eJediAiAction_MeleeCounterAttack: return "eJediAiAction_MeleeCounterAttack";

		// engage targets
		case eJediAiAction_Engage: return "eJediAiAction_Engage";
		case eJediAiAction_EngageTrandoshanInfantry: return "eJediAiAction_EngageTrandoshanInfantry";
		case eJediAiAction_EngageTrandoshanMelee: return "eJediAiAction_EngageTrandoshanMelee";
		case eJediAiAction_EngageTrandoshanCommando: return "eJediAiAction_EngageTrandoshanCommando";
		case eJediAiAction_EngageTrandoshanConcussive: return "eJediAiAction_EngageTrandoshanConcussive";
		case eJediAiAction_EngageTrandoshanFlutterpack: return "eJediAiAction_EngageTrandoshanFlutterpack";
		case eJediAiAction_EngageB1BattleDroid: return "eJediAiAction_EngageB1BattleDroid";
		case eJediAiAction_EngageB1GrenadeDroid: return "eJediAiAction_EngageB1GrenadeDroid";
		case eJediAiAction_EngageB1MeleeDroid: return "eJediAiAction_EngageB1MeleeDroid";
		case eJediAiAction_EngageB1JetpackDroid: return "eJediAiAction_EngageB1JetpackDroid";
		case eJediAiAction_EngageB2BattleDroid: return "eJediAiAction_EngageB2BattleDroid";
		case eJediAiAction_EngageB2RocketDroid: return "eJediAiAction_EngageB2RocketDroid";
		case eJediAiAction_EngageDroideka: return "eJediAiAction_EngageDroideka";
		case eJediAiAction_EngageCrabDroid: return "eJediAiAction_EngageCrabDroid";

		// idle
		case eJediAiAction_DefensiveStance: return "eJediAiAction_DefensiveStance";
		case eJediAiAction_WaitForThreat: return "eJediAiAction_WaitForThreat";
		case eJediAiAction_Taunt: return "eJediAiAction_Taunt";
		case eJediAiAction_Idle: return "eJediAiAction_Idle";

		// combat
		case eJediAiAction_Combat: return "eJediAiAction_Combat";

		// unknown
		default: return "<unknown action>";
	}
	compileTimeAssert(eJediAiAction_Count == 43);
}

const char *lookupJediAiActionSimResultName(EJediAiActionSimResult simResult) {
	switch (simResult) {
		case eJediAiActionSimResult_Impossible: return "Impossible";
		case eJediAiActionSimResult_Deadly: return "Deadly";
		case eJediAiActionSimResult_Hurtful: return "Hurtful";
		case eJediAiActionSimResult_Irrelevant: return "Irrelevant";
		case eJediAiActionSimResult_Cosmetic: return "Cosmetic";
		case eJediAiActionSimResult_Beneficial: return "Beneficial";
		case eJediAiActionSimResult_Safe: return "Safe";
		case eJediAiActionSimResult_Urgent: return "Urgent";
		default: return "<unknown>";
	}
}

const char *lookupJediAiDestinationName(EJediAiDestination destination) {
	switch (destination) {
		case eJediAiDestination_None: return "eJediAiDestination_None";
		case eJediAiDestination_Victim: return "eJediAiDestination_Victim";
		case eJediAiDestination_ForceTkTarget: return "eJediAiDestination_ForceTkTarget";
		default: return "<unknown>";
	}
	compileTimeAssert(eJediAiDestination_Count == 3);
}

SJediAiActorState *lookupJediAiDestinationActorState(EJediAiDestination destination, const CJediAiMemory &memory, float *minDistance, float *maxDistance) {

	// defaults
	if (minDistance != NULL) *minDistance = 0.5f;
	if (maxDistance != NULL) *maxDistance = 1e20f;

	// handle the specified destination
	switch (destination) {

		// victim
		case eJediAiDestination_Victim: {
			if (memory.victimState->actor == NULL || memory.victimState->hitPoints <= 0.0f) {
				return NULL;
			}
			if (minDistance != NULL) {
				*minDistance = memory.victimState->collisionRadius + memory.selfState.collisionRadius;
			}
			return memory.victimState;
		}

		// force tk target
		case eJediAiDestination_ForceTkTarget: {
			if (memory.forceTkBestTargetState->actor == NULL) {
				return NULL;
			}
			if (minDistance != NULL) {
				*minDistance = memory.forceTkBestTargetState->collisionRadius + memory.selfState.collisionRadius;
			}
			return memory.forceTkBestTargetState;
		}

		// anything else
		default: {
			return NULL;
		}
	}
	compileTimeAssert(eJediAiDestination_Count == 3);
}

const char *lookupJediAiForceTkTargetName(EJediAiForceTkTarget gripTarget) {
	switch (gripTarget) {
		case eJediAiForceTkTarget_Recommended: return "eJediAiForceTkTarget_Recommended";
		case eJediAiForceTkTarget_Victim: return "eJediAiForceTkTarget_Victim";
		case eJediAiForceTkTarget_Object: return "eJediAiForceTkTarget_Object";
		case eJediAiForceTkTarget_Grenade: return "eJediAiForceTkTarget_Grenade";
		default: return "<unknown>";
	}
}

SJediAiActorState *lookupJediAiForceTkTargetActorState(bool throwing, EJediAiForceTkTarget gripTarget, const CJediAiMemory &memory) {

	// handle the specified grip target
	const SJediAiActorState *actorState = NULL;
	switch (gripTarget) {

		// recommended target
		case eJediAiForceTkTarget_Recommended: {
			actorState = (throwing ? memory.forceTkBestThrowTargetState : memory.forceTkBestTargetState);
		} break;

		// victim
		case eJediAiForceTkTarget_Victim: {
			actorState = memory.victimState;
		} break;

		// object
		case eJediAiForceTkTarget_Object: {
			for (int i = 0; i < memory.forceTkObjectStateCount; ++i) {
				const SJediAiActorState *objectState = &memory.forceTkObjectStates[i];

				// if I can't hit my victim with this object, skip it
				if (!(objectState->flags & kJediAiActorStateFlag_ForceTkObjectCanHitVictim)) {
					continue;
				}

				// if this object is too close to me, skip it
				if (objectState->distanceToSelf < kJediAiForceTkActivationDistanceMin) {
					continue;
				}

				// if this object is too close to my victim, skip it
				if (memory.victimState->actor != NULL) {
					float distToVictimSq = objectState->wPos.distanceSqTo(memory.victimState->wPos);
					if (distToVictimSq < SQ(kJediAiForceTkActivationDistanceMin)) {
						continue;
					}
				}

				// pick this object
				actorState = objectState;
				break;
			}
		} break;

		// grenade
		case eJediAiForceTkTarget_Grenade: {
			const SJediAiThreatState *threatState = memory.threatTypeDataTable[eJediThreatType_Grenade].shortestDurationThreat;
			if (threatState != NULL) {
				actorState = threatState->objectState;
			}
		} break;

		// anything else
		default: {
			return NULL;
		}
	}
	compileTimeAssert(eJediAiForceTkTarget_Count == 4);

	// if I didn't get an actor state, bail
	if (actorState == NULL || actorState->actor == NULL) {
		return NULL;
	}

	// if the target is incapacitated, we can't use it
	if (actorState->flags & kJediAiActorStateFlag_Incapacitated) {
		return NULL;
	}

	// if we are getting a grip target (not a throw target), make sure that we can grip it
	if (!throwing && !(actorState->flags & kJediAiActorStateFlag_Grippable)) {
		return NULL;
	}

	// return our actor state
	return const_cast<SJediAiActorState*>(actorState);
}

// determine the result of a simulation by comparing a summary of the initial state to the post-simulation memory
EJediAiActionSimResult computeSimResult(SJediAiActionSimSummary &initialSummary, const CJediAiMemory &simMemory) {

	// if this sim leaves me inside another jedi, I can't do it
	if (!initialSummary.ignoreTooCloseToAnotherJedi && simMemory.selfState.isTooCloseToAnotherJedi) {
		return eJediAiActionSimResult_Impossible;

	// if this sim moves me out from inside another jedi, I must do it
	} else if (initialSummary.selfIsTooCloseToAnotherJedi && !simMemory.selfState.isTooCloseToAnotherJedi) {
		return eJediAiActionSimResult_Urgent;

	// if we are in defensive mode and this action damages my victim, I can't do it
	} else if (
		(simMemory.selfState.defensiveModeEnabled) &&
		(simMemory.victimState->hitPoints < initialSummary.victimHitPoints)
	) {
		return eJediAiActionSimResult_Impossible;

	// if any actors targeted by a player were disturbed during this simulation, I can't do it
	} else if (simMemory.playerTargetDisturbedDuringSimulation) {
		return eJediAiActionSimResult_Impossible;

	// if we are more hurt than before, the sim was deadly or hurtful
	} else if (simMemory.selfState.hitPoints < initialSummary.selfHitPoints) {
		if (simMemory.selfState.hitPoints <= 0.0f) {
			return eJediAiActionSimResult_Deadly;
		} else {
			return eJediAiActionSimResult_Hurtful;
		}

	// if our threat level has increased, the sim was hurtful
	} else if (simMemory.threatLevel > initialSummary.threatLevel) {
		return eJediAiActionSimResult_Hurtful;

	// if we killed the victim too soon, that is hurtful
	// one exception to this is the kamikaze trandoshan flutterpack
	} else if (
		(simMemory.victimState->hitPoints <= 0) && (initialSummary.victimHitPoints > 0.0f) &&
		(simMemory.victimTimer <= simMemory.victimDesiredKillTime) &&
		!(
			(simMemory.victimState->enemyType == eJediEnemyType_TrandoshanFlutterpack) &&
			(simMemory.victimState->flags & kJediAiActorStateFlag_InRushAttack)
		)
	) {
		return eJediAiActionSimResult_Hurtful;

	// if our threat level has decreased, the sim was helpful
	// if it decreased by a lot, the sim was necessary
	} else if (simMemory.threatLevel < initialSummary.threatLevel) {
		float delta = (initialSummary.threatLevel - simMemory.threatLevel);
		if (delta < 0.05f) {
			return eJediAiActionSimResult_Safe;
		} else {
			return eJediAiActionSimResult_Urgent;
		}

	// if any actors engaged with another jedi died during this simulation, the sim was hurtful
	} else if (simMemory.numActorsEngagedWithOtherJediDiedDuringSimulation > 0) {
		return eJediAiActionSimResult_Hurtful;

	// if the victim is more hurt than before, the sim was helpful
	} else if (simMemory.victimState->hitPoints < initialSummary.victimHitPoints) {
		return eJediAiActionSimResult_Beneficial;

	// if the victim was shielded and is no longer, the sim was helpful
	} else if (!(simMemory.victimState->flags & kJediAiActorStateFlag_Shielded) && (initialSummary.victimFlags & kJediAiActorStateFlag_Shielded)) {
		return eJediAiActionSimResult_Beneficial;
	}

	// otherwise, the sim was irrelevant
	return eJediAiActionSimResult_Irrelevant;
}

// initialize a jedi ai action simulation summary
void setSimSummaryMemoryData(SJediAiActionSimSummary &simSummary, const CJediAiMemory &memory) {
	simSummary.selfHitPoints = memory.selfState.hitPoints;
	simSummary.victimHitPoints = memory.victimState->hitPoints;
	simSummary.threatLevel = memory.threatLevel;
	simSummary.victimFlags = memory.victimState->flags;
	simSummary.selfIsTooCloseToAnotherJedi = memory.selfState.isTooCloseToAnotherJedi;
	simSummary.ignoreTooCloseToAnotherJedi = false;
}

// initialize a jedi ai action simulation summary
void initSimSummary(SJediAiActionSimSummary &simSummary, const CJediAiMemory &memory) {
	simSummary.result = eJediAiActionSimResult_Impossible;
	setSimSummaryMemoryData(simSummary, memory);
}

// set a jedi ai action simulation summary
void setSimSummary(SJediAiActionSimSummary &simSummary, const CJediAiMemory &memory) {
	simSummary.result = computeSimResult(simSummary, memory);
	setSimSummaryMemoryData(simSummary, memory);
}

// empty jedi ai actor state
static SJediAiActorState kEmptyJediAiActorState;
SJediAiActorState *gEmptyJediAiActorState = &kEmptyJediAiActorState;
