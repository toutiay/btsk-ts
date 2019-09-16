#include "pch.h"
#include "jedi_ai_constraints.h"
#include "jedi_ai_actions.h"
#include "jedi_ai_memory.h"


/////////////////////////////////////////////////////////////////////////////
//
// constraint
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionConstraint::CJediAiActionConstraint() {
	nextConstraint = NULL;
	skipWhileInProgress = false;
	skipWhileSimulating = false;
}

void CJediAiActionConstraint::reset() {
	if (nextConstraint != NULL) {
		nextConstraint->reset();
	}
	skipWhileInProgress = false;
	skipWhileSimulating = false;
}


/////////////////////////////////////////////////////////////////////////////
//
// CJediAiActionConstraintKillTimer
//
/////////////////////////////////////////////////////////////////////////////

const float CJediAiActionConstraintKillTimer::kKillTimeIgnored = 0.0f;
const float CJediAiActionConstraintKillTimer::kKillTimeDesired = -1.0f;

CJediAiActionConstraintKillTimer::CJediAiActionConstraintKillTimer() {
	memset(&defaultParams, 0, sizeof(defaultParams));
	defaultSkipWhileInProgress = false;
	reset();
}

CJediAiActionConstraintKillTimer::CJediAiActionConstraintKillTimer(float minKillTime, float maxKillTime, bool _skipWhileInProgress) {
	memset(&defaultParams, 0, sizeof(defaultParams));
	defaultParams.minKillTime = minKillTime;
	defaultParams.maxKillTime = maxKillTime;
	defaultSkipWhileInProgress = _skipWhileInProgress;
	reset();
}

void CJediAiActionConstraintKillTimer::reset() {

	// base class version
	BASECLASS::reset();

	// reset my params and data
	params = defaultParams;
	skipWhileInProgress = defaultSkipWhileInProgress;
}

EJediAiActionResult CJediAiActionConstraintKillTimer::checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const {

	// if we are skipping this check while in progress and our action is in progress, bail
	if (action.isInProgress() && skipWhileInProgress) {
		return eJediAiActionResult_InProgress;
	}

	// if we are skipping this check while simulating and our action is simulating, bail
	if (simulating && skipWhileSimulating) {
		return eJediAiActionResult_InProgress;
	}

	// if we have a minimum threshold and our kill timer is below it, we fail
	if (params.minKillTime != kKillTimeIgnored) {
		float minKillTime = (params.minKillTime == kKillTimeDesired ? simMemory.victimDesiredKillTime : params.minKillTime);
		if (simMemory.victimTimer < minKillTime) {
			return eJediAiActionResult_Failure;
		}
	}

	// if we have a maximum threshold and our kill timer is above it, we fail
	if (params.maxKillTime != kKillTimeIgnored) {
		float maxKillTime = (params.maxKillTime == kKillTimeDesired ? simMemory.victimDesiredKillTime : params.minKillTime);
		if (simMemory.victimTimer > maxKillTime) {
			return eJediAiActionResult_Failure;
		}
	}

	// constraint passed
	return eJediAiActionResult_InProgress;
}


/////////////////////////////////////////////////////////////////////////////
//
// CJediAiActionConstraintThreat
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionConstraintThreat::CJediAiActionConstraintThreat() {
	reset();
}

void CJediAiActionConstraintThreat::reset() {

	// base class version
	BASECLASS::reset();

	// clear my params and data
	memset(&params, 0, sizeof(params));
	params.threatReaction = eThreatReaction_Count;
}

