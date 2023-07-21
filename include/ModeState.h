#ifndef MODESTATE_H
#define MODESTATE_H

#include "stev.h"

class ModeState {
  public:
    ModeState();

    State currentState;
    void handleEvent(Event event);

  private:

    void sToNormal();
    void sToInput();
    void sToCommand();
    void sToVisual();
    void sToVLine();
};

#endif
