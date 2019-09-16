// Game.h
#ifndef __GAME_H__
#define __GAME_H__

class Actor;

class Game
{
    static Game* s_pGame;
    bool m_quitting;

    Actor* m_pPlayer;
    Actor* m_pAiOpponent;

public:
    static void CreateGame(void);
    static void DestroyGame(void);
    static Game* GetInstance(void) { return s_pGame; }

    Game(void);
    ~Game(void);

    bool Init(void);
    void MainLoop(void);

    void ShowStatus(void);
    void Quit(void) { m_quitting = true; }
    bool IsQuitting(void) const { return m_quitting; }

private:
    void ResetActors(void);
    void HandleGameTransition(void);
};

#endif
