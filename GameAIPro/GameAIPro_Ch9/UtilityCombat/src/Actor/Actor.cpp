// Actor.cpp
#include "Actor.h"
#include <iostream>

using std::cout;

const float Actor::RUN_SUCCESS_CHANCE = 0.5f;  // there is a 50% chance for success when attempting to run

Actor::Actor(const std::string& name, int maxHp, int maxPotions, int minDamage, int maxDamage, int minHeal, int maxHeal)
    : m_name(name)
{
    m_maxHp = maxHp;
    m_maxPotions = maxPotions;
    m_minDamage = minDamage;
    m_maxDamage = maxDamage;
    m_minHeal = minHeal;
    m_maxHeal = maxHeal;

    m_hp = m_maxHp;
    m_potions = m_maxPotions;
}

bool Actor::PerformNextAction(Actor* pOpponent)
{
    ActionType action = ChooseNextAction(pOpponent);
    return DoAction(action, pOpponent);
}

void Actor::Reset(void)
{
    m_hp = m_maxHp;
    m_potions = m_maxPotions;
}

void Actor::PrintStatus(void)
{
    cout << m_name << "; hp = " << m_hp << "; potion = " << m_potions << "\n";
}

bool Actor::Hit(int damage)
{
    m_hp -= damage;
    if (m_hp < 0)
    {
        cout << m_name << " is dead!\n";
        return true;
    }
    return false;
}

bool Actor::DoAction(ActionType action, Actor* pOpponent)
{
    switch (action)
    {
        case ACTION_ATTACK : return Attack(pOpponent);
        case ACTION_RUN : return Run();
        case ACTION_HEAL : Heal(); return false;
        case ACTION_NULL : return true;
        default : cout << "[ERROR] Unknown action " << action << "\n";
    }

    return false;
}

bool Actor::Attack(Actor* pOpponent)
{
    int damage = (rand() % (m_maxDamage - m_minDamage)) + m_minDamage;
    cout << m_name << " struck for " << damage << " damage\n\n";
    return pOpponent->Hit(damage);
}

bool Actor::Run(void)
{
    float normalizedRand = (float)rand() / (RAND_MAX + 1);
    if (normalizedRand < RUN_SUCCESS_CHANCE)
    {
        cout << m_name << " ran away!\n\n";
        return true;
    }
    else
    {
        cout << m_name << " failed to run away!\n\n";
        return false;
    }
}

void Actor::Heal(void)
{
    if (m_potions > 0)
    {
        int healAmount = (rand() % (m_maxHeal - m_minHeal)) + m_minHeal;
        cout << m_name << " healed " << healAmount << " hp.\n\n";
        m_hp += healAmount;
        if (m_hp > m_maxHp)
            m_hp = m_maxHp;
        --m_potions;
    }
    else
    {
        cout << m_name << " attempted to heal but had no potions left.\n\n";
    }
}

