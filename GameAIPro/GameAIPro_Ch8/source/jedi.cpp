#include "pch.h"
#include "jedi.h"


///////////////////////////////////////////////////////////////////////////////
//
// CJedi class
//
///////////////////////////////////////////////////////////////////////////////

CJedi::CJedi() {
}

CJedi::~CJedi() {
}


// ----------------------------------------------------------------------------
//
bool CJedi::setup() {

	// setup my AI behavior tree
	aiMemory.selfState.jedi = this;
	aiCombatAction.init(&aiMemory);

	// success!
	return true;
}

void CJedi::process(float dt) {
	aiMemory.update(dt);
	aiCombatAction.update(dt);
}

bool CJedi::isDefensiveModeEnabled() const {
	return defensiveModeData.enabled;
}

void CJedi::enableDefensiveMode(bool disableOnPlayerAttack) {
	defensiveModeData.enabled = true;
	defensiveModeData.disableOnPlayerAttack = disableOnPlayerAttack;
}

void CJedi::disableDefensiveMode() {
	memset(&defensiveModeData, 0, sizeof(defensiveModeData));
}

void CJedi::onPlayerAggressive() {

	// if we are waiting for the player to be aggressive, disable defensive mode
	if (defensiveModeData.disableOnPlayerAttack) {
		disableDefensiveMode();
	}
}

bool CJedi::isModelInAnimationRange(const char *animRangeName) {
	return false; // stubbed for this example
}

void CJedi::updateActionBitfield() {

	// clear the bitfield
	unsigned prevStateBitfield = currentStateBitfield;
	currentStateBitfield = 0;

	// map actions to animation range names
	const char *jediStateRangeNames[eJediState_Count];
	jediStateRangeNames[eJediState_SwingingSaber] = "saberSwing";
	jediStateRangeNames[eJediState_Crouching] = "isCrouching";
	jediStateRangeNames[eJediState_Dodging] = "isDodging";
	jediStateRangeNames[eJediState_Dashing] = "isDashing";
	jediStateRangeNames[eJediState_Jumping] = "isJumping";
	jediStateRangeNames[eJediState_Flipping] = "isFlipping";
	jediStateRangeNames[eJediState_ForcePushing] = "isForcePushing";
	jediStateRangeNames[eJediState_UsingForceTk] = "isUsingForceTk";
	jediStateRangeNames[eJediState_ForceTkAllowed] = "forceTkAllowed";
	jediStateRangeNames[eJediState_Kicking] = "isKicking";
	jediStateRangeNames[eJediState_Blocking] = "isBlocking";
	jediStateRangeNames[eJediState_KnockedAround] = "isKnockedAround";
	jediStateRangeNames[eJediState_Deflecting] = "isDeflecting";
	jediStateRangeNames[eJediState_Reflecting] = "isReflecting";
	jediStateRangeNames[eJediState_AiBlocked] = "block_ai";
	jediStateRangeNames[eJediState_Turning] = "isTurning";
	jediStateRangeNames[eJediState_DodgingLeft] = "isDodgingLeft";
	jediStateRangeNames[eJediState_DodgingRight] = "isDodgingRight";
	compileTimeAssert(eJediState_Count == 18); // add your new jedi actions to this list

	// evaluate each action
	for (int i = 0; i < eJediState_Count; ++i) {
		if (jediStateRangeNames[i] != NULL && isModelInAnimationRange(jediStateRangeNames[i])) {
			currentStateBitfield |= (1 << i);
		}
	}

	// if we have no active saber, we aren't deflecting
	if (!isSaberActive()) {
		currentStateBitfield &= ~((1 << eJediState_Deflecting));
	}
}

void CJedi::setCanDoAllActions(bool allow) {

	// set all actions' disabled bit
	for (int i = 0; i < eJediAction_Count; ++i) {
		setCanDoAction((EJediAction)i, allow, false);
	}
}

void CJedi::setCanAiDoAllActions(bool allow) {

	// set all actions' disabled bit
	for (int i = 0; i < eJediAction_Count; ++i) {
		setCanDoAction((EJediAction)i, allow, true);
	}
}

