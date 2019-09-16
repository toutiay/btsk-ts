// AiActor.h
#ifndef __AIACTOR_H__
#define __AIACTOR_H__

#include "Actor.h"
#include <vector>

class AiActor : public Actor
{
public:
    typedef std::pair<ActionType, float> ActionScore;

private:
    const static float BASE_ATTACK_SCORE;
    const static float RUN_SCORE_EXPONENT_COEFFICIENT;
    const static float RUN_SCORE_POTION_EFFECT_EXPONENT;
    const static float HEAL_SCORE_LOGISTIC_RANGE;
    const static float HEAL_SCORE_STEEPNESS;

    std::vector<ActionScore> m_lastScores;

public:
    explicit AiActor(const std::string& name, int maxHp, int maxPotions, int minDamage, int maxDamage, int minHeal, int maxHeal);
    virtual void Reset(void);

protected:
    virtual ActionType ChooseNextAction(Actor* pOpponent);

private:
    float ScoreAttack(int opponentHp);
    float ScoreRun(float threatScore);
    float ScoreHeal(float threatScore);
};

#endif
