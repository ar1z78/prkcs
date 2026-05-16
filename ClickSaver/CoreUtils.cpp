#include "CoreUtils.h"
#include <windows.h>

extern "C" void ShowErrorMessage(const char* message)
{
    MessageBoxA(NULL, message, "ClickSaver Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
}
