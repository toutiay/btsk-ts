#ifndef __ACTOR__
#define __ACTOR__

#ifndef __VECTOR__
	#include "vector.h"
#endif

#ifndef __JEDI_COMMON__
	#include "jedi_common.h"
#endif


/////////////////////////////////////////////////////////////////////////////
//
// CActor class
//
/////////////////////////////////////////////////////////////////////////////

class CJedi;

class CActor {
public:

	// construction
	CActor() {
		wPos = kZeroVector;
		wBoundsCenter = kZeroVector;
		iRightDir = kUnitVectorX;
		iUpDir = kUnitVectorY;
		iFrontDir = kUnitVectorZ;
		currentTarget = NULL;
	}

	// virtual dtor
	virtual ~CActor() {
	}

	// current position
	const CVector &getPos() const { return wPos; }
	CVector wPos;

	// bounding box center
	const CVector &getBoundsCenter() const { return wBoundsCenter; }
	CVector wBoundsCenter;

	// axis vectors
	const CVector &getRightDir() const { return iRightDir; }
	const CVector &getUpDir() const { return iUpDir; }
	const CVector &getFrontDir() const { return iFrontDir; }
	CVector iRightDir;
	CVector iUpDir;
	CVector iFrontDir;

	// hitpoints
	float getHitPoints() const { return 100.0f; }

	// collision radius
	float getCollisionRadius() const { return 5.0f; }

	// get inertial velocity
	CVector getInertialVelocity() const { return kZeroVector; }

	// is this actor force grippable?
	// assume all actors are grippable for this example
	bool isForceGrippable() const { return true; }

	// is this actor force gripped?
	bool isForceGripped() const { return false; }

	// is the specified Jedi force gripping this actor?
	bool isBeingForceGrippedByJedi(CJedi *jedi) const { return false; }

	// get the jedi combat type
	EJediCombatType getJediCombatType() const { return eJediCombatType_Unknown; }

	// get the jedi enemy type
	EJediEnemyType getJediEnemyType() const { return eJediEnemyType_Unknown; }

	// is this actor an enemy?
	bool isJediEnemy() const { return getJediEnemyType() != eJediEnemyType_Unknown; }

	// is this actor shielded?
	bool isShielded() const { return false; }

	// is this actor stumbling?
	bool isStumbling() const { return false; }

	// is this actor incapacitated?
	bool isIncapacitated() const { return false; }

	// is this actor a jedi?
	// in the real game, we would use RTTI to do this, but this will work for this example
	virtual bool isJedi() const { return false; }

	// is this actor a force-tk object?
	// in the real game, we would use RTTI to do this, but this will work for this example
	virtual bool isForceTkObject() const { return false; }

	// can this actor be tanted?
	bool canBeTaunted() const { return true; }

	// taunt this actor
	// the base class ignores this event
	virtual void taunt() {}

	// get the height of the floor under this actor
	float getFloorHeight() const { return 0.0f; }


	//------------------------------------
	// current target
	//------------------------------------

	// Instructs the character to begin attacking the specified victim.
	// if the target is changing, ...
	// tell the old target (if they still exist) that we are no longer targeting them
	// set our current target to our new target
	// if we have a target, set the ai's victim as well
	// otherwise, clear the ai's victim
	void setCurrentTarget(CActor *target) { currentTarget = target; }

	// Returns the current target of the jedi
	CActor *getCurrentTarget() { return currentTarget; }

	// returns true if we have a target along with filling in the distance to target squared
	// will return false if we do not currently have a target and distanceToTargetSq will remain unchanged
	bool distanceToTarget(float *distanceToTargetSq = NULL) {
		if (currentTarget == NULL)
			return false;
		if (distanceToTargetSq != NULL)
			wPos.distanceSqTo(currentTarget->getPos());
		return true;
	}

	// is my current target visible?
	bool isCurrentTargetVisible() const { return true; }

protected:

	// current target
	CActor *currentTarget;
};

#endif // __ACTOR__
