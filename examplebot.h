#pragma once

#include "rlbot/bot.h"
#include "falken/actions.h"
#include "falken/brain.h"
#include "falken/episode.h"
#include "falken/observations.h"
#include "falken/service.h"
#include "falken/session.h"
#include "falken/primitives.h"
#include "linear_algebra/math.h"

class ExampleBot : public rlbot::Bot {
public:
  ExampleBot(int _index, int _team, std::string _name);
  ~ExampleBot();
  rlbot::Controller GetOutput(rlbot::GameTickPacket gametickpacket) override;

private:
  bool ExampleBot::InitializeFalken();
  bool ExampleBot::FloatEquals(float a, float b);
  void ExampleBot::Reset();
  float ExampleBot::RandomFloat(float Min, float Max);
  void ExampleBot::StartInferenceSession();
  void ExampleBot::StartEvaluationSession();

  std::shared_ptr<falken::Service> service;
  std::shared_ptr<falken::BrainBase> brain;
  std::shared_ptr<falken::Session> session;
  std::shared_ptr<falken::Episode> episode;

  const int kMaxSteps = 120 * 10 * 1;
  bool brain_control = false;
  float last_touch_time = 0.f;
  bool initialized = false;
  bool inferenceStarted = false;
  float absTol = .00005;
  int switchThreshold = 30;
  int touchCounter = 0;
};
