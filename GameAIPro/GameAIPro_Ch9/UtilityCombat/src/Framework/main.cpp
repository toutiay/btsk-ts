// UtilityCombat.cpp
// Author: David "Rez" Graham

#include "Game.h"

#include <iostream>
#include <string>
#include <time.h>

using std::cout;
using std::cin;

int main(void)
{
    srand((unsigned int)time(NULL));

    Game::CreateGame();
    if (!Game::GetInstance()->Init())
    {
        cout << "Failed to initialize game!\n";
        Game::DestroyGame();
        return -1;
    }

    Game::GetInstance()->MainLoop();

    Game::DestroyGame();
    return 0;
}

