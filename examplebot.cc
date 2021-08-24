#include "examplebot.h"

#include <ctime>
#include <math.h>
#include <string>
#include <assert.h>

#include "rlbot/bot.h"
#include "rlbot/color.h"
#include "rlbot/interface.h"
#include "rlbot/rlbot_generated.h"
#include "rlbot/scopedrenderer.h"
#include "rlbot/statesetting.h"

#define PI 3.1415

ExampleBot::ExampleBot(int _index, int _team, std::string _name)
    : Bot(_index, _team, _name) {
  rlbot::GameState gamestate = rlbot::GameState();

  //gamestate.ballState.physicsState.location = {0, 0, 1000};
  //gamestate.ballState.physicsState.velocity = {0, 0, 5000};

  //rlbot::CarState carstate = rlbot::CarState();
  //carstate.physicsState.location = {0, 500, 1000};
  //carstate.physicsState.velocity = {500, 1000, 1000};
  //carstate.physicsState.angularVelocity = {1, 2, 3};

  //carstate.boostAmount = 50;

  //gamestate.carStates[_index] = carstate;

  //rlbot::Interface::SetGameState(gamestate);

}

ExampleBot::~ExampleBot() {
  // Free your allocated memory here.
}

rlbot::Controller ExampleBot::GetOutput(rlbot::GameTickPacket gametickpacket) {

  if (!initialized) {
      initialized = InitializeFalken();
  }

  auto& brain_spec = brain->brain_spec_base();
    
  rlbot::flat::Vector3 ballLocation =
      *gametickpacket->ball()->physics()->location();
  rlbot::flat::Rotator ballRotation =
      *gametickpacket->ball()->physics()->rotation();
  rlbot::flat::Vector3 carLocation =
      *gametickpacket->players()->Get(index)->physics()->location();
  rlbot::flat::Rotator carRotation =
      *gametickpacket->players()->Get(index)->physics()->rotation();

  vec3 car_r = vec3({carRotation.roll(),carRotation.pitch(),carRotation.yaw()});
  mat3 car_m = euler_to_rotation(car_r);
  quaternion car_quat = rotation_to_quaternion(car_m);

  vec3 ball_r = vec3({ ballRotation.roll(),ballRotation.pitch(),ballRotation.yaw() });
  mat3 ball_m = euler_to_rotation(ball_r);
  quaternion ball_quat = rotation_to_quaternion(ball_m);

  brain_spec.observations_base().position.set_value(falken::Position({
      carLocation.x(),carLocation.y(),carLocation.z() }));
  brain_spec.observations_base().rotation.set_value(falken::Rotation({
      car_quat[0],car_quat[1],car_quat[2],car_quat[3]}));
  brain_spec.observations_base().entity("ball")->position.set_value(
      falken::Position({ballLocation.x(),ballLocation.y(),ballLocation.z()}));
  brain_spec.observations_base().entity("ball")->rotation.set_value(
      falken::Rotation({ball_quat[0],ball_quat[1],ball_quat[2],ball_quat[3] }));

  // Calculate the velocity of the ball.
  //float velocity = sqrt(ballVelocity.x() * ballVelocity.x() +
  //                      ballVelocity.y() * ballVelocity.y() +
  //                      ballVelocity.z() * ballVelocity.z());

  // This renderer will build and send the packet once it goes out of scope.
  //rlbot::ScopedRenderer renderer("test");

  // Load the ballprediction into a vector to use for rendering.
  //std::vector<const rlbot::flat::Vector3 *> points;

  //rlbot::BallPrediction ballprediction = GetBallPrediction();

  //for (uint32_t i = 0; i < ballprediction->slices()->size(); i++) {
  //  points.push_back(ballprediction->slices()->Get(i)->physics()->location());
  //}

  //renderer.DrawPolyLine3D(rlbot::Color::red, points);

  //renderer.DrawString2D("Hello world!", rlbot::Color::green,
  //                      rlbot::flat::Vector3{10, 10, 0}, 4, 4);
  //renderer.DrawString3D(std::to_string(velocity), rlbot::Color::magenta,
  //                      ballLocation, 2, 2);

  // Calculate to get the angle from the front of the bot's car to the ball.
  double botToTargetAngle = atan2(ballLocation.y() - carLocation.y(),
                                  ballLocation.x() - carLocation.x());
  double botFrontToTargetAngle = botToTargetAngle - carRotation.yaw();
  // Correct the angle.
  if (botFrontToTargetAngle < -PI)
    botFrontToTargetAngle += 2 * PI;
  if (botFrontToTargetAngle > PI)
    botFrontToTargetAngle -= 2 * PI;

  rlbot::Controller controller{0};

  // Decide which way to steer in order to get to the ball.
  if (botFrontToTargetAngle > 0)
    controller.steer = 1;
  else
    controller.steer = -1;

  controller.throttle = 1.0f;

  brain_spec.actions_base().set_source(
      falken::ActionsBase::kSourceHumanDemonstration);

  brain_spec.actions_base().attribute("steering")->set_joystick_x_axis(
      controller.steer);
  brain_spec.actions_base().attribute("steering")->set_joystick_y_axis(0);
  brain_spec.actions_base().attribute("throttle")->set_number(
      controller.throttle);

  episode->Step(0.f);

  if (episode->completed()) {
      episode = nullptr;
      episode = session->StartEpisode();
  }

  //if (last_touch_time == 0.f) {
  //    last_touch_time = gametickpacket->ball()->latestTouch()->gameSeconds();
  //}
  //float new_touch_time = gametickpacket->ball()->latestTouch()->gameSeconds();
  //if (new_touch_time != last_touch_time) {
  //    episode->Complete(falken::Episode::kCompletionStateSuccess);
  //    last_touch_time = new_touch_time;
  //    episode = session->StartEpisode();
  //}

  if (gametickpacket->gameInfo()->isMatchEnded()) {
      episode->Complete(falken::Episode::kCompletionStateAborted);
      episode = nullptr;
      auto snapshot_id = session->Stop();
      session = nullptr;
      brain = nullptr;
      service = nullptr;
  }

  return controller;
}
bool ExampleBot::InitializeFalken() {
    
    falken::ObservationsBase observations;
    falken::EntityBase ball(observations, "ball");

    falken::ActionsBase actions;

    falken::AttributeBase steering(actions, "steering",
        falken::kAxesModeDeltaPitchYaw,
        falken::kControlledEntityPlayer,
        falken::kControlFrameWorld);

    falken::AttributeBase throttle(actions, "throttle", -1.0f, 1.0f);

    falken::BrainSpecBase brain_spec_base(&observations, &actions);

    static const char *kJsonConfig = nullptr;
    static const char* project_id = nullptr; /*"MilleniumFalken";*/
    static const char* api_key = nullptr;  /*"4qw5VdOpS1K4iMgBfiV3GA==";*/
    service = falken::Service::Connect(
        project_id, api_key, kJsonConfig);

    static const char *kBrainName = "MilleniumFalken";
    const char *brain_id;
    const char *snapshot_id;

    brain = service->CreateBrain(kBrainName, brain_spec_base);

    session = brain->StartSession(
        falken::Session::kTypeInteractiveTraining, kMaxSteps);

    episode = session->StartEpisode();
    return true;
}