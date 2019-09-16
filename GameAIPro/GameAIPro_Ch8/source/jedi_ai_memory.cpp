#include "pch.h"
#include "jedi_ai_memory.h"
#include "jedi.h"
#include <ctime>


/////////////////////////////////////////////////////////////////////////////
//
// globals
//
/////////////////////////////////////////////////////////////////////////////

// jedi ai memory constants
static const float kCollisionCheckDistance = 15.0f;
static const float kThreatMaxAwareDistance = 50.0f;
static const float kThreatMaxAwareDuration = 10.0f;
static const float kThreatRangedAwareDistance = 100.0f;
static const float kThreatMeleeAwareDistance = 10.0f;
static const float kThreatRushAwareDistance = 50.0f;
static const float kThreatRushAwareDuration = 10.0f;

// provide a way to get the time that has elapsed since our application started
struct SAppTime {
	SAppTime() {
		appStartTime = time(NULL);
	}
	float GetTimeSinceAppStart() {
		return (float)difftime(time(NULL), appStartTime);
	}
	time_t appStartTime;
} AppTimer;

// get timestamp for this frame
// we implement this as 'time since app started', but it doesn't really matter
// as long as it progresses at the correct rate over actual time
float getTime() {
	return AppTimer.GetTimeSinceAppStart();
}

// increment a timer
static void incrementTimer(float &timer, float dt, float timerMax) {
	timer += dt;
	if (timer > timerMax) {
		timer = timerMax;
	}
}

// define a macro to get the index of an array element
// returns -1 if outside the bounds of the array
int verifyArrayIndex(int index, int arraySize) {
	return ((index < 0 || index >= arraySize) ? -1 : index);
}
#define TR_INDEXOF(_elem, _array) verifyArrayIndex(((intptr_t(&_elem) - intptr_t(&_array)) / sizeof(_elem)), TR_COUNTOF(_array))


/////////////////////////////////////////////////////////////////////////////
//
// CJediAiMemory methods
//
/////////////////////////////////////////////////////////////////////////////

CJediAiMemory::CJediAiMemory()
{
	reset();
}

CJediAiMemory::CJediAiMemory(const CJediAiMemory &copyMe)
{
	copy(copyMe);
}

void CJediAiMemory::reset() {

	// wipe everything
	memset(this, 0, sizeof(CJediAiMemory));

	// initialize our internal pointers
	victimState = gEmptyJediAiActorState;
	forceTkTargetState = gEmptyJediAiActorState;
	forceTkBestTargetState = gEmptyJediAiActorState;
	forceTkBestThrowTargetState = gEmptyJediAiActorState;

	// update our active time
	currentTime = ::getTime();
}

void CJediAiMemory::copy(const CJediAiMemory &copyMe) {

	// flat copy
	memcpy(this, &copyMe, sizeof(CJediAiMemory));

	// update state pointers
	#define FIXUP_ACTOR_STATE(state)\
		if (state != gEmptyJediAiActorState) state = (SJediAiActorState*)((intptr_t)this + ((intptr_t)copyMe.state - (intptr_t)&copyMe))
	FIXUP_ACTOR_STATE(victimState);
	FIXUP_ACTOR_STATE(forceTkTargetState);
	FIXUP_ACTOR_STATE(forceTkBestTargetState);
	FIXUP_ACTOR_STATE(forceTkBestThrowTargetState);
	#undef FIXUP_ACTOR_STATE

	// update threat attacker pointers
	for (int i = 0; i < threatStateCount; ++i) {
		SJediAiThreatState *threatState = &threatStates[i];
		SJediAiActorState *attackerState = threatState->attackerState;
		if (attackerState != NULL) {
			threatState->attackerState = &enemyStates[(intptr_t)(attackerState - copyMe.enemyStates)];
			threatState->attackerState->threatState = threatState;
		}
		SJediAiActorState *objectState = threatState->objectState;
		if (objectState != NULL) {
			threatState->objectState = &forceTkObjectStates[(intptr_t)(objectState - copyMe.forceTkObjectStates)];
			threatState->objectState->threatState = threatState;
		}
	}

	// update threat type data pointers
	for (int i = 0; i < TR_COUNTOF(threatTypeDataTable); ++i) {
		const SJediAiThreatState *threatState = threatTypeDataTable[i].shortestDistanceThreat;
		if (threatState != NULL) {
			threatTypeDataTable[i].shortestDistanceThreat = &threatStates[(intptr_t)(threatState - copyMe.threatStates)];
		}
		threatState = threatTypeDataTable[i].shortestDurationThreat;
		if (threatState != NULL) {
			threatTypeDataTable[i].shortestDurationThreat = &threatStates[(intptr_t)(threatState - copyMe.threatStates)];
		}
	}
}

void CJediAiMemory::setup(CJedi *jedi) {
	selfState.jedi = jedi;
	update(0.0f);
}

void CJediAiMemory::update(float dt) {

	// update our active time
	currentTime = ::getTime();

	// we need a self to operate
	if (selfState.jedi == NULL || (!selfState.jedi->isAiControlled())) {
		return;
	}

	// update our victim timer
	victimTimer = (victim != NULL ? victimTimer + dt : 0.0f);

	// update self state
	querySelfState();
	bool victimChanged = (victim != victimState->actor);

	// update enemy states
	queryActorStates();
	updateCanForceTkObjectsHitVictim();

	// update threat states
	queryThreatStates();

	// update victim state
	queryVictimState();

	// update force tk target states
	queryForceTkTargetStates();
}

void CJediAiMemory::simulate(float dt, const SSimulateParams &params) {

	// if our victim is still alive, increment our victim kill timer
	if (victimState->actor != NULL) {
		victimTimer += dt;
	}

	// update positional data
	if (params.wSelfPos != NULL) {
		CVector iDeltaPos = (*params.wSelfPos - selfState.wPos);
		selfState.wBoundsCenterPos += iDeltaPos;
		selfState.wPrevPos = selfState.wPos;
		selfState.wPos = *params.wSelfPos;
	}
	if (params.iSelfFrontDir != NULL) {
		selfState.iFrontDir = *params.iSelfFrontDir;
		selfState.iFrontDir.normalize();
		selfState.iRightDir = selfState.iFrontDir.crossProduct(kUnitVectorY);
		selfState.iRightDir.normalize();
	}

	// simulate our actors for this timestep
	simulateActors(dt, params);

	// simulate the threats for this timestep
	simulateThreats(dt, params);

	// if any actors died during this simulation, update our specific actors
	if (numActorsDiedDuringSimulation > 0) {

		// if my tk target died, clear it
		if (forceTkTargetState->hitPoints <= 0.0f) {
			forceTkTargetState = gEmptyJediAiActorState;
		}

		// if my best tk target died, clear it
		if (forceTkBestTargetState->hitPoints <= 0.0f) {
			forceTkBestTargetState = gEmptyJediAiActorState;
		}

		// if my best tk throw target died, clear it
		if (forceTkBestThrowTargetState->hitPoints <= 0.0f) {
			forceTkBestThrowTargetState = gEmptyJediAiActorState;
		}
	}

	// increment our simulation timer
	simulationDuration += dt;
}

void CJediAiMemory::simulateDamage(float damage, SJediAiActorState &actorState) {

	// if the actor is already dead, you can't hurt him
	if (actorState.hitPoints <= 0.0f) {
		return;
	}

	// if there is no damage, bail
	if (damage <= 0.0f) {
		return;
	}

	// if the actor is targeted by a player, mark that we've disturbed him
	if (actorState.flags & kJediAiActorStateFlag_TargetedByPlayer) {
		playerTargetDisturbedDuringSimulation = true;
	}

	// apply the damage
	// if the actor dies, update our state
	actorState.hitPoints -= damage;
	if (actorState.hitPoints <= 0.0f) {
		actorState.hitPoints = 0.0f;
		++numActorsDiedDuringSimulation;
		if (actorState.flags & kJediAiActorStateFlag_EngagedWithOtherJedi) {
			++numActorsEngagedWithOtherJediDiedDuringSimulation;
		}
	}
}

void CJediAiMemory::querySelfState() {

	// query self state
	selfState.skillLevel = selfState.jedi->getSkillLevel();
	selfState.hitPoints = selfState.jedi->getHitPoints();
	selfState.collisionRadius = selfState.jedi->getCollisionRadius();
	selfState.maxJumpForwardDistance = kJediForwardJumpMaxDistance;
	selfState.currentStateBitfield = selfState.jedi->currentStateBitfield;
	selfState.disabledActionBitfield = selfState.jedi->disabledActionAiBitfield;
	selfState.isAiControlled = selfState.jedi->isAiControlled();
	selfState.defensiveModeEnabled = selfState.jedi->isDefensiveModeEnabled();
	selfState.isTooCloseToAnotherJedi = false;

	// query self damage data
	selfState.saberDamage = (selfState.jedi->isSaberActive() ? selfState.jedi->getSaberDamage() : 0.0f);
	selfState.kickDamage = selfState.jedi->getKickDamage();
	selfState.forcePushMaxDist = selfState.jedi->getForcePushMaxDist();
	selfState.forcePushDamageRadius = selfState.jedi->getForcePushRadius();

	// query self positional data
	selfState.wPrevPos = selfState.wPos;
	selfState.wPos = selfState.jedi->getPos();
	selfState.wBoundsCenterPos = selfState.jedi->getBoundsCenter();
	selfState.iFrontDir = selfState.jedi->getFrontDir();
	selfState.iRightDir = selfState.jedi->getRightDir();

	// get my victim
	// if it changed, reset the victim timer
	CActor *prevVictim = victim;
	victim = selfState.jedi->getCurrentTarget();
	victimChanged = (victim != prevVictim);
	if (victimChanged) {
		victimTimer = 0.0f;
	}

	// can I see my victim?
	victimInView = selfState.jedi->isCurrentTargetVisible();

	// can my victim be navigated to?
	victimCanBeNavigatedTo = determinePathFindValidity(selfState.wPos, victimState->wPos);

	// query my self collisions
	querySelfCollisions();
}

