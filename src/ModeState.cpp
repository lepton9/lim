#include "../include/ModeState.h"


ModeState::ModeState() : currentState(State::NORMAL) {}

void ModeState::handleEvent(Event event) {
  switch (currentState) {
  case State::NORMAL:
    if (event == Event::INPUT) sToInput();
    else if (event == Event::COMMAND) sToCommand();
    else if (event == Event::VISUAL) sToVisual();
    else if (event == Event::VLINE) sToVLine();
    break;
  case State::INPUT:
    if (event == Event::BACK) sToNormal();
      break;
  case State::COMMAND:
    if (event == Event::BACK) sToNormal();
    break;
  case State::VISUAL:
    if (event == Event::BACK) sToNormal();
    else if (event == Event::COMMAND) sToCommand();
    else if (event == Event::INPUT) sToInput();
    else if (event == Event::VLINE) sToVLine();
    break;
  case State::VLINE:
    if (event == Event::BACK) sToNormal();
    else if (event == Event::COMMAND) sToCommand();
    else if (event == Event::INPUT) sToInput();
    break;

  default:
    break;
  }
}


void ModeState::sToNormal() {
  currentState = State::NORMAL;
};
void ModeState::sToInput() {
  currentState = State::INPUT;
};
void ModeState::sToCommand() {
  currentState = State::COMMAND;
};
void ModeState::sToVisual() {
  currentState = State::VISUAL;
};
void ModeState::sToVLine() {
  currentState = State::VLINE;
};