EJediAiActionResult CJediAiActionConstraintThreat::checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const {

	// if we are skipping this check while in progress and our action is in progress, bail
	if (action.isInProgress() && skipWhileInProgress) {
		return eJediAiActionResult_InProgress;
	}

	// if we are skipping this check while simulating and our action is simulating, bail
	if (simulating && skipWhileSimulating) {
		return eJediAiActionResult_InProgress;
	}

	// check all threats
	switch (params.threatReaction) {
		case eThreatReaction_SucceedIfNone: {
			if (simMemory.threatStateCount <= 0) {
				return eJediAiActionResult_Success;
			}
		} break;
		case eThreatReaction_SucceedIfAny: {
			if (simMemory.threatStateCount > 0) {
				return eJediAiActionResult_Success;
			}
		} break;
		case eThreatReaction_FailIfNone: {
			if (simMemory.threatStateCount <= 0) {
				return eJediAiActionResult_Failure;
			}
		} break;
		case eThreatReaction_FailIfAny: {
			if (simMemory.threatStateCount > 0) {
				return eJediAiActionResult_Failure;
			}
		} break;
	}

	// check for 'succeed if any' reactions
	for (int i = 0; i < params.threatTypeReactionTable[eThreatReaction_SucceedIfAny].count; ++i) {
		EJediThreatType type = params.threatTypeReactionTable[eThreatReaction_SucceedIfAny].list[i];
		if (simMemory.threatTypeDataTable[type].count > 0) {
			return eJediAiActionResult_Success;
		}
	}

	// check for 'fail if any' reactions
	for (int i = 0; i < params.threatTypeReactionTable[eThreatReaction_FailIfAny].count; ++i) {
		EJediThreatType type = params.threatTypeReactionTable[eThreatReaction_FailIfAny].list[i];
		if (simMemory.threatTypeDataTable[type].count > 0) {
			return eJediAiActionResult_Failure;
		}
	}

	// check for 'succeed if none' reactions
	if (params.threatTypeReactionTable[eThreatReaction_SucceedIfNone].count > 0) {
		bool foundOne = false;
		for (int i = 0; i < params.threatTypeReactionTable[eThreatReaction_SucceedIfNone].count; ++i) {
			EJediThreatType type = params.threatTypeReactionTable[eThreatReaction_SucceedIfNone].list[i];
			if (simMemory.threatTypeDataTable[type].count > 0) {
				foundOne = true;
				break;
			}
		}
		if (!foundOne) {
			return eJediAiActionResult_Success;
		}
	}

	// check for 'fail if none' reactions
	if (params.threatTypeReactionTable[eThreatReaction_FailIfNone].count > 0) {
		bool foundOne = false;
		for (int i = 0; i < params.threatTypeReactionTable[eThreatReaction_FailIfNone].count; ++i) {
			EJediThreatType type = params.threatTypeReactionTable[eThreatReaction_FailIfNone].list[i];
			if (simMemory.threatTypeDataTable[type].count > 0) {
				foundOne = true;
				break;
			}
		}
		if (!foundOne) {
			return eJediAiActionResult_Failure;
		}
	}

	// constraint passed
	return eJediAiActionResult_InProgress;
}


/////////////////////////////////////////////////////////////////////////////
//
// CJediAiActionConstraintSkillLevel
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionConstraintSkillLevel::CJediAiActionConstraintSkillLevel() {
	defaultParams.minSkillLevel = 0.0f;
	defaultParams.maxSkillLevel = 1.0f;
	reset();
}

CJediAiActionConstraintSkillLevel::CJediAiActionConstraintSkillLevel(float minSkillLevel, float maxSkillLevel) {
	defaultParams.minSkillLevel = minSkillLevel;
	defaultParams.maxSkillLevel = maxSkillLevel;
	reset();
}

void CJediAiActionConstraintSkillLevel::reset() {

	// base class version
	BASECLASS::reset();

	// clear my params and data
	memset(&params, 0, sizeof(params));
	params.minSkillLevel = defaultParams.minSkillLevel;
	params.maxSkillLevel = defaultParams.maxSkillLevel;
}

EJediAiActionResult CJediAiActionConstraintSkillLevel::checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const {

	// if we are skipping this check while in progress and our action is in progress, bail
	if (action.isInProgress() && skipWhileInProgress) {
		return eJediAiActionResult_InProgress;
	}

	// if we are skipping this check while simulating and our action is simulating, bail
	if (simulating && skipWhileSimulating) {
		return eJediAiActionResult_InProgress;
	}

	// if our skill level is below our minimum threshold, we fail
	if (simMemory.selfState.skillLevel < params.minSkillLevel) {
		return eJediAiActionResult_Failure;
	}

	// if our skill level is above our maximum threshold, we fail
	if (simMemory.selfState.skillLevel > params.maxSkillLevel) {
		return eJediAiActionResult_Failure;
	}

	// constraint passed
	return eJediAiActionResult_InProgress;
}


/////////////////////////////////////////////////////////////////////////////
//
// CJediAiActionConstraintDistance
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionConstraintDistance::CJediAiActionConstraintDistance() {
	reset();
}

void CJediAiActionConstraintDistance::reset() {

	// base class version
	BASECLASS::reset();

	// clear my params and data
	memset(&params, 0, sizeof(params));
	params.maxDistance = 1e20f;
	params.belowMinResult = eJediAiActionResult_Failure;
	params.aboveMaxResult = eJediAiActionResult_Failure;
}