bool CJedi::canDoAction(EJediAction action) const {

	// if I'm under player control, check the regular bitfield
	if (!isAiControlled()) {
		return ((disabledActionBitfield & (1 << action)) == 0);
	}

	// is this action disabled
	return ((disabledActionAiBitfield & (1 << action)) == 0);
}

void CJedi::setCanDoAction(EJediAction action, bool allow, bool aiOnly) {

	// clear or set the action's disabled bit
	if (allow) {
		if (!aiOnly) {
			disabledActionBitfield &= ~(1 << action);
		}
		disabledActionAiBitfield &= ~(1 << action);
	} else {
		if (!aiOnly) {
			disabledActionBitfield |= (1 << action);
		}
		disabledActionAiBitfield |= (1 << action);
	}
}

bool CJedi::hasCommand(ECommand command) const {
	return ((commands & (1 << command)) != 0);
}

void CJedi::setCommand(ECommand command) {
	commands |= (1 << command);
}

void CJedi::clearJediCommandParams() {
	memset(&commandSwingSaberParams, 0, sizeof(commandSwingSaberParams));
	memset(&commandDodgeParams, 0, sizeof(commandDodgeParams));
	memset(&commandDashParams, 0, sizeof(commandDashParams));
	memset(&commandForceTkGripParams, 0, sizeof(commandForceTkGripParams));
	memset(&commandForceTkThrowParams, 0, sizeof(commandForceTkThrowParams));
	memset(&commandJumpOverParams, 0, sizeof(commandJumpOverParams));
	memset(&commandJumpForwardParams, 0, sizeof(commandJumpForwardParams));
	memset(&commandJumpForwardData, 0, sizeof(commandJumpForwardData));
	memset(&commandKickParams, 0, sizeof(commandKickParams));
	memset(&commandDeflectParams, 0, sizeof(commandDeflectParams));
	memset(&commandBlockParams, 0, sizeof(commandBlockParams));
}

void CJedi::setCommandJediStandDefensive() {
	setCommand(eCommandStandDefensive);
}

void CJedi::setCommandJediStrafe() {
	setCommand(eCommandStrafe);
}

void CJedi::setCommandJediSwingSaber(EJediSwingSaberDir dir) {
	commandSwingSaberParams.dir = dir;
	setCommand(eCommandSwingSaber);
}

void CJedi::setCommandJediDodge(EJediDodgeDir dir, bool attack) {
	commandDodgeParams.dir = dir;
	commandDodgeParams.attack = attack;
	setCommand(eCommandDodge);
}

void CJedi::setCommandJediDodgeFlip(EJediDodgeDir dir) {
	setCommandJediDodge(dir, false);
	commandDodgeParams.flip = true;
}

void CJedi::setCommandJediDash(CActor *target, float distanceToTarget, bool attack) {
	if (target == NULL) {
		return;
	}
	commandDashParams.target = target;
	commandDashParams.distanceToTarget = distanceToTarget;
	commandDashParams.attack = attack;
	setCommand(eCommandDash);
}

void CJedi::setCommandJediForcePushCharge() {
	setCommand(eCommandForcePushCharge);
}

void CJedi::setCommandJediForcePushThrow() {
	setCommand(eCommandForcePushThrow);
}

void CJedi::setCommandJediForceTkGrip(CActor *target, bool twoHand, float holdHeight, bool skipEnter) {
	if (target == NULL) {
		return;
	}
	commandForceTkGripParams.target = target;
	commandForceTkGripParams.twoHand = twoHand;
	commandForceTkGripParams.holdHeight = holdHeight;
	commandForceTkGripParams.skipEnter = skipEnter;
	setCommand(eCommandForceTkGrip);
}