void CJediAiMemory::querySelfCollisions() {
	float distance = kCollisionCheckDistance;
	CActor *actor = NULL;

	// make sure that my current collision check index is valid
	if (selfState.nearestCollisionCheckIndex < 0 || selfState.nearestCollisionCheckIndex >= eCollisionDir_Count) {
		selfState.nearestCollisionCheckIndex = 0;
	}

	// clear the result
	memset(&selfState.nearestCollisionTable[selfState.nearestCollisionCheckIndex], 0, sizeof(selfState.nearestCollisionTable[selfState.nearestCollisionCheckIndex].distance));

	// figure out which direction this check is for
	CVector iDir;
	switch (selfState.nearestCollisionCheckIndex) {
		case eCollisionDir_Left: iDir = -selfState.iRightDir; break;
		case eCollisionDir_Right: iDir = selfState.iRightDir; break;
		case eCollisionDir_Forward: iDir = selfState.iFrontDir; break;
		case eCollisionDir_Backward: iDir = -selfState.iFrontDir; break;
		case eCollisionDir_Victim: {
			if (victimState->victim == NULL) {
				return;
			}
			iDir = -victimState->iToSelfDir;
			distance = victimState->distanceToSelf - victimState->collisionRadius - selfState.collisionRadius;
		} break;
	}
	compileTimeAssert(eCollisionDir_Count == 5);

	// compute my target position
	float radius = selfState.collisionRadius * 0.5f;
	CVector wStartPos = selfState.wPos;
	CVector iDelta = iDir * (distance - radius);
	CVector wTargetPos = wStartPos + iDelta;

	// how far can I navigate in the specified direction?
	CVector wClosestNavigablePos = kZeroVector;
	bool collided = false;
	if (navMeshCollideRay(wStartPos, wTargetPos, &wClosestNavigablePos)) {
		wTargetPos = wClosestNavigablePos - (iDir * selfState.collisionRadius);
		iDelta = (wTargetPos - wStartPos);
		distance = selfState.wPos.distanceTo(wTargetPos);
		actor = NULL;
		collided = true;
	}

	// how close is the closest physical thing in the specified direction?
	if (distance > 0.0f) {
		wStartPos = selfState.wBoundsCenterPos + (iDir * radius);
		wTargetPos = wStartPos + iDelta;
		CVector wCollisionPos;
		CActor *collisionActor = NULL;
		if (collideWorldWithMovingSphere(wStartPos, radius, iDelta, &wCollisionPos, &collisionActor)) {
			wTargetPos = wCollisionPos - (iDir * selfState.collisionRadius);
			iDelta = (wTargetPos - wStartPos);
			distance = selfState.wPos.distanceTo(wTargetPos);
			actor = collisionActor;
			collided = true;
		}
	}

	// move on to the next index for next frame
	selfState.nearestCollisionTable[selfState.nearestCollisionCheckIndex].distance = distance;
	selfState.nearestCollisionTable[selfState.nearestCollisionCheckIndex].actor = actor;
	++selfState.nearestCollisionCheckIndex;

	/// debug visual
	//gEd->queueCapsule(wStartPos, wTargetPos, radius, (collided ? 0x66ff0000 : 0x6600ff00), eCollisionDir_Count);
}

bool CJediAiMemory::isSelfInState(EJediState state) const {
	return ((selfState.currentStateBitfield & (1 << state)) != 0);
}

void CJediAiMemory::setSelfInState(EJediState state, bool inState) {
	if (inState) {
		selfState.currentStateBitfield |= (1 << state);
	} else {
		selfState.currentStateBitfield &= ~(1 << state);
	}
}

bool CJediAiMemory::canSelfDoAction(EJediAction action) const {
	return ((selfState.disabledActionBitfield & (1 << action)) == 0);
}

void CJediAiMemory::updateEntityToSelfState(SJediAiEntityState &updateMe) {

	// update entity properties that are relative to my self
	updateMe.iToSelfDir = updateMe.wPos.xzDirectionTo(selfState.wPos);
	updateMe.distanceToSelf = selfState.wPos.xzDistanceTo(updateMe.wPos);
	updateMe.faceSelfPct = updateMe.iToSelfDir.dotProduct(updateMe.iFrontDir);
	updateMe.selfFacePct = (-updateMe.iToSelfDir).dotProduct(selfState.iFrontDir);
}

void CJediAiMemory::queryActorState(CActor *actor, SJediAiActorState &actorState) {

	// we need an actor for this
	if (actor == NULL) {
		memset(&actorState, 0, sizeof(actorState));
		return;
	}

	// pull the actor state from the actor
	actorState.actor = actor;
	actorState.victim = NULL;
	actorState.wPos = actor->getPos();
	actorState.wBoundsCenterPos = actor->getBoundsCenter();
	actorState.iVelocity = actor->getInertialVelocity();
	actorState.iFrontDir = actor->getFrontDir();
	actorState.iRightDir = actor->getRightDir();
	actorState.collisionRadius = actor->getCollisionRadius();

	// update the actor state relative to self
	updateEntityToSelfState(actorState);

	// get actor data
	actorState.victim = actor->getCurrentTarget();
	actorState.hitPoints = actor->getHitPoints();
	actorState.combatType = actor->getJediCombatType();
	actorState.enemyType = actor->getJediEnemyType();

	// check shield state
	if (actor->isShielded()) {
		actorState.flags |= kJediAiActorStateFlag_Shielded;
	}

	// is my self this character's victim?
	CJedi *actorVictimAsJedi = NULL;
	if (actorState.victim == selfState.jedi) {
		actorState.flags |= kJediAiActorStateFlag_TargetingSelf;

	// otherwise, is this character engaged with another jedi
	// this means that both the other jedi and the character are targeting each other
	} else if (actorState.victim->isJedi() && actorState.victim->getCurrentTarget() == actor) {
		actorState.flags |= kJediAiActorStateFlag_EngagedWithOtherJedi;
	}

	// is this actor stumbling?
	if (actor->isStumbling()) {
		actorState.flags |= kJediAiActorStateFlag_Stumbling;
		actorState.iVelocity *= 0.25f;
	}

	// is this victim incapacitated?
	if (actor->isIncapacitated()) {
		actorState.flags |= kJediAiActorStateFlag_Incapacitated;
		actorState.iVelocity.zero();
	}

	// can this biped be taunted?
	if (actor->canBeTaunted()) {
		actorState.flags |= kJediAiActorStateFlag_CanBeTaunted;
	}

	// if this is a jedi, retrieve jedi-specific info
	if (actor->isJedi()) {
		CJedi *jedi = (CJedi*)actor;

		// is this jedi a player?
		if (!jedi->isAiControlled()) {
			actorState.flags |= kJediAiActorStateFlag_IsPlayer;
		}

		// is my self inside this jedi?
		if (actorState.distanceToSelf < ((actorState.collisionRadius + selfState.collisionRadius) * 2.0f)) {
			selfState.isTooCloseToAnotherJedi = true;
		}
	}

	// is this actor force grippable?
	if (actor->isForceGrippable()) {
		if (actorState.distanceToSelf < kJediForceSelectRange) {
			if (actorState.hitPoints > 0.0f) {
				actorState.flags |= kJediAiActorStateFlag_Grippable;
			}
		}
	}

	// is this actor gripped already?
	if (actor->isForceGripped()) {
		actorState.flags |= kJediAiActorStateFlag_Grippable;
		actorState.flags |= kJediAiActorStateFlag_Gripped;
	}

	// is this actor gripped by my self?
	if (actor->isBeingForceGrippedByJedi(selfState.jedi)) {
		actorState.flags |= kJediAiActorStateFlag_Grippable;
		actorState.flags |= kJediAiActorStateFlag_Gripped;
		actorState.flags |= kJediAiActorStateFlag_GrippedBySelf;
	}

	// update our 'throwable' flag
	updateActorStateThrowableFlag(actorState);

	// is this actor dead?
	if (actorState.hitPoints <= 0.0f) {
		actorState.flags |= kJediAiActorStateFlag_Dead;
	}
}

void CJediAiMemory::updateActorStateThrowableFlag(SJediAiActorState &actorState) const {

	// can I throw this actor?
	switch (actorState.enemyType) {

		// these types are not throwable
		case eJediEnemyType_TrandoshanMelee:
		case eJediEnemyType_TrandoshanConcussive:
		case eJediEnemyType_B2BattleDroid:
		case eJediEnemyType_B2RocketDroid: {
			actorState.flags &= ~kJediAiActorStateFlag_Throwable;
		} break;

		// these types are only throwable while stumbling
		case eJediEnemyType_TrandoshanCommando:
		case eJediEnemyType_B1MeleeDroid: {
			if ((actorState.flags & kJediAiActorStateFlag_Stumbling) || (actorState.flags & kJediAiActorStateFlag_GrippedBySelf)) {
				actorState.flags |= kJediAiActorStateFlag_Throwable;
			} else {
				actorState.flags &= ~kJediAiActorStateFlag_Throwable;
			}
		} break;

		// these types are only throwable while rolling (rush attack)
		case eJediEnemyType_Droideka: {
			if ((actorState.flags & kJediAiActorStateFlag_InRushAttack) || (actorState.flags & kJediAiActorStateFlag_GrippedBySelf)) {
				actorState.flags |= kJediAiActorStateFlag_Throwable;
			} else {
				actorState.flags &= ~kJediAiActorStateFlag_Throwable;
			}
		} break;

		// all other types are throwable
		default: {
			actorState.flags |= kJediAiActorStateFlag_Throwable;
		} break;
	}
}

