#include "examplebot.h"

#include <ctime>
#include <math.h>
#include <string>
#include <assert.h>
#include <iostream>

#include "rlbot/bot.h"
#include "rlbot/color.h"
#include "rlbot/interface.h"
#include "rlbot/rlbot_generated.h"
#include "rlbot/scopedrenderer.h"
#include "rlbot/statesetting.h"

#define PI 3.1415

ExampleBot::ExampleBot(int _index, int _team, std::string _name)
	: Bot(_index, _team, _name) {
}

ExampleBot::~ExampleBot() {
	// Free your allocated memory here.
}

rlbot::Controller ExampleBot::GetOutput(rlbot::GameTickPacket gametickpacket) {

	if (!initialized) {
		initialized = InitializeFalken();
	}

	bool training_complete =
		//(gametickpacket->gameInfo()->secondsElapsed() > snapTime);
		session->training_state() == falken::Session::kTrainingStateComplete;
	switch (session->type()) {
	case falken::Session::kTypeInteractiveTraining:
		if (training_complete) {
			//snapTime += 60;
			StartEvaluationSession();
		}
			break;
	case falken::Session::kTypeEvaluation:
		if (training_complete) {
			//snapTime += 60;
			StartInferenceSession();
		}
		break;
	default:
		break;
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

	//falken::Rotation falk_car_rot = falken::Rotation::FromEulerAngles(
	//	carRotation.pitch(), carRotation.yaw(), carRotation.roll());
	//falken::Rotation falk_ball_rot = falken::Rotation::FromEulerAngles(
	//	ballRotation.pitch(), ballRotation.yaw(), ballRotation.roll());

	//vec3 car_r = vec3({ carRotation.pitch(),carRotation.yaw(),carRotation.roll() });
	vec3 car_r = vec3({ 0,carRotation.yaw(),0 });
	mat3 car_m = euler_to_rotation(car_r);
	quaternion car_quat = rotation_to_quaternion(car_m);

	vec3 ball_r = vec3({ ballRotation.pitch(),ballRotation.yaw(),ballRotation.roll() });
	//vec3 ball_r = vec3({ 0,0,0 });
	mat3 ball_m = euler_to_rotation(ball_r);
	quaternion ball_quat = rotation_to_quaternion(ball_m);

	falken::Position falk_car_pos = falken::Position({
		carLocation.x(),carLocation.z(),-1 * carLocation.y() });
	falken::Rotation falk_car_rot = falken::Rotation({
		car_quat[0],car_quat[1],car_quat[2],car_quat[3] });
	falken::Position falk_ball_pos = falken::Position({
		ballLocation.x(),ballLocation.z(),-1 * ballLocation.y() });
	falken::Rotation falk_ball_rot = falken::Rotation({
		ball_quat[0],ball_quat[1],ball_quat[2],ball_quat[3] });


	brain_spec.observations_base().position.set_value(falk_car_pos);
	brain_spec.observations_base().rotation.set_value(falk_car_rot);
	brain_spec.observations_base().entity("entity_0")->position.set_value(falk_ball_pos
	);
	brain_spec.observations_base().entity("entity_0")->rotation.set_value(falk_ball_rot);

	// Calculate the velocity of the ball.
	//float velocity = sqrt(ballVelocity.x() * ballVelocity.x() +
	//                      ballVelocity.y() * ballVelocity.y() +
	//                      ballVelocity.z() * ballVelocity.z());

	// This renderer will build and send the packet once it goes out of scope.
	rlbot::ScopedRenderer renderer("test");

	// Load the ballprediction into a vector to use for rendering.
	//std::vector<const rlbot::flat::Vector3 *> points;

	//rlbot::BallPrediction ballprediction = GetBallPrediction();

	//for (uint32_t i = 0; i < ballprediction->slices()->size(); i++) {
	//  points.push_back(ballprediction->slices()->Get(i)->physics()->location());
	//}

	//renderer.DrawPolyLine3D(rlbot::Color::red, points);

	rlbot::Controller controller{ 0 };

	if (!brain_control) {
		renderer.DrawString2D("ATBA CONTROL", rlbot::Color::green,
			rlbot::flat::Vector3{ 10, 10, 0 }, 4, 4);
		// Calculate to get the angle from the front of the bot's car to the ball.
		double botToTargetAngle = atan2(ballLocation.y() - carLocation.y(),
			ballLocation.x() - carLocation.x());
		double botFrontToTargetAngle = botToTargetAngle - carRotation.yaw();
		// Correct the angle.
		if (botFrontToTargetAngle < -PI)
			botFrontToTargetAngle += 2 * PI;
		if (botFrontToTargetAngle > PI)
			botFrontToTargetAngle -= 2 * PI;

		// Decide which way to steer in order to get to the ball.
		if (abs(botFrontToTargetAngle) > .01) {
			if (botFrontToTargetAngle > 0) {
				controller.steer = 1;
			}
			else {
				controller.steer = -1;
			}
		}
		else {
			controller.steer = 0;
		}

		controller.throttle = 1.0f;

		brain_spec.actions_base().set_source(
			falken::ActionsBase::kSourceHumanDemonstration);

		brain_spec.actions_base().attribute("steering")->set_joystick_x_axis(
			controller.steer);
		brain_spec.actions_base().attribute("steering")->set_joystick_y_axis(0);
		brain_spec.actions_base().attribute("throttle")->set_number(
			controller.throttle);
	} else {
		brain_spec.actions_base().set_source(falken::ActionsBase::kSourceNone);
	}

	episode->Step(0.f);

	if (episode->completed()) {
		episode = nullptr;
		Reset();
		episode = session->StartEpisode();
	} else if (brain_control) {
		renderer.DrawString2D("BRAIN CONTROL", rlbot::Color::blue,
			rlbot::flat::Vector3{ 10, 10, 0 }, 4, 4);
		float joystick_steer = brain_spec.actions_base().attribute("steering")->
			joystick_x_axis();
		if (isnan(joystick_steer)) {
			controller.steer = 0;
		} else {
			controller.steer = joystick_steer;
		}
		controller.throttle = brain_spec.actions_base().attribute("throttle")->
			number();
	}
	//renderer.DrawString3D(std::to_string(velocity), rlbot::Color::magenta,
	//                      ballLocation, 2, 2);


	if (gametickpacket->ball()->latestTouch() != NULL) {
		float new_touch_time = gametickpacket->ball()->latestTouch()->gameSeconds();
		int new_touch_player = gametickpacket->ball()->latestTouch()->playerIndex();
		if (!FloatEquals(new_touch_time, last_touch_time) && new_touch_player == index) {
			std::cout << "I touched it!!";
			if (brain_control) {
				touchCounter += 1;
				if (touchCounter > 2) {
				switchThreshold += 240;
				}
			}
			last_touch_time = new_touch_time;
			Reset();
			episode->Complete(falken::Episode::kCompletionStateSuccess);
			episode = session->StartEpisode();
		}
	}

	if (gametickpacket->gameInfo()->secondsElapsed() > switchThreshold) {
		if (!brain_control) {
			switchThreshold += 30;
			brain_control = true;
			touchCounter = 0;
		}
		else {
			switchThreshold += 20;
			brain_control = false;
		}
	}

	//if (gametickpacket->gameInfo()->secondsElapsed() > snapTime) {
	//	snapTime += 60 * 15;
	//	episode->Complete(falken::Episode::kCompletionStateAborted);
	//	episode = nullptr;
	//	auto snapshot_id = session->Stop();
	//	session = nullptr;
	//	session = brain->StartSession(
	//		falken::Session::kTypeInteractiveTraining , kMaxSteps);
	//	Reset();
	//	//brain_control = true;
	//	episode = session->StartEpisode();
	//}

	return controller;
}

bool ExampleBot::InitializeFalken() {

	falken::ObservationsBase observations;
	falken::EntityBase entity_0(observations, "entity_0");

	falken::ActionsBase actions;

	falken::AttributeBase steering(actions, "steering",
		falken::kAxesModeDeltaPitchYaw,
		falken::kControlledEntityPlayer,
		falken::kControlFrameWorld);

	falken::AttributeBase throttle(actions, "throttle", -1.0f, 1.0f);

	falken::BrainSpecBase brain_spec_base(&observations, &actions);

	static const char* kJsonConfig = nullptr;
	static const char* project_id = nullptr; /*"MilleniumFalken";*/
	static const char* api_key = nullptr;
	service = falken::Service::Connect(
		project_id, api_key, kJsonConfig);

	static const char* kBrainName = "MilleniumFalken";
	const char* brain_id = "953716c9-a336-4e4d-a38f-899ee9e4fdb9";
	const char* snapshot_id = "d9621174-1f29-4473-8990-ddef50ede5f2";

	//brain = service->CreateBrain(kBrainName, brain_spec_base);
	brain = service->GetBrain(brain_id, snapshot_id);

	session = brain->StartSession(
		falken::Session::kTypeInteractiveTraining, kMaxSteps);
	//session = brain->StartSession(
	//	falken::Session::kTypeInference, kMaxSteps);

	episode = session->StartEpisode();
	return true;
}
bool ExampleBot::FloatEquals(float a, float b) {
	return (std::abs(a - b) <= absTol * std::max({ 1.0f, std::abs(a), std::abs(b) }));
}

void ExampleBot::Reset() {
	rlbot::GameState gamestate = rlbot::GameState();
	rlbot::CarState carstate = rlbot::CarState();

	float ball_x = RandomFloat(-3000.f, 3000.f);
	float ball_y = RandomFloat(-2500.f, 2500.f);
	float car_x = RandomFloat(-3500.f, 3500.f);
	float car_y = RandomFloat(-4000.f, 4000.f);
	float car_yaw = RandomFloat(-PI, PI);
	float small_v = RandomFloat(-.5f, .5f);

	carstate.physicsState.location = { car_x, car_y, 17.01 };
	carstate.physicsState.velocity = { 0, 0, 0 };
	carstate.physicsState.rotation = { 0, car_yaw, 0 };
	gamestate.ballState.physicsState.location = { ball_x, ball_y, 92.76 };
	gamestate.ballState.physicsState.rotation = { 0, 0, 0 };
	gamestate.ballState.physicsState.velocity = { small_v, 0, 0 };
	gamestate.carStates[index] = carstate;

	rlbot::Interface::SetGameState(gamestate);
}

float ExampleBot::RandomFloat(float Min, float Max) {
	return ((float(rand()) / float(RAND_MAX)) * (Max - Min)) + Min;
}

void ExampleBot::StartEvaluationSession() {
	episode->Complete(falken::Episode::kCompletionStateAborted);
	episode = nullptr;
	auto snapshot_id = session->Stop();
	session = nullptr;
	session = brain->StartSession(falken::Session::kTypeEvaluation, kMaxSteps);
	Reset();
	episode = session->StartEpisode();
	brain_control = true;
}

void ExampleBot::StartInferenceSession() {
	// Because the episode was started at Reset, Step the episode before stopping
	// with an empty episode.
	episode->Step(0.f);
	episode->Complete(falken::Episode::kCompletionStateAborted);
	episode = nullptr;
	auto snapshot_id = session->Stop();
	session = nullptr;
	session = brain->StartSession(falken::Session::kTypeInference, kMaxSteps);
	Reset();
	episode = session->StartEpisode();
}