EJediAiActionResult CJediAiActionConstraintDistance::checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const {

	// if we are skipping this check while in progress and our action is in progress, bail
	if (action.isInProgress() && skipWhileInProgress) {
		return eJediAiActionResult_InProgress;
	}

	// if we are skipping this check while simulating and our action is simulating, bail
	if (simulating && skipWhileSimulating) {
		return eJediAiActionResult_InProgress;
	}

	// get my destination actor
	float minDistance = 0.5f;
	float maxDistance = 1e20f;
	SJediAiActorState *destActorState = lookupJediAiDestinationActorState(params.destination, simMemory, &minDistance, &maxDistance);
	if (destActorState == NULL) {
		return eJediAiActionResult_Failure;
	}

	// if I'm already close enough, succeed
	float minDistanceFromTarget = limit(minDistance, params.minDistance, maxDistance);
	if (destActorState->distanceToSelf <= minDistanceFromTarget) {
		return params.belowMinResult;
	}

	// if I'm too far away, fail
	float maxDistanceToTarget = limit(minDistance, params.maxDistance, maxDistance);
	if (destActorState->distanceToSelf > maxDistanceToTarget) {
		return params.aboveMaxResult;
	}

	// constraint passed
	return eJediAiActionResult_InProgress;
}


/////////////////////////////////////////////////////////////////////////////
//
// CJediAiActionConstraintMeleeSpaceTooCrowded
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionConstraintMeleeSpaceTooCrowded::CJediAiActionConstraintMeleeSpaceTooCrowded() {
	reset();
}

void CJediAiActionConstraintMeleeSpaceTooCrowded::reset() {

	// base class version
	BASECLASS::reset();
}

EJediAiActionResult CJediAiActionConstraintMeleeSpaceTooCrowded::checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const {

	// if we are skipping this check while in progress and our action is in progress, bail
	if (action.isInProgress() && skipWhileInProgress) {
		return eJediAiActionResult_InProgress;
	}

	// if we are skipping this check while simulating and our action is simulating, bail
	if (simulating && skipWhileSimulating) {
		return eJediAiActionResult_InProgress;
	}

	// if I don't have a victim, bail
	const SJediAiActorState *victimState = simMemory.victimState;
	if (victimState->actor == NULL) {
		return eJediAiActionResult_Failure;
	}

	// if I'm too far away from my victim, bail
	float minDistance = (kJediMeleeCorrectionTweak + simMemory.selfState.collisionRadius + victimState->collisionRadius);
	if (victimState->distanceToSelf > minDistance) {
		return eJediAiActionResult_Failure;
	}

	// if my victim is neither targeted by a player nor engaged with another jedi, bail
	if (!(victimState->flags & (kJediAiActorStateFlag_TargetedByPlayer | kJediAiActorStateFlag_EngagedWithOtherJedi))) {
		return eJediAiActionResult_Failure;
	}

	// are any of my partner jedi crowding me out?
	bool aJediIsCrowdingMeOut = false;
	for (int i = 0; i < simMemory.partnerJediStateCount; ++i) {

		// get the next partner jedi state
		const SJediAiActorState &partnerJediState = simMemory.partnerJediStates[i];
		if (partnerJediState.actor == NULL) {
			continue;
		}

		// if this jedi and I don't share victims, bail
		if (partnerJediState.victim != victimState->actor) {
			continue;
		}

		// if this jedi and I are on opposite sides of the victim, he is not crowding me out
		CVector iVictimToPartnerJediDir = victimState->wPos.xzDirectionTo(partnerJediState.wPos);
		float victimFacePartnerJediPct = victimState->iFrontDir.dotProduct(iVictimToPartnerJediDir);
		float selfAndPartnerOnSameSidePct = (victimState->faceSelfPct * victimFacePartnerJediPct);
		if (selfAndPartnerOnSameSidePct < 0.125f) {
			continue;
		}

		// if this jedi is not a player and my victim is not engaged with him, he is not crowding me out
		if (!(partnerJediState.flags & kJediAiActorStateFlag_IsPlayer)) {
			if (victimState->victim != partnerJediState.actor) {
				continue;
			}
		}

		// yeah, this guy is crowding me out
		aJediIsCrowdingMeOut = true;
		break;
	}

	// if no one was crowding me out, bail
	if (!aJediIsCrowdingMeOut) {
		return eJediAiActionResult_Failure;
	}

	// constraint passed
	return eJediAiActionResult_InProgress;
}


/////////////////////////////////////////////////////////////////////////////
//
// CJediAiActionConstraintVictimCombatType
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionConstraintVictimCombatType::CJediAiActionConstraintVictimCombatType() {
	reset();
}