void CJediAiMemory::queryVictimState() {

	// clear victim state
	victimState = gEmptyJediAiActorState;
	victimDesiredKillTime = 0.0f;

	// we need a victim to do anything
	if (victim == NULL) {
		return;
	}

	// if our victim is one of our enemies, use it
	// otherwise, use our buffer
	victimState = findEnemyState(victim);

	// query the victim state
	queryActorState(victim, *victimState);

	// get the victim's combat type
	victimState->combatType = victim->getJediCombatType();

	// get the victim's enemy type
	victimState->enemyType = victim->getJediEnemyType();

	// compute how long it should take to kill this victim
	victimDesiredKillTime = computeTimeToKill(selfState.jedi, victimState->enemyType, selfState.skillLevel);

	// get the victim's floor height
	victimFloorHeight = victim->getFloorHeight();
}

void CJediAiMemory::updateVictimToSelfState() {

	// set the victim state
	if (victimState->actor != NULL) {
		updateEntityToSelfState(*victimState);
	} else {
		victimState->iToSelfDir.zero();
		victimState->distanceToSelf = 0.0f;
		victimState->faceSelfPct = 0.0f;
		victimState->selfFacePct = 0.0f;
	}
}

void CJediAiMemory::queryForceTkTargetStates() {

	// compute my force tk target state
	forceTkTargetState = gEmptyJediAiActorState;
	if (selfState.jedi->getForceSelectedActor() != NULL) {
		SJediAiActorState *forceSelectedActorState = findEnemyState(selfState.jedi->getForceSelectedActor());
		if (forceSelectedActorState != NULL) {
			forceTkTargetState = forceSelectedActorState;
		} else {
			queryActorState(selfState.jedi->getForceSelectedActor(), *forceTkTargetState);
		}
	}

	// query the best force tk target state
	queryForceTkBestTargetState();

	// query the best force tk throw target state
	queryForceTkBestThrowTargetState();
}

void CJediAiMemory::queryForceTkBestTargetState() {

	// if we have a tk target, just copy that and return
	if (forceTkTargetState->actor != NULL) {
		forceTkBestTargetState = forceTkTargetState;
		return;
	}

	// if there are any tk-able enemies nearby that I can see, choose one
	for (int i = 0; i < enemyStateCount; ++i) {
		SJediAiActorState &actorState = enemyStates[i];
		int desiredFlags = (kJediAiActorStateFlag_Grippable | kJediAiActorStateFlag_Throwable);
		if ((actorState.flags & desiredFlags) == desiredFlags) {
			if (!(actorState.flags & (kJediAiActorStateFlag_TargetedByPlayer | kJediAiActorStateFlag_EngagedWithOtherJedi))) {
				if (actorState.selfFacePct > 0.25f) {
					forceTkBestTargetState = &actorState;
					return;
				}
			}
		}
	}

	// if there are any tk-able objects nearby, choose one
	for (int i = 0; i < forceTkObjectStateCount; ++i) {
		SJediAiActorState &actorState = forceTkObjectStates[i];
		int desiredFlags = (kJediAiActorStateFlag_Grippable | kJediAiActorStateFlag_ForceTkObjectCanHitVictim);
		if ((actorState.flags & desiredFlags) == desiredFlags) {
			forceTkBestTargetState = &actorState;
			return;
		}
	}

	// there is no one around to force tk
	forceTkTargetState = gEmptyJediAiActorState;
}

void CJediAiMemory::queryForceTkBestThrowTargetState() {

	// clear all of my force tk target data
	forceTkBestThrowTargetState = gEmptyJediAiActorState;

	// if I have nothing to throw, don't look for things to throw at
	if (forceTkBestTargetState->actor == NULL) {
		return;
	}

	// if I am not throwing my victim and I have a victim within throw range, throw at my victim
	if (forceTkBestTargetState->actor != victimState->actor) {
		if (victimState->actor != NULL) {
			if (!(victimState->flags & (kJediAiActorStateFlag_TargetedByPlayer | kJediAiActorStateFlag_EngagedWithOtherJedi))) {
				float distSq = victimState->wBoundsCenterPos.distanceSqTo(forceTkBestTargetState->wBoundsCenterPos);
				if (distSq < SQ(kJediThrowRange)) {
					forceTkBestTargetState = victimState;
					return;
				}
			}
		}
	}

	// minimum distance
	const float kMinDistanceSq = SQ(10.0f);

	// set the actor states
	forceTkBestThrowTargetState = gEmptyJediAiActorState;
	float bestEnemyDistanceSq =  SQ(9999.0f);
	for (int i = 0; i < enemyStateCount; ++i) {
		SJediAiActorState &actorState = enemyStates[i];
		if (actorState.actor == NULL) {
			continue;
		}

		// we can't throw a target at itself
		if (actorState.actor == forceTkBestTargetState->actor) {
			continue;
		}

		// if this actor is targeted by a player, skip it
		// if this actor is engaged with another jedi, skip it
		// if this target is incapacitated, skip it
		unsigned int skipItFlags = (
			kJediAiActorStateFlag_TargetedByPlayer |
			kJediAiActorStateFlag_EngagedWithOtherJedi |
			kJediAiActorStateFlag_Incapacitated
		);
		if (actorState.flags & skipItFlags) {
			continue;
		}

		// if this actor is gripped by someone other than me, skip it
		if ((actorState.flags & kJediAiActorStateFlag_Gripped) && !(actorState.flags & kJediAiActorStateFlag_GrippedBySelf)) {
			continue;
		}

		// if this target is not too close and is closer to my tk target than the last actor, use it instead
		float distanceSq = forceTkBestTargetState->wPos.distanceSqTo(actorState.wPos);
		if (distanceSq >= kMinDistanceSq && distanceSq < bestEnemyDistanceSq) {
			bestEnemyDistanceSq = distanceSq;
			forceTkBestThrowTargetState = &actorState;
		}
	}
}

void CJediAiMemory::updateForceTkTargetToSelfStates() {

	// update forceTkTarget properties that are relative to my self
	updateEntityToSelfState(*forceTkTargetState);
	updateEntityToSelfState(*forceTkBestTargetState);
	updateEntityToSelfState(*forceTkBestThrowTargetState);
}

int CJediAiMemory::findEnemyStateIndex(CActor *enemy) const {

	// find the index of this enemy's state
	for (int i = 0; i < enemyStateCount; ++i) {
		if (enemyStates[i].actor == enemy) {
			return i;
		}
	}
	return -1;
}

SJediAiActorState *CJediAiMemory::findEnemyState(CActor *enemy) {
	int enemyStateIndex = findEnemyStateIndex(enemy);
	return (enemyStateIndex < 0 ? NULL : &enemyStates[enemyStateIndex]);
}

void CJediAiMemory::clearCanForceTkObjectsHitVictim() {

	// reset the index that we check
	canForceTkObjectHitVictimCheckIndex = 0;

	// clear the 'canHit' flags
	for (int i = 0; i < forceTkObjectStateCount; ++i) {
		forceTkObjectStates[i].flags &= ~kJediAiActorStateFlag_ForceTkObjectCanHitVictim;
	}
}

void CJediAiMemory::updateCanForceTkObjectsHitVictim() {

	// if I have no items or victim, bail
	if (forceTkObjectStateCount <= 0 || victimState->actor == NULL) {
		clearCanForceTkObjectsHitVictim();
		return;
	}

	// keep my check index within bounds
	if (canForceTkObjectHitVictimCheckIndex >= forceTkObjectStateCount || canForceTkObjectHitVictimCheckIndex < 0) {
		canForceTkObjectHitVictimCheckIndex = 0;
	}

	// check whether or not the current 'check' object can hit my victim
	SJediAiActorState &forceTkObjectState = forceTkObjectStates[canForceTkObjectHitVictimCheckIndex];
	float radius = forceTkObjectState.collisionRadius;
	CVector wStartPos = forceTkObjectState.wPos + (kUnitVectorY * (radius + 0.1f));
	CVector wEndPos = victimState->wPos + (kUnitVectorY * (radius + 0.1f));
	CVector iDir = wStartPos.directionTo(wEndPos);
	wStartPos += (iDir * radius);
	wEndPos -= (iDir * radius);
	CVector iDelta = wEndPos - wStartPos;
	bool canHitVictim = !collideWorldWithMovingSphere(wStartPos, radius, iDelta, NULL);
	if (canHitVictim) {
		forceTkObjectState.flags |= kJediAiActorStateFlag_ForceTkObjectCanHitVictim;
	} else {
		forceTkObjectState.flags &= ~kJediAiActorStateFlag_ForceTkObjectCanHitVictim;
	}

	// move to the next index
	++canForceTkObjectHitVictimCheckIndex;
	if (canForceTkObjectHitVictimCheckIndex >= forceTkObjectStateCount) {
		canForceTkObjectHitVictimCheckIndex = 0;
	}
}


int CJediAiMemory::findForceTkObjectStateIndex(CActor *object) const {
	for (int i = 0; i < forceTkObjectStateCount; ++i) {
		if (forceTkObjectStates[i].actor == object) {
			return i;
		}
	}
	return -1;
}

SJediAiActorState *CJediAiMemory::findForceTkObjectState(CActor *object) {
	int objectStateIndex = findForceTkObjectStateIndex(object);
	return (objectStateIndex < 0 ? NULL : &forceTkObjectStates[objectStateIndex]);
}

