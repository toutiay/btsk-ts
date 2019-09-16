// Types.h
#ifndef __TYPES_H__
#define __TYPES_H__

enum ActionType
{
    ACTION_ATTACK,
    ACTION_RUN,
    ACTION_HEAL,
    NUM_ACTIONS,
    ACTION_NULL  // used for error conditions; this appears after NUM_ACTIONS because it is not counted as a true action
};

inline const char* GetStrFromActionType(ActionType type)
{
    switch (type)
    {
        case ACTION_ATTACK : return "Attack";
        case ACTION_RUN : return "Run";
        case ACTION_HEAL : return "Heal";
        default : return "<Unknown>";
    }
}

#endif