void CJedi::setCommandJediForceTkThrow(CVector iThrowVelocity) {

	// get our grip target
	CActor *gripTarget;
	if (commandForceTkGripParams.target != NULL) {
		gripTarget = commandForceTkGripParams.target;
	} else {
		return;
	}

	// compute our throw direction
	CVector iThrowDir = iThrowVelocity;
	iThrowDir.normalize();
	float throwRightPct = getRightDir().dotProduct(iThrowDir);
	float throwUpPct = getUpDir().dotProduct(iThrowDir);

	// set our params
	commandForceTkThrowParams.iThrowVelocity = iThrowVelocity;
	commandForceTkThrowParams.target = NULL;
	commandForceTkThrowParams.throwRightPct = throwRightPct;
	commandForceTkThrowParams.throwUpPct = throwUpPct;
	setCommand(eCommandForceTkThrow);
}

void CJedi::setCommandJediForceTkThrowAtTarget(CActor *target) {

	// compute the velocity required to hit our target
	CVector iThrowVelocity;
	if (!advAiComputeThrowAtTargetVelocity(target, iThrowVelocity)) {
		return;
	}

	// compute our throw direction
	CVector iThrowDir = iThrowVelocity;
	iThrowDir.normalize();
	float throwRightPct = getRightDir().dotProduct(iThrowDir);
	float throwUpPct = getUpDir().dotProduct(iThrowDir);

	// set our params
	commandForceTkThrowParams.iThrowVelocity = iThrowVelocity;
	commandForceTkThrowParams.target = target;
	commandForceTkThrowParams.throwRightPct = throwRightPct;
	commandForceTkThrowParams.throwUpPct = throwUpPct;
	setCommand(eCommandForceTkThrow);
}

bool CJedi::advAiComputeThrowAtTargetVelocity(CActor *target, CVector &iThrowVelocity) const {

	// get our grip target
	CActor *gripTarget;
	if (commandForceTkGripParams.target != NULL) {
		gripTarget = commandForceTkGripParams.target;
	} else {
		iThrowVelocity.zero();
		return false;
	}

	// compute the velocity required to hit the specified target
	iThrowVelocity = (target->getBoundsCenter() - gripTarget->getBoundsCenter());
	float speedSq = iThrowVelocity.lengthSq();
	float ySpeedToNegateGravity = (getGravity() / 2.0f);
	const float kMinSpeed = 30.0f;
	if (speedSq < SQ(kMinSpeed)) {
		ySpeedToNegateGravity *= (sqrt(speedSq) / kMinSpeed);
		float initialY = iThrowVelocity.y;
		iThrowVelocity.setLength(kMinSpeed);
		iThrowVelocity.y = initialY;
	}
	iThrowVelocity.y += ySpeedToNegateGravity;

	// success!
	return true;
}

void CJedi::setCommandJediJumpOver(CActor *target, bool attack) {
	if (target == NULL) {
		return;
	}
	commandJumpOverParams.target = target;
	commandJumpOverParams.attack = attack;
	setCommand(eCommandJumpOver);
}

void CJedi::setCommandJediJumpForward(CActor *target, EJediAiJumpForwardAttack attack, float distance) {
	memset(&commandJumpForwardParams, 0, sizeof(commandJumpForwardParams));
	if (target == NULL) {
		return;
	}
	commandJumpForwardParams.target = target;
	commandJumpForwardParams.attack = attack;
	commandJumpForwardParams.distance = distance;
	setCommand(eCommandJumpForward);
}

void CJedi::setCommandJediCrouch() {
	setCommand(eCommandCrouch);
}

void CJedi::setCommandJediCrouchAttack() {
	setCommand(eCommandCrouchAttack);
}

void CJedi::setCommandJediKick(bool allowDisplacement) {
	commandKickParams.allowDisplacement = allowDisplacement;
	setCommand(eCommandKick);
}

void CJedi::setCommandJediDeflect(bool deflectAtEnemies) {
	commandDeflectParams.deflectAtEnemies = deflectAtEnemies;
	setCommand(eCommandDeflect);
}

void CJedi::setCommandJediBlock(EJediBlockDir dir) {
	commandBlockParams.dir = dir;
	setCommand(eCommandBlock);
}

void CJedi::setCommandJediBlockImpact() {
	setCommand(eCommandBlockImpact);
}

void CJedi::setCommandJediTaunt() {
	setCommand(eCommandTaunt);
}
