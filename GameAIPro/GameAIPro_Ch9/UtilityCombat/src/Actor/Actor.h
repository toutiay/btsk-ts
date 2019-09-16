// Actor.h
#ifndef __ACTOR_H__
#define __ACTOR_H__

#include "../types.h"
#include <string>


class Actor
{
    const static float RUN_SUCCESS_CHANCE;

protected:
    std::string m_name;

    int m_maxHp;
    int m_hp;

    int m_maxPotions;
    int m_potions;

    int m_minDamage;
    int m_maxDamage;

    int m_minHeal;
    int m_maxHeal;

public:
    explicit Actor(const std::string& name, int maxHp, int maxPotions, int minDamage, int maxDamage, int minHeal, int maxHeal);
    virtual ~Actor(void) { }

    bool PerformNextAction(Actor* pOpponent);  // returns true if the game is over
    virtual void Reset(void);

    void PrintStatus(void);
    bool Hit(int damage);

    const std::string& GetName(void) const { return m_name; }
    int GetHp(void) const { return m_hp; }
    int GetMaxDamage(void) const { return m_maxDamage; }

protected:
    virtual ActionType ChooseNextAction(Actor* pOpponent) = 0;

private:
    bool DoAction(ActionType action, Actor* pOpponent);  // returns true if the opponent died
    bool Attack(Actor* pOpponent);
    bool Run(void);
    void Heal(void);
};

#endif
