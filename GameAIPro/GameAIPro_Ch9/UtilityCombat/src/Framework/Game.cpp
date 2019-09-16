// Game.cpp
#include "Game.h"

#include "../Actor/HumanActor.h"
#include "../Actor/AiActor.h"

#include <iostream>

using std::cout;


Game* Game::s_pGame = NULL;

void Game::CreateGame(void)
{
    if (s_pGame == NULL)
        s_pGame = new Game;
}

void Game::DestroyGame(void)
{
    delete s_pGame;
    s_pGame = NULL;
}


Game::Game(void)
{
    m_pPlayer = NULL;
    m_pAiOpponent = NULL;
    m_quitting = false;
}

Game::~Game(void)
{
    delete m_pPlayer;
    m_pPlayer = NULL;

    delete m_pAiOpponent;
    m_pAiOpponent = NULL;
}

bool Game::Init(void)
{
    m_pPlayer = new HumanActor("Player", 100, 3, 15, 20, 45, 75);
    m_pAiOpponent = new AiActor("Ferocious Utility Curve", 100, 3, 10, 25, 45, 75);

    return true;
}

void Game::MainLoop(void)
{
    HandleGameTransition();

    while (!IsQuitting())
    {
        ShowStatus();

        bool gameOver = m_pPlayer->PerformNextAction(m_pAiOpponent);
        if (gameOver)
        {
            HandleGameTransition();
            continue;
        }

        gameOver = m_pAiOpponent->PerformNextAction(m_pPlayer);
        if (gameOver)
        {
            HandleGameTransition();
            continue;
        }
    }
}

void Game::ShowStatus(void)
{
    m_pPlayer->PrintStatus();
    m_pAiOpponent->PrintStatus();
}

void Game::ResetActors(void)
{
    m_pPlayer->Reset();
    m_pAiOpponent->Reset();
}

void Game::HandleGameTransition(void)
{
    if (!IsQuitting())
    {
        ResetActors();
        cout << "A " << m_pAiOpponent->GetName() << " draws near!  Command?\n";
    }
}

