// HumanActor.h
#ifndef __HUMANACTOR_H__
#define __HUMANACTOR_H__

#include "Actor.h"

class HumanActor : public Actor
{
public:
    explicit HumanActor(const std::string& name, int maxHp, int maxPotions, int minDamage, int maxDamage, int minHeal, int maxHeal) 
                : Actor(name, maxHp, maxPotions, minDamage, maxDamage, minHeal, maxHeal) { }

protected:
    virtual ActionType ChooseNextAction(Actor* pOpponent);
};

#endif
