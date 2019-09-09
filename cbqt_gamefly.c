#include "cbqt_gamefly.h"

#define K_NULL  0
#define K_ESC   27
#define K_PAUSE 32  // space

struct {

    struct {
        float x;
        float y;
    } coords;

    int pressedKey;

} player_s;

struct {

    int status;

} gamefly_s;

enum {
    GF_EXIT = -1,    // terminate const value
    GF_PAUSE,
    GF_PLAY
};

void keycatch(void)
{
    if (kbhit())
        player_s.pressedKey = getch();
    else
        player_s.pressedKey = K_NULL;
}

int startComponents(int argc, CBQArg_t* argv);
int playerStart(int argc, CBQArg_t* argv);
int showMsg(int argc, CBQArg_t* argv);
int loopMsg(int argc, CBQArg_t* argv);

void CBQ_T_GAMEFLY(void)
{
    CBQueue_t qGame;

    CBQ_QueueInit(&qGame, CBQ_SI_TINY, CBQ_SM_MAX, 0);

    gamefly_s.status = GF_PLAY;

    CBQ_Push(&qGame, startComponents, 1, (CBQArg_t) {.qVar = &qGame});
//    CBQ_Push(&qGame, showMsg, 2, (CBQArg_t) {.qVar = &qGame}, (CBQArg_t) {.sVar = str});

    do {

        if (CBQ_HAVECALL(qGame) && gamefly_s.status == GF_PLAY)
            CBQ_Exec(&qGame, NULL);

        keycatch();

        /* base key events */
        if (player_s.pressedKey == K_PAUSE) {
            if (gamefly_s.status == GF_PLAY)
                gamefly_s.status = GF_PAUSE;
            else if (gamefly_s.status == GF_PAUSE)
                gamefly_s.status = GF_PLAY;
        }

        if (player_s.pressedKey == K_ESC)
            gamefly_s.status = GF_EXIT;

    } while (gamefly_s.status != GF_EXIT);

    CBQ_QueueFree(&qGame);

}

int startComponents(int argc, CBQArg_t* argv)
{

    return 0;
}

int playerStart(int argc, CBQArg_t* argv)
{
    gamefly_s.status = GF_PAUSE;
    return 0;
}

int showMsg(int argc, CBQArg_t* argv)
{
    CBQ_Push(argv[0].qVar, loopMsg, 2, (CBQArg_t) {.qVar = argv[0].qVar}, (CBQArg_t) {.sVar = argv[1].sVar});

    return 0;
}

int loopMsg(int argc, CBQArg_t* argv)
{
    printf("%s", argv[1].sVar);
    fflush(stdout);
    CBQ_Push(argv[0].qVar, loopMsg, 2, (CBQArg_t) {.qVar = argv[0].qVar}, (CBQArg_t) {.sVar = argv[1].sVar});

    return 0;
}