bool findActorsInVicinityCallback(CActor *actor, void *context) {
	CJediAiMemory *memory = (CJediAiMemory*)context;

	// skip my self
	if (actor == memory->selfState.jedi) {
		return false;
	}

	// if this actor is not alive, skip it
	if (actor->getHitPoints() <= 0.0f) {
		return false;
	}

	// force tk objects
	if (actor->isForceTkObject()) {

		// if this object isn't grippable, skip it
		if (!actor->isForceGrippable()) {
			return false;
		}

		// if the object isn't in my view, skip it
		CVector iSelfToObjectDir = memory->selfState.wPos.xzDirectionTo(actor->getPos());
		float selfFacePct = memory->selfState.iFrontDir.dotProduct(iSelfToObjectDir);
		if (selfFacePct < 0.15f) {
			return false;
		}

		// keep this object
		return true;
	}

	// partner jedi
	if (actor->isJedi()) {
		CJedi *jedi = (CJedi*)actor;

		// if this jedi is a padawan, skip it
		if (jedi->isPadawan()) {
			return false;
		}

		// keep this jedi
		return true;
	}

	// always keep enemies
	if (actor->isJediEnemy()) {
		return true;
	}

	// skip this actor
	return false;
}

void CJediAiMemory::queryActorStates() {

	// save off whether or not a given forceTkObject can hit my victim
	struct {
		CActor *object;
		bool canHitVictim;
	} canHitVictimList[TR_COUNTOF(forceTkObjectStates)];
	int canHitVictimCount = 0;
	for (int i = 0; i < forceTkObjectStateCount; ++i) {
		canHitVictimList[canHitVictimCount].object = forceTkObjectStates[i].actor;
		canHitVictimList[canHitVictimCount++].canHitVictim = ((forceTkObjectStates[i].flags & kJediAiActorStateFlag_ForceTkObjectCanHitVictim) != 0);
	}

	// clear current states
	partnerJediStateCount = 0;
	memset(partnerJediStates, 0, sizeof(partnerJediStates));
	enemyStateCount = 0;
	memset(enemyStates, 0, sizeof(enemyStates));
	forceTkObjectStateCount = 0;
	memset(forceTkObjectStates, 0, sizeof(forceTkObjectStates));

	// find all nearby actors
	CActor *actorList[256];
	int actorCount = findActorsInVicinity(selfState.wPos, 100.0f, actorList, TR_COUNTOF(actorList), findActorsInVicinityCallback, this);
	if (actorCount <= 0) {
		return;
	}

	// update our actor state lists
	for (int i = 0; i < actorCount; ++i) {

		// calculate the distance to the actor
		CActor *actor = actorList[i];
		CVector wActorPos = actor->getPos();

		// determine which list this actor will go into
		SJediAiActorState *list;
		int *count;
		int listSize;

		// this is a force tk object
		if (actor->isForceTkObject()) {
			list = forceTkObjectStates;
			count = &forceTkObjectStateCount;
			listSize = TR_COUNTOF(forceTkObjectStates);

		// this is a partner jedi
		} else if (actor->isJedi()) {
			list = partnerJediStates;
			count = &partnerJediStateCount;
			listSize = TR_COUNTOF(partnerJediStates);

		// this is an enemy
		} else if (actor->isJediEnemy()) {
			list = enemyStates;
			count = &enemyStateCount;
			listSize = TR_COUNTOF(enemyStates);
		}

		// how close is this actor to my self
		float distanceToSelfSq = selfState.wPos.distanceSqTo(wActorPos);

		// insert this threat into the list, sorting by distance
		int insertIdx = *count;
		while (insertIdx > 0) {

			// if I've reached my place in the sorted list, break
			// NOTE: while we are finding the closest actors to store in our lists,
			// state.distanceToSelf actually stores the distanceToSelfSq, for speed
			SJediAiActorState &prevState = list[insertIdx - 1];
			if (distanceToSelfSq >= prevState.distanceToSelf) {
				break;
			}

			// move the previous state to make room for the new guy
			if (insertIdx < listSize) {
				list[insertIdx].actor = prevState.actor;
				list[insertIdx].distanceToSelf = prevState.distanceToSelf;
			} else {
				--(*count);
			}
			--insertIdx;
		}

		// if the list has no room for this actor, skip it
		if (insertIdx >= listSize) {
			continue;
		}

		// insert the actor state into the list
		SJediAiActorState &actorState = list[insertIdx];
		actorState.actor = actor;
		actorState.distanceToSelf = distanceToSelfSq;
		++(*count);
	}

	// query our partner jedi states
	for (int i = 0; i < partnerJediStateCount; ++i) {
		SJediAiActorState &actorState = partnerJediStates[i];
		CActor *actor = actorState.actor;
		queryActorState(actor, actorState);
	}

	// query our enemy states
	for (int i = 0; i < enemyStateCount; ++i) {
		SJediAiActorState &actorState = enemyStates[i];
		CActor *actor = actorState.actor;
		queryActorState(actor, actorState);
	}

	// query our forceTkObject states
	for (int i = 0; i < forceTkObjectStateCount; ++i) {
		SJediAiActorState &actorState = forceTkObjectStates[i];
		CActor *actor = actorState.actor;
		queryActorState(actor, actorState);
	}

	// set whether or not we can hit our victim with these objects
	for (int i = 0; i < forceTkObjectStateCount; ++i) {
		for (int j = 0; j < canHitVictimCount; ++j) {
			if (canHitVictimList[j].object == forceTkObjectStates[i].actor) {
				if (canHitVictimList[j].canHitVictim) {
					forceTkObjectStates[i].flags |= kJediAiActorStateFlag_ForceTkObjectCanHitVictim;
				} else {
					forceTkObjectStates[i].flags &= ~kJediAiActorStateFlag_ForceTkObjectCanHitVictim;
				}
			}
		}
	}
}

void CJediAiMemory::updateActorToSelfStates() {

	// actor state lists
	struct { SJediAiActorState *list; int count; } stateLists[] = {
		{ partnerJediStates, partnerJediStateCount },
		{ enemyStates, enemyStateCount },
		{ forceTkObjectStates, forceTkObjectStateCount },
	};

	// set the actor states
	for (int i = 0; i < TR_COUNTOF(stateLists); ++i) {
		SJediAiActorState *list = stateLists[i].list;
		int count = stateLists[i].count;
		for (int j = 0; j < count; ++j) {
			updateEntityToSelfState(list[j]);
		}
	}
}

void CJediAiMemory::simulateActor(float dt, const CJediAiMemory::SSimulateParams &params, SJediAiActorState &actorState) {

	// did we update my positional data this frame?
	bool noUpdatedPositionalData = (params.wSelfPos == NULL) && (params.iSelfFrontDir == NULL);

	// if the actor is gripped with force tk, it won't move
	if (actorState.flags & kJediAiActorStateFlag_Gripped) {
		actorState.iVelocity.zero();
		if (actorState.threatState != NULL) {
			actorState.threatState->iVelocity.zero();
		}
	}

	// if the actor is in a rush attack, make his velocity move straight toward me
	if (actorState.flags & kJediAiActorStateFlag_InRushAttack) {

		// if the actor has a rush threat, just use its velocity
		if (actorState.threatState != NULL && actorState.threatState->type == eJediThreatType_Rush) {
			actorState.iVelocity = actorState.threatState->iVelocity;

		// if there was no rush threat, just set my velocity toward my victim
		} else {
			float speed = actorState.iVelocity.length();
			if (speed < 1.0f) {
				speed = 1.0f;
			}
			const CVector &iToSelfDir = (params.wSelfPos == NULL ? actorState.iToSelfDir : actorState.wPos.xzDirectionTo(selfState.wPos));
			actorState.iVelocity = (iToSelfDir * speed);
		}
	}

	// certain threat states override our actor velocity
	if (actorState.threatState != NULL) {

		// if this is a trandoshan flutterpack in kamikaze mode, use the threat's velocity
		if (actorState.threatState->type == eJediThreatType_Explosion && actorState.enemyType == eJediEnemyType_TrandoshanFlutterpack) {
			actorState.iVelocity = actorState.threatState->iVelocity;

		// if this actor is performing a melee attack, assume it won't move
		} else if (actorState.threatState->type == eJediThreatType_Melee) {
			actorState.iVelocity.zero();
		}
	}

	// update our 'throwable' flag
	updateActorStateThrowableFlag(actorState);

	// if this actor didn't move and we didn't move, skip it
	bool noVelocity = actorState.iVelocity.isCloseTo(kZeroVector, 0.001f);
	if (noVelocity) {
		if (noUpdatedPositionalData) {
			return;
		}

	// if it did move, update it's position
	} else {

		// compute our position delta
		CVector iDelta = actorState.iVelocity * dt;
		float deltaLengthSq = iDelta.lengthSq();

		// if this actor is a grenade, make it stop on its end pos
		if (actorState.threatState != NULL && &actorState == actorState.threatState->objectState && actorState.threatState->type == eJediThreatType_Grenade) {
			CVector wEndPos = actorState.threatState->wEndPos;
			float distanceToEndPosSq = actorState.wPos.distanceSqTo(wEndPos);
			if (distanceToEndPosSq <= deltaLengthSq) {
				iDelta = (wEndPos - actorState.wPos);
				actorState.iVelocity.zero();
				actorState.threatState->iVelocity.zero();
				actorState.threatState->wBoundsCenterPos -= actorState.threatState->wPos;
				actorState.threatState->wPos = wEndPos;
				actorState.threatState->wBoundsCenterPos += actorState.threatState->wPos;
				updateEntityToSelfState(*actorState.threatState);
			}

		// otherwise, if the actor was moving toward me, don't let it go through me
		} else {
			float distToSelf = selfState.wPos.xzDistanceTo(actorState.wPos);
			float maxDeltaLength = distToSelf - actorState.collisionRadius - selfState.collisionRadius;
			if (maxDeltaLength < 0.0f) {
				maxDeltaLength = 0.0f;
			}
			if (actorState.faceSelfPct > 0.8f && SQ(maxDeltaLength) < deltaLengthSq) {
				iDelta.setLength(maxDeltaLength);
			}
		}

		// update the actor position
		actorState.wPos += iDelta;
		actorState.wBoundsCenterPos += iDelta;
	}

	// was this actor targeted by a player and disturbed during this sim step?
	if (!playerTargetDisturbedDuringSimulation && (actorState.flags & kJediAiActorStateFlag_TargetedByPlayer)) {
		if (
			(actorState.flags & kJediAiActorStateFlag_GrippedBySelf) ||
			(actorState.flags & kJediAiActorStateFlag_Stumbling) ||
			(actorState.flags & kJediAiActorStateFlag_Incapacitated) ||
			(actorState.flags & kJediAiActorStateFlag_Dead)
		) {
			playerTargetDisturbedDuringSimulation = true;
		}
	}

	// update the actor's positional data in relation to me
	updateEntityToSelfState(actorState);
}

