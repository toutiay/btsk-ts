// HumanActor.cpp
#include "HumanActor.h"
#include "../Framework/Game.h"
#include <iostream>

using std::cout;
using std::cin;

ActionType HumanActor::ChooseNextAction(Actor* pOpponent)
{
    cout << "\n";
    cout << "1) Attack\n";
    cout << "2) Run\n";
    cout << "3) Heal\n";
    cout << "4) Status\n";
    cout << "Q to quit\n";

    while (true)
    {
        char choice;
        cin >> choice;

        switch (choice)
        {
            // actions
            case '1' : return ACTION_ATTACK;
            case '2' : return ACTION_RUN;
            case '3' : return ACTION_HEAL;

            // other commands
            case '4' : 
            {
                Game::GetInstance()->ShowStatus();
                break;
            }
            case 'Q' :
            case 'q' :
            {
                Game::GetInstance()->Quit();
                return ACTION_NULL;
            }

            default : cout << "Unknown command '" << choice << "'\n";
        }
    }
}



