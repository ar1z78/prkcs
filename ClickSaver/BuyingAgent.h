#ifndef BUYING_AGENT_H
#define BUYING_AGENT_H


#define BUYINGAGENT_TIMER 1

#include <windows.h>

// Buying agent execution functions
int BuyingAgent(void);
void EndBuyingAgent(void);

HWND hMainWnd;

static UINT_PTR g_TimerID = 0;

// Slider setup and helper functions
void _dragMouse(int x0, int y0, int x1, int y1);
float _linIinterp(float lo, float hi, float ratio);
void _setSliders(int easy_hard, int good_bad, int order_chaos, int open_hidden, int phys_myst, int headon_stealth, int money_xp);

#endif // BUYING_AGENT_H