void CJediAiMemory::simulateActors(float dt, const SSimulateParams &params) {

	// special actor states
	SJediAiActorState *specialActorStates[] = {
		victimState,
		forceTkTargetState,
		(forceTkBestTargetState == victimState || forceTkBestTargetState == forceTkTargetState ? NULL : forceTkBestTargetState),
		forceTkBestThrowTargetState,
	};

	// update special actor states
	for (int i = 0; i < TR_COUNTOF(specialActorStates); ++i) {

		// get the next valid state
		if (specialActorStates[i] == NULL) {
			continue;
		}
		SJediAiActorState &actorState = *specialActorStates[i];
		if (actorState.actor == NULL) {
			continue;
		}

		// if this state is in one of the normal lists, skip it
		if (TR_INDEXOF(actorState, enemyStates) != -1) {
			continue;
		}
		if (TR_INDEXOF(actorState, partnerJediStates) != -1) {
			continue;
		}
		if (TR_INDEXOF(actorState, forceTkObjectStates) != -1) {
			continue;
		}

		// simulate this actor
		simulateActor(dt, params, actorState);
	}

	// actor state lists
	struct { SJediAiActorState *list; int count; } stateLists[] = {
		{ partnerJediStates, partnerJediStateCount },
		{ enemyStates, enemyStateCount },
		{ forceTkObjectStates, forceTkObjectStateCount },
	};

	// set the actor states
	for (int i = 0; i < TR_COUNTOF(stateLists); ++i) {
		SJediAiActorState *list = stateLists[i].list;
		int count = stateLists[i].count;
		for (int j = 0; j < count; ++j) {
			SJediAiActorState &actorState = list[j];
			if (actorState.actor == NULL) {
				continue;
			}

			// simulate this actor
			simulateActor(dt, params, actorState);
		}
	}

	// is my self inside jedi?
	selfState.isTooCloseToAnotherJedi = false;
	for (int i = 0; i < partnerJediStateCount; ++i) {
		SJediAiActorState &actorState = partnerJediStates[i];
		if (actorState.distanceToSelf < ((actorState.collisionRadius + selfState.collisionRadius) * 2.0f)) {
			selfState.isTooCloseToAnotherJedi = true;
		}
	}
}

void CJediAiMemory::queryThreatStates() {

	// reset the output lists
	threatLevel = 0.0f;
	threatStateCount = 0;
	memset(threatStates, 0, sizeof(threatStates));
	memset(threatTypeDataTable, 0, sizeof(threatTypeDataTable));
	recommendedDeflectionDuration = 0.0f;
	recommendedCrouchDuration = 0.0f;
	nextRushThreatDuration = 0.0f;
	highestBlockableThreatLevel = 0.0f;
	memset(blockDirThreatInfoTable, 0, sizeof(blockDirThreatInfoTable));
	highestDodgeableThreatLevel = 0.0f;
	memset(dodgeDirThreatLevelTable, 0, sizeof(dodgeDirThreatLevelTable));

	// look through the threat list for threats targeting me
	float threatLevels[TR_COUNTOF(threatStates)] = {};
	for (int i = 0; i < gThreatCount; ++i) {

		// get the threat
		SJediThreatInfo *threat = &gThreatList[i];
		if (threat == NULL) {
			continue;
		}

		// if this threat does no damage, ignore it
		if (threat->strength <= 0.0f) {
			continue;
		}

		// get the attacker
		CActor *attacker = threat->creator;
		if (attacker == NULL) {
			continue;
		}

		// if this threat is too far from me, ignore it
		float threatDistSq = threat->wPos.distanceSqTo(selfState.wPos);
		if (threatDistSq > SQ(kThreatMaxAwareDistance)) {
			continue;
		}

		// query some information about this threat
		SJediAiThreatState threatState;
		memset(&threatState, 0, sizeof(threatState));
		threatState.threat = threat;
		threatState.type = threat->type;
		threatState.wPos = threat->wPos;
		threatState.wEndPos = threat->wEndPos;
		threatState.wBoundsCenterPos = threatState.wPos;
		threatState.iFrontDir = threat->iDir;
		if ((threatState.type == eJediThreatType_Melee) || (threatState.iFrontDir.isCloseTo(kZeroVector, 0.001f))) {
			threatState.iFrontDir = attacker->getFrontDir();
			if (threatState.iFrontDir.isCloseTo(kZeroVector, 0.001f)) {
				threatState.iFrontDir = kUnitVectorZ;
			}
		}
		threatState.iFrontDir.normalize();
		threatState.iVelocity = threat->iDir * threat->speed;
		threatState.iRightDir = threatState.iFrontDir.crossProduct(kUnitVectorY);
		threatState.iRightDir.normalize();
		threatState.attackerState = findEnemyState(attacker);
		threatState.objectState = (threat->object->isForceTkObject() ? findForceTkObjectState(threat->object) : NULL);
		threatState.duration = threat->delayToAttackTime;
		threatState.strength = threat->strength;
		threatState.damageRadius = threat->damageRadius;
		threatState.attackLevel = threat->attackLevel;
		if (threatState.threat->isMelee360) {
			threatState.flags |= kJediAiThreatStateFlag_Melee360;
		}
		updateThreatToSelfState(threatState);

		// if this threat is too long from now, ignore it
		if (threatState.duration > kThreatMaxAwareDuration) {
			continue;
		}

		// for forward facing threats, what is the minimum face pct I am threatened by?
		CVector iThreatCollisionExtentDelta = threatState.iToSelfDir.crossProduct(kUnitVectorY) * (selfState.collisionRadius * 1.5f);
		CVector iThreatCollisionExtentDir = threatState.wPos.directionTo(selfState.wPos + iThreatCollisionExtentDelta);
		float minFaceSelfPct = threatState.iToSelfDir.dotProduct(iThreatCollisionExtentDir);

		// handle each damage type specifically
		switch (threatState.type) {

			// skip unknown types
			default: continue;

			// blaster threats
			case eJediThreatType_Blaster: {

				// if the person who shot this blaster bolt is not my enemy, ignore it
				if (!attacker->isJediEnemy()) {
					continue;
				}

				// if I am too far away to notice this threat, ignore it
				if (threatState.distanceToSelf > kThreatRangedAwareDistance) {
					continue;
				}

				// if this threat isn't facing me enough, ignore it
				if (threatState.faceSelfPct < minFaceSelfPct) {
					continue;
				}

			} break;

			// melee threats
			case eJediThreatType_Melee: {

				// if the attacker is dead or incapacitated, ignore this threat
				if (attacker->getHitPoints() <= 0 || attacker->isIncapacitated()) {
					continue;
				}

				// if the attacker is targeting someone besides me, ignore it
				if (threat->intendedVictim != NULL && threat->intendedVictim != selfState.jedi) {
					continue;
				}

				// if I am too far away to notice this threat, ignore it
				if (threatState.distanceToSelf > kThreatMeleeAwareDistance) {
					continue;
				}

				// if I am far enough outside the threat's damage radius, ignore it
				float threatDamageDist = threatState.distanceToSelf - threatState.damageRadius;
				if (threatDamageDist > selfState.collisionRadius) {
					continue;
				}

				// is this attack horizontal?
				CVector originDir = threat->iDir;
				if (originDir.x != 0.0f) {
					threatState.flags |= kJediAiThreatStateFlag_Horizontal;
				} else {
					threatState.flags &= ~kJediAiThreatStateFlag_Horizontal;
				}

				// if this isn't a 360 melee attack, make sure that me attacker is facing me enough
				if (!(threatState.flags & kJediAiThreatStateFlag_Melee360)) {

					// how much is the attacker facing me?
					// if I have information on the attacker, just use that to save time
					float attackerFacePct;
					if (threatState.attackerState != NULL) {
						attackerFacePct = threatState.attackerState->faceSelfPct;
					} else {
						CVector wAttackerPos = attacker->getPos();
						CVector iAttackerDir = attacker->getFrontDir();
						CVector iAttackerToJediDir = wAttackerPos.xzDirectionTo(selfState.wPos);
						attackerFacePct = iAttackerDir.dotProduct(iAttackerToJediDir);
					}

					// how much must the attacker be facing me?
					float minAttackerFacePct;
					if (threatState.flags & kJediAiThreatStateFlag_Horizontal) {
						minAttackerFacePct = 0.0f;
					} else {
						minAttackerFacePct = minFaceSelfPct;
					}

					// if the attacker isn't facing me enough, ignore the threat
					if (attackerFacePct < minAttackerFacePct) {
						continue;
					}
				}

			} break;

			// tackle threats
			case eJediThreatType_Rush: {

				// if the attacker is dead or incapacitated, ignore this threat
				if (attacker->getHitPoints() <= 0 || attacker->isIncapacitated()) {
					continue;
				}

				// if this threat is headed away from me, ignore it
				if (threatState.distanceToSelf > 20.0f) {
					CVector iThreatMoveDir = threatState.iVelocity;
					iThreatMoveDir.normalize();
					float toMePct = iThreatMoveDir.dotProduct(threatState.iToSelfDir);
					if (toMePct < 0.5f) {
						continue;
					}
				}

				// if the attacker is targeting someone besides me, ignore it
				if (threat->intendedVictim != NULL && threat->intendedVictim != selfState.jedi) {
					continue;
				}

				// if I am too far away to notice this threat, ignore it
				if (threatState.distanceToSelf > kThreatRushAwareDistance) {
					continue;
				}

				// if this threat is too long away, ignore it
				if (threatState.duration > kThreatRushAwareDuration) {
					continue;
				}

			} break;

			// rocket threats
			case eJediThreatType_Rocket: {

				// if the person who threw this rocket is not an enemy, ignore it
				if (!attacker->isJediEnemy()) {
					continue;
				}

				// if this threat isn't facing me enough, ignore it
				if (threatState.faceSelfPct < minFaceSelfPct) {
					continue;
				}

			} break;

			// grenade threats
			case eJediThreatType_Grenade: {

				// if the person who threw this grenade is not an enemy and it is moving away from me, ignore it
				if (!attacker->isJediEnemy() && threatState.objectState != NULL) {
					CVector iMoveDir = threatState.objectState->iVelocity;
					iMoveDir.normalize();
					float movingTowardPct = iMoveDir.dotProduct(threatState.iToSelfDir);
					if (movingTowardPct < 0.5f) {
						continue;
					}
				}

			} break;

			// explosion threats
			case eJediThreatType_Explosion: {
			} break;
		}

		// compute the threat level
		float threatLevel = computeThreatStateLevel(threatState);

		// insert this threat into the list, sorting by distance
		int insertIdx = threatStateCount;
		while (insertIdx > 0) {

			// if I've reached my place in the sorted list, break
			SJediAiThreatState &prevThreatState = threatStates[insertIdx - 1];
			if (threatState.distanceToSelf >= prevThreatState.distanceToSelf) {
				break;
			}

			// move the previous threat to make room for the new guy
			// if there isn't any room for the previous threat, remove it's type from the type count table
			if (insertIdx < TR_COUNTOF(threatStates)) {
				threatStates[insertIdx] = prevThreatState;
				threatLevels[insertIdx] = threatLevels[insertIdx - 1];
			} else {
				--threatStateCount;
			}
			--insertIdx;
		}

		// insert the new threat into it's place in the list
		if (insertIdx < TR_COUNTOF(threatStates)) {
			threatStates[insertIdx] = threatState;
			threatLevels[insertIdx] = threatLevel;
			++threatStateCount;
		}
	}

	// analyze the threats we found
	for (int i = 0; i < threatStateCount; ++i) {
		SJediAiThreatState &threatState = threatStates[i];

		// if we have an attacker state and this threat has less duration than its current threat,
		// set this threat as the attacker's threat
		if (threatState.attackerState != NULL) {
			if (threatState.attackerState->threatState == NULL || threatState.attackerState->threatState->duration > threatState.duration) {
				threatState.attackerState->threatState = &threatState;
			}
			if (threatState.type == eJediThreatType_Rush && threatState.attackerState != NULL) {
				threatState.attackerState->flags |= kJediAiActorStateFlag_InRushAttack;
			}
		}

		// if we have an object state and this threat has less duration than its current threat,
		// set this threat as the object's threat
		if (threatState.objectState != NULL) {
			if (threatState.objectState->threatState == NULL || threatState.objectState->threatState->duration > threatState.duration) {
				threatState.objectState->threatState = &threatState;
			}
		}

		// update our aggregate threat level
		float threatStateThreatLevel = threatLevels[i];
		threatLevel += threatStateThreatLevel;

		// if I'm facing this threat enough, update our recommended deflection duration
		if (threatState.type == eJediThreatType_Blaster && threatState.selfFacePct > 0.25f) {
			if (recommendedDeflectionDuration < threatState.duration) {
				recommendedDeflectionDuration = threatState.duration;
			}
		}

		// update our recommended crouch duration
		if ((threatState.flags & kJediAiThreatStateFlag_Horizontal) && recommendedCrouchDuration < threatState.duration) {
			recommendedCrouchDuration = threatState.duration;
		}

		// update our incoming tackle delay time
		if (threatState.type == eJediThreatType_Rush && nextRushThreatDuration < threatState.duration) {
			nextRushThreatDuration = threatState.duration;
		}

		// update our block dir threat info
		if (threatState.blockDir != eJediBlockDir_None) {
			if (highestBlockableThreatLevel < threatStateThreatLevel) {
				highestBlockableThreatLevel = threatStateThreatLevel;
			}
			blockDirThreatInfoTable[threatState.blockDir].threatLevel += threatStateThreatLevel;
			if (blockDirThreatInfoTable[threatState.blockDir].maxDuration < threatState.duration) {
				blockDirThreatInfoTable[threatState.blockDir].maxDuration = threatState.duration;
			}
			blockDirThreatInfoTable[eJediBlockDir_Mid].threatLevel += threatStateThreatLevel;
			if (blockDirThreatInfoTable[eJediBlockDir_Mid].maxDuration < threatState.duration) {
				blockDirThreatInfoTable[eJediBlockDir_Mid].maxDuration = threatState.duration;
			}
		}

		// update our dodge dir threat info
		if (threatState.dodgeDirMask != kJediAiThreatDodgeDirMask_None) {
			if (highestDodgeableThreatLevel < threatStateThreatLevel) {
				highestDodgeableThreatLevel = threatStateThreatLevel;
			}
			for (int d = 0; d < eJediDodgeDir_Count; ++d) {
				if (threatState.doesDodgeDirWork((EJediDodgeDir)d)) {
					dodgeDirThreatLevelTable[d] += threatStateThreatLevel;
				}
			}
		}

		// update our threat type data
		SJediThreatTypeData &threatTypeData = threatTypeDataTable[threatState.type];
		++threatTypeData.count;
		if (threatTypeData.shortestDurationThreat == NULL || threatTypeData.shortestDurationThreat->duration > threatState.duration) {
			threatTypeData.shortestDurationThreat = &threatState;
		}
		if (threatTypeData.shortestDistanceThreat == NULL || threatTypeData.shortestDistanceThreat->distanceToSelf > threatState.distanceToSelf) {
			threatTypeData.shortestDistanceThreat = &threatState;
		}
	}

	// average our threat level
	if (threatStateCount > 0) {
		threatLevel /= (float)TR_COUNTOF(threatStates);
	}
}

