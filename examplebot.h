#pragma once

#include "rlbot/bot.h"
#include "falken/actions.h"
#include "falken/brain.h"
#include "falken/episode.h"
#include "falken/observations.h"
#include "falken/service.h"
#include "falken/session.h"

class ExampleBot : public rlbot::Bot {
public:
  ExampleBot(int _index, int _team, std::string _name);
  ~ExampleBot();
  rlbot::Controller GetOutput(rlbot::GameTickPacket gametickpacket) override;

private:
  std::shared_ptr<falken::Service> service;
  std::shared_ptr<falken::BrainBase> brain;
  std::shared_ptr<falken::Session> session;
  std::shared_ptr<falken::Episode> episode;

  const int kMaxSteps = 500;
  bool eval_complete_ = false;
};
