// AiActor.cpp
#include "AiActor.h"
#include <algorithm>
#include <math.h>
#include <iostream>

using std::cout;

const float AiActor::BASE_ATTACK_SCORE = 0.4f;
const float AiActor::RUN_SCORE_EXPONENT_COEFFICIENT = 0.25f;
const float AiActor::RUN_SCORE_POTION_EFFECT_EXPONENT = 3;
const float AiActor::HEAL_SCORE_LOGISTIC_RANGE = 6.f;
const float AiActor::HEAL_SCORE_STEEPNESS = 1.848431643f;  // e * 0.68 where e == Euler's number

// Set this to 1 if you want the AI to choose an action with a weighted random.  Set it to 0 if you want the 
// AI to choose the top scoring action.
#define USE_WEIGHTED_RANDOM 1

bool ScoreSortingFunction(const AiActor::ActionScore& left, const AiActor::ActionScore& right)
{
    return left.second > right.second;
}

AiActor::AiActor(const std::string& name, int maxHp, int maxPotions, int minDamage, int maxDamage, int minHeal, int maxHeal)
    : Actor(name, maxHp, maxPotions, minDamage, maxDamage, minHeal, maxHeal)
{
    m_lastScores.reserve(NUM_ACTIONS);
}

void AiActor::Reset(void)
{
    Actor::Reset();
    m_lastScores.clear();
}

ActionType AiActor::ChooseNextAction(Actor* pOpponent)
{
    float total = 0;
    m_lastScores.clear();

    // threat score
    float threat = std::min(pOpponent->GetMaxDamage() / (float)m_hp, 1.f);

    // attack
    float score = ScoreAttack(pOpponent->GetHp());
    total += score;
    m_lastScores.push_back(std::make_pair(ACTION_ATTACK, score));

    // run
    score = ScoreRun(threat);
    total += score;
    m_lastScores.push_back(std::make_pair(ACTION_RUN, score));

    // heal
    score = ScoreHeal(threat);
    total += score;
    m_lastScores.push_back(std::make_pair(ACTION_HEAL, score));

    std::sort(m_lastScores.begin(), m_lastScores.end(), &ScoreSortingFunction);

    // logging
    for (std::vector<ActionScore>::iterator it = m_lastScores.begin(); it != m_lastScores.end(); ++it)
    {
        cout << "\tScoring action '" << GetStrFromActionType(it->first) << "': utility = " << it->second << "\n";
    }

#if USE_WEIGHTED_RANDOM
    // select the best score with a weighted random
    float choice = (rand() * (1.f / RAND_MAX)) * total;
    float accumulation = 0;
    for (std::vector<ActionScore>::iterator it = m_lastScores.begin(); it != m_lastScores.end(); ++it)
    {
        accumulation += it->second / total;
        if (choice < accumulation)
            return it->first;
    }

    // something's broken, just run away
    cout << "[ERROR] Failed to find option, which means there's a bug in the weighted random code.";
    return ACTION_RUN;
    
#else
    return m_lastScores[0].first;
#endif
}

float AiActor::ScoreAttack(int opponentHp)
{
    // Linear interpolation from BASE_ATTACK_SCORE to 1 over the course of the opponent's hp going from 
    // max damage to min damage.  In other words, attack score is constant until it becomes possible to 
    // kill the opponent in one hit, at which point it scales up until the opponent is guaranteed to be 
    // killed with one hit.
    float inverseRatio = 1 - ((float)(opponentHp - m_minDamage) / (m_maxDamage - m_minDamage));
    float score = (inverseRatio * (1 - BASE_ATTACK_SCORE)) + BASE_ATTACK_SCORE;
    return std::max(std::min(score, 1.f), BASE_ATTACK_SCORE);
}

float AiActor::ScoreRun(float threatScore)
{
    // This is an exponential curve that gets considerably less steep as the number of potions drop.  The 
    // RUN_SCORE_EXPONENT_COEFFICIENT can be tuned to change the overall steepness of the curve.  As this 
    // coefficient approaches 1, the curve will become more linear.  The RUN_SCORE_POTION_EFFECT_EXPONENT
    // value is used to tune how much of an effect potions have on the curve.  The higher this number, the 
    // sharper the curve is for larger values of m_potions.  When m_potions == 0, this exponent will have 
    // no effect.  Make this number very high if you want it to be nearly impossible for the actor to run
    // while he still has potions.
    float ratio = ((float)m_hp / m_maxHp);
    float exponent = (1 / powf(((float)m_potions + 1), RUN_SCORE_POTION_EFFECT_EXPONENT)) * RUN_SCORE_EXPONENT_COEFFICIENT;
    float score = 1 - powf(ratio, exponent);
    return score * threatScore;
}

float AiActor::ScoreHeal(float threatScore)
{
    // This is a logistic function that has it's biggest rate of change in the center of the data set.
    // This causes light wounds to have very little effect on the actor's desire to heal but as he takes 
    // more damage, it will begin to ramp up very quickly.  The HEAL_SCORE_LOGISTIC_RANGE constant is 
    // used to control which part of the curve the 0 - m_maxHp range is mapped to.  The HEAL_SCORE_STEAPNESS
    // constant controls how sharp/steep the curve is.  A larger number will create a very steep curve 
    // that rises sharply and suddenly.  A smaller number will have a more gradual, softer curve.  A 
    // good rule of thumb for logistics functions is to start with something like Euler's number, which 
    // produces a nicely balanced curve, and then tune from there.  That's how I came up with the currently
    // tuned value.  Also note the early-out if m_potions == 0.
    if (m_potions == 0)
        return 0;
    float exponent = -(((float)m_hp / m_maxHp) * (HEAL_SCORE_LOGISTIC_RANGE * 2)) + HEAL_SCORE_LOGISTIC_RANGE;
    float denominator = 1 + powf(HEAL_SCORE_STEEPNESS, exponent);
    float score = 1 - (1 / denominator);
    return score * threatScore;
}


