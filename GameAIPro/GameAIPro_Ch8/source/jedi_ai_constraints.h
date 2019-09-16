#ifndef __JEDI_AI_CONSTRAINTS__
#define __JEDI_AI_CONSTRAINTS__

#ifndef __JEDI_COMMON__
	#include "jedi_common.h"
#endif

/////////////////////////////////////////////////////////////////////////////
//
// globals
//
/////////////////////////////////////////////////////////////////////////////

// forward decls
class CJediAiMemory;
class CJediAiAction;


/////////////////////////////////////////////////////////////////////////////
//
// constraint
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionConstraint {
public:

	// next constraint in the list
	CJediAiActionConstraint *nextConstraint;

	// don't check this constraint while the action is in progress
	bool skipWhileInProgress;

	// don't check this constraint while the action is simulating
	bool skipWhileSimulating;

	// construction
	CJediAiActionConstraint();

	// clear all of my data
	virtual void reset();

	// check our constraint
	virtual EJediAiActionResult checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const = 0;
};


/////////////////////////////////////////////////////////////////////////////
//
// kill timer constraint
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionConstraintKillTimer : public CJediAiActionConstraint {
public:
	typedef CJediAiActionConstraint BASECLASS;

	// params
	static const float kKillTimeIgnored;
	static const float kKillTimeDesired;
	struct {
		float minKillTime;
		float maxKillTime;
	} params, defaultParams;
	bool defaultSkipWhileInProgress;

	// construction
	CJediAiActionConstraintKillTimer();
	CJediAiActionConstraintKillTimer(float minKillTime, float maxKillTime, bool skipWhileInProgress = false);

	// clear all of my data
	virtual void reset();

	// CJediAiActionConstraint methods
	virtual EJediAiActionResult checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// threat constraint
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionConstraintThreat : public CJediAiActionConstraint {
public:
	typedef CJediAiActionConstraint BASECLASS;

	// options
	enum EThreatReaction {
		eThreatReaction_SucceedIfNone,
		eThreatReaction_SucceedIfAny,
		eThreatReaction_FailIfNone,
		eThreatReaction_FailIfAny,
		eThreatReaction_Count
	};

	// params
	struct {
		EThreatReaction threatReaction;
		struct {
			EJediThreatType list[eJediThreatType_Count];
			int count;
		} threatTypeReactionTable[eThreatReaction_Count];
		void addThreatReaction(EJediThreatType type, EThreatReaction reaction) {
			if (threatTypeReactionTable[reaction].count >= eJediThreatType_Count) {
				assert(threatTypeReactionTable[reaction].count < eJediThreatType_Count);
				return;
			}
			threatTypeReactionTable[reaction].list[threatTypeReactionTable[reaction].count++]= type;
		}
	} params;

	// construction
	CJediAiActionConstraintThreat();

	// CJediAiAction methods
	virtual void reset();

	// CJediAiActionConstraint methods
	virtual EJediAiActionResult checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// skill level constraint
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionConstraintSkillLevel : public CJediAiActionConstraint {
public:
	typedef CJediAiActionConstraint BASECLASS;

	// params
	struct {
		float minSkillLevel;
		float maxSkillLevel;
	} params, defaultParams;

	// construction
	CJediAiActionConstraintSkillLevel();
	CJediAiActionConstraintSkillLevel(float minSkillLevel, float maxSkillLevel);

	// clear all of my data
	virtual void reset();

	// CJediAiActionConstraint methods
	virtual EJediAiActionResult checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// distance constraint
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionConstraintDistance : public CJediAiActionConstraint {
public:
	typedef CJediAiActionConstraint BASECLASS;

	// params
	struct {
		EJediAiDestination destination;
		float minDistance;
		float maxDistance;
		EJediAiActionResult belowMinResult; // default is 'failure'
		EJediAiActionResult aboveMaxResult; // default is 'failure'
	} params;

	// construction
	CJediAiActionConstraintDistance();

	// clear all of my data
	virtual void reset();

	// CJediAiActionConstraint methods
	virtual EJediAiActionResult checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// too crowded melee space constraint
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionConstraintMeleeSpaceTooCrowded : public CJediAiActionConstraint {
public:
	typedef CJediAiActionConstraint BASECLASS;

	// construction
	CJediAiActionConstraintMeleeSpaceTooCrowded();

	// clear all of my data
	virtual void reset();

	// CJediAiActionConstraint methods
	virtual EJediAiActionResult checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// victim combat type constraint
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionConstraintVictimCombatType : public CJediAiActionConstraint {
public:
	typedef CJediAiActionConstraint BASECLASS;

	// parameters
	struct {
		unsigned int allowedCombatTypeFlags;
		inline void setAllCombatTypesAllowed() { allowedCombatTypeFlags = (unsigned int)-1; }
		inline void setAllCombatTypesDisallowed() { allowedCombatTypeFlags = 0; }
		inline void setCombatTypeAllowed(EJediCombatType combatType) { allowedCombatTypeFlags |= (1 << combatType); }
		inline void setCombatTypeDisallowed(EJediCombatType combatType) { allowedCombatTypeFlags &= ~(1 << combatType); }
		inline bool isCombatTypeAllowed(EJediCombatType combatType) const {
			return ((allowedCombatTypeFlags & (1 << combatType)) != 0);
		}
	} params;

	// construction
	CJediAiActionConstraintVictimCombatType();

	// clear all of my data
	virtual void reset();

	// CJediAiActionConstraint methods
	virtual EJediAiActionResult checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// victim enemy type constraint
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionConstraintVictimEnemyType : public CJediAiActionConstraint {
public:
	typedef CJediAiActionConstraint BASECLASS;

	// parameters
	struct {
		unsigned int allowedEnemyTypeFlags;
		inline void setAllEnemyTypesAllowed() { allowedEnemyTypeFlags = (unsigned int)-1; }
		inline void setAllEnemyTypesDisallowed() { allowedEnemyTypeFlags = 0; }
		inline void setEnemyTypeAllowed(EJediEnemyType enemyType) { allowedEnemyTypeFlags |= (1 << enemyType); }
		inline void setEnemyTypeDisallowed(EJediEnemyType enemyType) { allowedEnemyTypeFlags &= ~(1 << enemyType); }
		inline bool isEnemyTypeAllowed(EJediEnemyType enemyType) const {
			return ((allowedEnemyTypeFlags & (1 << enemyType)) != 0);
		}
	} params;

	// construction
	CJediAiActionConstraintVictimEnemyType();

	// clear all of my data
	virtual void reset();

	// CJediAiActionConstraint methods
	virtual EJediAiActionResult checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// flags constraint
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionConstraintFlags : public CJediAiActionConstraint {
public:
	typedef CJediAiActionConstraint BASECLASS;

	// parameters
	struct {
		bool succeedOnAnyFlags;
		unsigned int allowedFlags;
		inline void setAllFlagsAllowed() { allowedFlags = (unsigned int)-1; }
		inline void setAllFlagsDisallowed() { allowedFlags = 0; }
		inline void setFlagsAllowed(unsigned int flags) { allowedFlags |= flags; }
		inline void setFlagsDisallowed(unsigned int flags) { allowedFlags &= ~flags; }
		inline bool areAllFlagsAllowed(unsigned int flags) const {
			return ((allowedFlags & flags) == flags);
		}
		inline bool areAnyFlagsAllowed(unsigned int flags) const {
			return ((allowedFlags & flags) != 0);
		}
	} params;

	// construction
	CJediAiActionConstraintFlags();

	// clear all of my data
	virtual void reset();

	// CJediAiActionConstraint methods
	virtual EJediAiActionResult checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// self is too close to another jedi constraint
//
/////////////////////////////////////////////////////////////////////////////

class CJediAiActionConstraintSelfIsTooCloseToOtherJedi : public CJediAiActionConstraint {
public:
	typedef CJediAiActionConstraint BASECLASS;

	// parameters
	struct {
		bool desiredValue;
	} params;

	// construction
	CJediAiActionConstraintSelfIsTooCloseToOtherJedi();

	// clear all of my data
	virtual void reset();

	// CJediAiActionConstraint methods
	virtual EJediAiActionResult checkConstraint(const CJediAiMemory &simMemory, const CJediAiAction &action, bool simulating) const;
};

#endif // __JEDI_AI_CONSTRAINTS__
