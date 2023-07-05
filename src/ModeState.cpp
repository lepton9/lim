#include "../include/ModeState.h"


ModeState::ModeState() : currentState(State::NORMAL) {}

void ModeState::handleEvent(Event event) {
  switch (currentState) {
  case State::NORMAL:
    if(event == Event::INPUT) sToInput();
    if (event == Event::COMMAND) sToCommand();
    break;
  case State::INPUT:
    if (event == Event::BACK) sToNormal();
      break;
  case State::COMMAND:
      if (event == Event::BACK) sToNormal();
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