void CJediAiMemory::simulateThreats(float dt, const SSimulateParams &params) {

	// clear our aggregate data
	memset(threatTypeDataTable, 0, sizeof(threatTypeDataTable));
	recommendedDeflectionDuration = 0.0f;
	recommendedCrouchDuration = 0.0f;
	nextRushThreatDuration = 0.0f;
	highestBlockableThreatLevel = 0.0f;
	memset(blockDirThreatInfoTable, 0, sizeof(blockDirThreatInfoTable));
	highestDodgeableThreatLevel = 0.0f;
	memset(dodgeDirThreatLevelTable, 0, sizeof(dodgeDirThreatLevelTable));

	// keep track of all of the threats which survive this simulation
	int survivingThreatCount = 0;
	int survivingThreats[TR_COUNTOF(threatStates)] = {};
	float threatLevels[TR_COUNTOF(threatStates)] = {};

	// update all of my threats
	for (int i = 0; i < threatStateCount; ++i) {
		SJediAiThreatState &threatState = threatStates[i];

		// if we have an attacker state, clear it's threat reference
		if (threatState.attackerState != NULL) {
			threatState.attackerState->threatState = NULL;
		}

		// if we have an object state, clear it's threat reference
		if (threatState.objectState != NULL) {
			threatState.objectState->threatState = NULL;
		}

		// certain types of threats require that their attacker be alive and not incapacitated or stumbling
		if (threatState.attackerState != NULL) {
			switch (threatState.type) {
				case eJediThreatType_Explosion:
					if (
						(threatState.attackerState->enemyType != eJediEnemyType_TrandoshanConcussive) &&
						(threatState.attackerState->enemyType != eJediEnemyType_TrandoshanFlutterpack)
					) {
						break;
					}
				case eJediThreatType_Melee:
				case eJediThreatType_Rush: {
					if (
						(threatState.attackerState->hitPoints <= 0.0f) ||
						(threatState.attackerState->flags & kJediAiActorStateFlag_Stumbling) ||
						(threatState.attackerState->flags & kJediAiActorStateFlag_Incapacitated)
					) {
						continue;
					}
				}
			}
		}

		// if this threat has expired and is no longer a threat, skip it
		if (threatState.duration <= dt) {
			if (threatState.strength <= 0.0f) {
				continue;
			}
		}

		// if this threat is moving, update it's position
		if (!threatState.iVelocity.isCloseTo(kZeroVector, 0.001f)) {
			CVector iDelta = threatState.iVelocity * dt;
			threatState.wPos += iDelta;
		}

		// update the 'to-self' data for this threat
		float prevFaceSelfPct = threatState.faceSelfPct;
		float prevSelfFacePct = threatState.selfFacePct;
		float prevDistanceToSelf = threatState.distanceToSelf;
		updateThreatToSelfState(threatState);

		// for forward facing threats, what is the minimum face pct I am threatened by?
		CVector iThreatToSelfDelta = selfState.wPos - threatState.wPos;
		CVector iThreatToSelfDir = iThreatToSelfDelta;
		iThreatToSelfDir.normalize();
		CVector iThreatToSelfBoundsDir = iThreatToSelfDelta + (iThreatToSelfDir.crossProduct(kUnitVectorY) * (selfState.collisionRadius * 1.5f));
		iThreatToSelfBoundsDir.normalize();
		float minFaceSelfPct = iThreatToSelfDir.dotProduct(iThreatToSelfBoundsDir);
		if (minFaceSelfPct < 0.5f) {
			minFaceSelfPct = 0.5f;
		} else if (minFaceSelfPct > 0.99f) {
			minFaceSelfPct = 0.99f;
		}

		// if I moved through a blaster bolt or rocket, it damaged me and is no more
		if (threatState.type == eJediThreatType_Blaster || threatState.type == eJediThreatType_Rocket) {
			bool faceDirsAreOpposite = ((threatState.faceSelfPct * prevFaceSelfPct) < 0.0f);
			if (faceDirsAreOpposite) {

				// if I jumped over this threat, skip it
				if (isSelfInState(eJediState_Jumping)) {
					float yDelta = (selfState.wPos.y + 10.0f - threatState.wPos.y);
					if (yDelta > selfState.collisionRadius) {
						continue;
					}
				}

				// if the threat was facing me enough that it hit me as I moved through it, apply damage
				float offsetFromMovedThroughPct = fabs(threatState.faceSelfPct + prevFaceSelfPct);
				if (offsetFromMovedThroughPct < (1.0f - minFaceSelfPct)) {

					// if I'm deflecting and this is a blaster bolt, skip it
					if (threatState.type == eJediThreatType_Blaster && isSelfInState(eJediState_Deflecting) && prevSelfFacePct > 0.0f) {

						// if I'm deflecting back at enemies, damage the attacker
						if (isSelfInState(eJediState_Reflecting)) {
							if (threatState.attackerState != NULL) {
								simulateDamage(threatState.strength, *threatState.attackerState);
							} else {
								CActor *attacker = threatState.threat->creator;
								if (attacker != NULL && attacker == victimState->actor) {
									simulateDamage(threatState.strength, *victimState);
								}
							}
						}
						continue;
					}

					// if I didn't deflect this blast, it damaged me
					selfState.hitPoints -= threatState.strength;
					continue;
				}
			}
		}

		// if this threat expires during this timestep, apply damage
		if (threatState.duration <= dt) {

			// see if this threat hits me
			switch (threatState.type) {

				case eJediThreatType_Blaster: {

					// if this threat isn't facing me enough, skip it
					if (threatState.faceSelfPct < minFaceSelfPct) {
						continue;
					}

					// if I'm deflecting, skip it
					if (isSelfInState(eJediState_Deflecting)) {

						// if I'm deflecting back at enemies, damage the attacker
						if (isSelfInState(eJediState_Reflecting)) {
							if (threatState.attackerState != NULL) {
								simulateDamage(threatState.strength, *threatState.attackerState);
							} else {
								CActor *attacker = threatState.threat->creator;
								if (attacker != NULL && attacker == victimState->actor) {
									simulateDamage(threatState.strength, *victimState);
								}
							}
						}
						continue;
					}

				} break;

				case eJediThreatType_Melee: {

					// if I'm jumping, skip horizontal threats
					if (isSelfInState(eJediState_Jumping) && (threatState.flags & kJediAiThreatStateFlag_Horizontal)) {
						continue;
					}

					// if this threat requires blocking in the direction I'm blocking, skip it
					if (threatState.blockDir != eJediBlockDir_None && (params.blockDir == threatState.blockDir || params.blockDir == eJediBlockDir_Mid)) {
						continue;
					}

					// if this threat is horizontal and I'm crouching, skip it
					if ((threatState.flags & kJediAiThreatStateFlag_Horizontal) && isSelfInState(eJediState_Crouching)) {
						continue;
					}

					// at the moment that the melee attack impacted, how close was I?
					CVector wSelfPosAtImpact = lerp(selfState.wPrevPos, selfState.wPos, (threatState.duration / dt));
					float distanceSqAtImpact = threatState.wPos.distanceSqTo(wSelfPosAtImpact);

					// if I was far enough from the attack when it hit, ignore this threat
					float minDamageDistance = selfState.collisionRadius + threatState.damageRadius;
					if (distanceSqAtImpact > SQ(minDamageDistance)) {
						continue;
					}

					// if this isn't a 360 melee attack, make sure that me attacker is facing me enough
					if (!(threatState.flags & kJediAiThreatStateFlag_Melee360)) {

						// at the moment of impact, how much was the attacker facing me?
						// if I have information on the attacker, just use that to save time
						float attackerFacePct;
						if (threatState.attackerState != NULL) {
							CVector iAttackerToJediDir = threatState.attackerState->wPos.xzDirectionTo(wSelfPosAtImpact);
							attackerFacePct = threatState.attackerState->iFrontDir.dotProduct(iAttackerToJediDir);
						} else {
							CActor *attacker = threatState.threat->creator;
							if (attacker != NULL) {
								CVector wAttackerPos = attacker->getPos();
								CVector iAttackerDir = attacker->getFrontDir();
								CVector iAttackerToJediDir = wAttackerPos.xzDirectionTo(wSelfPosAtImpact);
								attackerFacePct = iAttackerDir.dotProduct(iAttackerToJediDir);
							}
						}

						// if this is a horizontal attack and the attacker isn't facing me enough, ignore the threat
						if (threatState.flags & kJediAiThreatStateFlag_Horizontal) {

							// if the attacker isn't facing me at all, I'm safe
							if (attackerFacePct < 0.0f) {
								continue;
							}

							// if I can dodge to a side and I am on that side, I'm safe
							float sidePct = threatState.iRightDir.dotProduct(threatState.iToSelfDir);
							if (threatState.doesDodgeDirWork(eJediDodgeDir_Left) && sidePct < 0.5f) {
								continue;
							} else if (threatState.doesDodgeDirWork(eJediDodgeDir_Right) && sidePct > 0.5f) {
								continue;
							}

						// otherwise, check the 'minFaceSelfPct'
						} else if (attackerFacePct < minFaceSelfPct) {
							continue;
						}

					}

				} break;

				case eJediThreatType_Rush: {

					// if I am far enough outside the threat's damage radius, ignore it
					float threatDamageDist = threatState.distanceToSelf - threatState.damageRadius;
					if (threatDamageDist > selfState.collisionRadius) {
						continue;
					}

					// how much is the attacker facing me?
					// if I have information on the attacker, just use that to save time
					float attackerFacePct;
					if (threatState.attackerState != NULL) {
						attackerFacePct = threatState.attackerState->faceSelfPct;
					} else {
						CActor *attacker = threatState.threat->creator;
						if (attacker != NULL) {
							CVector wAttackerPos = attacker->getPos();
							CVector iAttackerDir = attacker->getFrontDir();
							CVector iAttackerToJediDir = wAttackerPos.xzDirectionTo(selfState.wPos);
							attackerFacePct = iAttackerDir.dotProduct(iAttackerToJediDir);
						}
					}

					// if the attacker isn't facing me enough, ignore the threat
					if (attackerFacePct < minFaceSelfPct) {
						continue;
					}

				} break;

				case eJediThreatType_Rocket:

					// if this threat isn't facing me enough, ignore it
					if (threatState.faceSelfPct < minFaceSelfPct) {
						continue;
					}

				case eJediThreatType_Grenade:
				case eJediThreatType_Explosion: {
					float damageRadius = threatState.damageRadius;

					// if this explosion is from a suicidal trandoshan flutterpack, it will kill the trandoshan as well
					if (threatState.attackerState != NULL) {
						if (threatState.attackerState->enemyType == eJediEnemyType_TrandoshanFlutterpack) {
							if (threatState.attackerState->flags & kJediAiActorStateFlag_InRushAttack) {
								simulateDamage(threatState.attackerState->hitPoints, *threatState.attackerState);
							}
						}
					}

					// explosions damage all nearby enemies as well
					for (int i = 0; i < enemyStateCount; ++i) {
						SJediAiActorState &enemyState = enemyStates[i];
						float distanceSq = threatState.wPos.distanceSqTo(enemyState.wPos);
						float minDamageDistance = enemyState.collisionRadius + damageRadius;
						if (distanceSq <= SQ(minDamageDistance)) {
							simulateDamage(threatState.strength, enemyState);
						}
					}

					// at the moment that the threat exploded, how close was I?
					CVector wSelfPosAtExplosion = lerp(selfState.wPrevPos, selfState.wPos, (threatState.duration / dt));
					float distanceSqAtExplosion = threatState.wPos.distanceSqTo(wSelfPosAtExplosion);

					// if I was far enough from the explosion when it exploded, ignore this threat
					float minDamageDistance = selfState.collisionRadius + damageRadius;
					if (distanceSqAtExplosion > SQ(minDamageDistance)) {
						continue;
					}

				} break;

			}

			// we haven't stopped this threat from hitting us
			// apply its damage to our hitPoints
			selfState.hitPoints -= threatState.strength;

			// if this threat's attack level is high enough, we will be knocked around
			if (threatState.threat->attackLevel > eAttackLevel_Light) {
				setSelfInState(eJediState_KnockedAround, true);
			}

			// this threat no longer exists
			continue;
		}

		// apply the timestep
		threatState.duration -= dt;

		// recalculate the threat level
		threatLevels[survivingThreatCount] = computeThreatStateLevel(threatState);

		// this threat survives
		survivingThreats[survivingThreatCount++] = i;
	}

	// keep only the surviving threats in the list
	threatLevel = 0;
	for (int i = 0; i < survivingThreatCount; ++i) {

		// save off the surviving threat
		threatStates[i] = threatStates[survivingThreats[i]];
		SJediAiThreatState &threatState = threatStates[i];

		// if we have an attacker state and this threat has less duration than its current threat,
		// set this threat as the attacker's threat
		if (threatState.attackerState != NULL) {
			if (threatState.attackerState->threatState == NULL || threatState.attackerState->threatState->duration > threatState.duration) {
				threatState.attackerState->threatState = &threatState;
			}
		}

		// if we have an object state and this threat has less duration than its current threat,
		// set this threat as the object's threat
		if (threatState.objectState != NULL) {
			if (threatState.objectState->threatState == NULL || threatState.objectState->threatState->duration > threatState.duration) {
				threatState.objectState->threatState = &threatState;
			}
		}

		// update our aggregate threat level
		float threatStateThreatLevel = threatLevels[i];
		threatLevel += threatStateThreatLevel;

		// if I'm facing this threat enough, update our recommended deflection duration
		if (threatState.type == eJediThreatType_Blaster && threatState.selfFacePct > 0.25f) {
			if (recommendedDeflectionDuration < threatState.duration) {
				recommendedDeflectionDuration = threatState.duration;
			}
		}

		// update our recommended crouch duration
		if ((threatState.flags & kJediAiThreatStateFlag_Horizontal) && recommendedCrouchDuration < threatState.duration) {
			recommendedCrouchDuration = threatState.duration;
		}

		// update our incoming tackle delay time
		if (threatState.type == eJediThreatType_Rush && nextRushThreatDuration < threatState.duration) {
			nextRushThreatDuration = threatState.duration;
		}

		// update our block dir threat info
		if (threatState.blockDir != eJediBlockDir_None) {
			if (highestBlockableThreatLevel < threatStateThreatLevel) {
				highestBlockableThreatLevel = threatStateThreatLevel;
			}
			blockDirThreatInfoTable[threatState.blockDir].threatLevel += threatStateThreatLevel;
			if (blockDirThreatInfoTable[threatState.blockDir].maxDuration < threatState.duration) {
				blockDirThreatInfoTable[threatState.blockDir].maxDuration = threatState.duration;
			}
			blockDirThreatInfoTable[eJediBlockDir_Mid].threatLevel += threatStateThreatLevel;
			if (blockDirThreatInfoTable[eJediBlockDir_Mid].maxDuration < threatState.duration) {
				blockDirThreatInfoTable[eJediBlockDir_Mid].maxDuration = threatState.duration;
			}
		}

		// update our dodge dir threat info
		if (threatState.dodgeDirMask != kJediAiThreatDodgeDirMask_None) {
			if (highestDodgeableThreatLevel < threatStateThreatLevel) {
				highestDodgeableThreatLevel = threatStateThreatLevel;
			}
			for (int d = 0; d < eJediDodgeDir_Count; ++d) {
				if (threatState.doesDodgeDirWork((EJediDodgeDir)d)) {
					dodgeDirThreatLevelTable[d] += threatStateThreatLevel;
				}
			}
		}

		// update our threat type data
		SJediThreatTypeData &threatTypeData = threatTypeDataTable[threatState.type];
		++threatTypeData.count;
		if (threatTypeData.shortestDurationThreat == NULL || threatTypeData.shortestDurationThreat->duration > threatState.duration) {
			threatTypeData.shortestDurationThreat = &threatState;
		}
		if (threatTypeData.shortestDistanceThreat == NULL || threatTypeData.shortestDistanceThreat->distanceToSelf > threatState.distanceToSelf) {
			threatTypeData.shortestDistanceThreat = &threatState;
		}
	}
	#if defined(_DEBUG)
		memset(threatStates + survivingThreatCount, 0, sizeof(SJediAiThreatState) * (threatStateCount - survivingThreatCount));
	#endif
	threatStateCount = survivingThreatCount;

	// update our threat level
	if (threatStateCount > 0) {
		threatLevel /= (float)TR_COUNTOF(threatStates);
	}
}