void CJediAiActionConstraintVictimCombatType::reset() {

	// base class version
	BASECLASS::reset();

	// clear parameters
	memset(&params, 0, sizeof(params));
}

EJediAiActionResult CJediAiActionConstraintVictimCombatType::checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const {

	// if we are skipping this check while in progress and our action is in progress, bail
	if (action.isInProgress() && skipWhileInProgress) {
		return eJediAiActionResult_InProgress;
	}

	// if we are skipping this check while simulating and our action is simulating, bail
	if (simulating && skipWhileSimulating) {
		return eJediAiActionResult_InProgress;
	}

	// if my victim's type isn't allowed, fail
	if (!params.isCombatTypeAllowed(simMemory.victimState->combatType)) {
		return eJediAiActionResult_Failure;
	}

	// constraint passed
	return eJediAiActionResult_InProgress;
}


/////////////////////////////////////////////////////////////////////////////
//
// CJediAiActionConstraintVictimEnemyType
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionConstraintVictimEnemyType::CJediAiActionConstraintVictimEnemyType() {
	reset();
}

void CJediAiActionConstraintVictimEnemyType::reset() {

	// base class version
	BASECLASS::reset();

	// clear parameters
	memset(&params, 0, sizeof(params));
}

EJediAiActionResult CJediAiActionConstraintVictimEnemyType::checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const {

	// if we are skipping this check while in progress and our action is in progress, bail
	if (action.isInProgress() && skipWhileInProgress) {
		return eJediAiActionResult_InProgress;
	}

	// if we are skipping this check while simulating and our action is simulating, bail
	if (simulating && skipWhileSimulating) {
		return eJediAiActionResult_InProgress;
	}

	// if my victim's type isn't allowed, fail
	if (!params.isEnemyTypeAllowed(simMemory.victimState->enemyType)) {
		return eJediAiActionResult_Failure;
	}

	// constraint passed
	return eJediAiActionResult_InProgress;
}


/////////////////////////////////////////////////////////////////////////////
//
// CJediAiActionConstraintFlags
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionConstraintFlags::CJediAiActionConstraintFlags() {
	reset();
}

void CJediAiActionConstraintFlags::reset() {

	// base class version
	BASECLASS::reset();

	// clear parameters
	memset(&params, 0, sizeof(params));
}

EJediAiActionResult CJediAiActionConstraintFlags::checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const {

	// if we are skipping this check while in progress and our action is in progress, bail
	if (action.isInProgress() && skipWhileInProgress) {
		return eJediAiActionResult_InProgress;
	}

	// if we are skipping this check while simulating and our action is simulating, bail
	if (simulating && skipWhileSimulating) {
		return eJediAiActionResult_InProgress;
	}

	// if my flag configuration isn't allowed, fail
	if (params.succeedOnAnyFlags) {
		if (!params.areAnyFlagsAllowed(simMemory.victimState->flags)) {
			return eJediAiActionResult_Failure;
		}
	} else {
		if (!params.areAllFlagsAllowed(simMemory.victimState->flags)) {
			return eJediAiActionResult_Failure;
		}
	}

	// constraint passed
	return eJediAiActionResult_InProgress;
}


/////////////////////////////////////////////////////////////////////////////
//
// CJediAiActionConstraintSelfIsTooCloseToOtherJedi
//
/////////////////////////////////////////////////////////////////////////////

CJediAiActionConstraintSelfIsTooCloseToOtherJedi::CJediAiActionConstraintSelfIsTooCloseToOtherJedi() {
	reset();
}

void CJediAiActionConstraintSelfIsTooCloseToOtherJedi::reset() {

	// base class version
	BASECLASS::reset();

	// clear parameters
	memset(&params, 0, sizeof(params));
}

EJediAiActionResult CJediAiActionConstraintSelfIsTooCloseToOtherJedi::checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const {

	// if we are skipping this check while in progress and our action is in progress, bail
	if (action.isInProgress() && skipWhileInProgress) {
		return eJediAiActionResult_InProgress;
	}

	// if we are skipping this check while simulating and our action is simulating, bail
	if (simulating && skipWhileSimulating) {
		return eJediAiActionResult_InProgress;
	}

	// if my self's 'too close' flag doesn't match my desired value, fail
	if (params.desiredValue != simMemory.selfState.isTooCloseToAnotherJedi) {
		return eJediAiActionResult_Failure;
	}

	// constraint passed
	return eJediAiActionResult_InProgress;
}
