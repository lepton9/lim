#ifndef STEV_H
#define STEV_H

enum class State {
  NORMAL,
  INPUT,
  COMMAND,
  VISUAL,
  VLINE
};

enum class Event {
  BACK,
  INPUT,
  COMMAND,
  VISUAL,
  VLINE
};

#endif