void CJediAiMemory::updateThreatToSelfState(SJediAiThreatState &updateMe) {

	// update positional data
	updateMe.iToSelfDir = updateMe.wPos.xzDirectionTo(selfState.wPos);
	updateMe.distanceToSelf = updateMe.wPos.xzDistanceTo(selfState.wPos);
	if (updateMe.distanceToSelf > 0.0001f) {
		updateMe.selfFacePct = (-updateMe.iToSelfDir).dotProduct(selfState.iFrontDir);
		updateMe.faceSelfPct = updateMe.iToSelfDir.dotProduct(updateMe.iFrontDir);
	} else {
		updateMe.selfFacePct = 0.0f;
		updateMe.faceSelfPct = 0.0f;
	}

	// where is this threat relative to me?
	CVector iSelfToThreatDir = selfState.wPos.directionTo(updateMe.wPos);
	float frontPct = selfState.iFrontDir.dotProduct(iSelfToThreatDir);
	float rightPct = selfState.iRightDir.dotProduct(iSelfToThreatDir);

	// update type-specific data
	updateMe.blockDir = eJediBlockDir_None;
	updateMe.dodgeDirMask = kJediAiThreatDodgeDirMask_None;
	switch (updateMe.type) {

		case eJediThreatType_Blaster: {

			// blaster threat durations begin as 'travel time to maxDistance'
			// change this to 'travel time to self'
			float speed = updateMe.iVelocity.length();
			updateMe.duration = (speed > 0.0f ? updateMe.distanceToSelf / speed : 0.0f);

			// dodge perpendicular to the blaster bolts if I have time
			if (updateMe.duration > (kJediDodgeDuration * 0.2f)) {
				updateMe.dodgeDirMask = kJediAiThreatDodgeDirMask_None;
				if (fabs(frontPct) > fabs(rightPct)) {
					updateMe.dodgeDirMask |= kJediAiThreatDodgeDirMask_Lateral;
				} else {
					updateMe.dodgeDirMask |= kJediAiThreatDodgeDirMask_Back;
				}
			}

		} break;

		case eJediThreatType_Melee: {

			// can I block this melee attack?
			bool canBlock = (updateMe.attackerState != NULL && updateMe.attackerState->combatType == eJediCombatType_Brawler);

			// if I am facing this threat and it isn't a 360 degree melee attack, determine which direction I would
			// need to block or dodge to nullify it
			if (updateMe.selfFacePct >= 0.85f && !(updateMe.flags & kJediAiThreatStateFlag_Melee360)) {
				if (updateMe.threat->iDir.x > 0.0f) {
					if (canBlock) {
						updateMe.blockDir = eJediBlockDir_Left;
					}
					updateMe.dodgeDirMask = kJediAiThreatDodgeDirMask_Right | kJediAiThreatDodgeDirMask_Back;
				} else if (updateMe.threat->iDir.x < 0.0f) {
					if (canBlock) {
						updateMe.blockDir = eJediBlockDir_Right;
					}
					updateMe.dodgeDirMask = kJediAiThreatDodgeDirMask_Left | kJediAiThreatDodgeDirMask_Back;
				} else if (updateMe.threat->iDir.y > 0.0f) {
					if (canBlock) {
						updateMe.blockDir = eJediBlockDir_High;
					}
					updateMe.dodgeDirMask = kJediAiThreatDodgeDirMask_All;
				} else {
					updateMe.dodgeDirMask = kJediAiThreatDodgeDirMask_Back;
				}

			// otherwise, just dodge away from it
			} else {
				updateMe.dodgeDirMask = kJediAiThreatDodgeDirMask_None;
				if (frontPct > -0.5f) {
					updateMe.dodgeDirMask |= kJediAiThreatDodgeDirMask_Back;
				}
				if (rightPct > -0.66f) {
					if (canBlock) {
						updateMe.blockDir = eJediBlockDir_Right;
					}
					updateMe.dodgeDirMask |= kJediAiThreatDodgeDirMask_Left;
				}
				if (rightPct < 0.66f) {
					if (canBlock) {
						updateMe.blockDir = eJediBlockDir_Left;
					}
					updateMe.dodgeDirMask |= kJediAiThreatDodgeDirMask_Right;
				}
			}

		} break;

		case eJediThreatType_Rocket: {

			// rocket threat durations begin as 'travel time to maxDistance'
			// change this to 'travel time to self'
			float speed = updateMe.iVelocity.length();
			updateMe.duration = (speed > 0.0f ? updateMe.distanceToSelf / speed : 0.0f);

		} // fall through into 'Rush'

		case eJediThreatType_Rush: {

			// dodge perpendicular to the threat direction
			updateMe.dodgeDirMask = kJediAiThreatDodgeDirMask_None;
			if (fabs(frontPct) > fabs(rightPct)) {
				updateMe.dodgeDirMask |= kJediAiThreatDodgeDirMask_Lateral;
			} else {
				updateMe.dodgeDirMask |= kJediAiThreatDodgeDirMask_Back;
			}

		} break;

		case eJediThreatType_Grenade:
		case eJediThreatType_Explosion: {

			// dodge away from this explosion
			if (updateMe.distanceToSelf < (selfState.collisionRadius * 0.5f)) {
				updateMe.dodgeDirMask = kJediAiThreatDodgeDirMask_All;
			} else {
				updateMe.dodgeDirMask = kJediAiThreatDodgeDirMask_None;
				if (frontPct > 0.0f) {
					updateMe.dodgeDirMask |= kJediAiThreatDodgeDirMask_Back;
				}
				if (rightPct > 0.0f) {
					updateMe.dodgeDirMask |= kJediAiThreatDodgeDirMask_Left;
				} else {
					updateMe.dodgeDirMask |= kJediAiThreatDodgeDirMask_Right;
				}
			}

		} break;
	}
}

float CJediAiMemory::computeThreatStateLevel(SJediAiThreatState &threatState) {

	// if this is a big attack, get out of the way
	if (threatState.threat->attackLevel >= eAttackLevel_Heavy) {

		// if this is a grenade or explosion, we are safe if we are outside the blast radius
		if (threatState.type == eJediThreatType_Grenade || threatState.type == eJediThreatType_Explosion) {
			float damageDistance = ((selfState.collisionRadius * 2.0f) + threatState.damageRadius);
			if (threatState.distanceToSelf < damageDistance) {
				return 2.0f;
			} else {
				return 0.0f;
			}
		}

		// otherwise, we consider this threat serious
		return 1.0f;
	}

	// otherwise, take damage and time into account
	float timeFactor = (1.0f - limit(0.0f, ((threatState.duration - 0.5f) / kThreatMaxAwareDuration), 1.0f));
	float damageFactor = limit(0.0f, (threatState.threat->strength / max(0.001f, selfState.hitPoints)), 1.0f);
	return (timeFactor * damageFactor);
}
