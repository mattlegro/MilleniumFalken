
# Millenium Falken

## Problem Statement

Rocket League is a game developed by Psyonix where rocket powered cars play soccer in an enclosed indoor soccer style arena. In their non-ranked team based modes, when a human player leaves they are replaced by Psyonix's hard-coded AI. The AI, unfortunately, is significantly worse than the 50th percentile player even at its highest difficulty. This leads to disadvantaged teams upon bot insertion in the upper echelons of player skill tiers. A strong player community, RL Bot, exists and is doing their best to build stronger AI. While hard coded bots can handily beat Psyonix AI, they are still only as good as the average RL player. The holy grail of the RL Bot community is a bot built on machine learning techniques, as policy networks in other games have achieved mastery given enough time in tuning, with one of the first being AlphaStar in Starcraft II. 

## Primary Objectives

In July of this year, Google Research released an Imitation Learning framework named `falken`. The stated purpose of this framework is to allow game developers to reduce manual QA test time by teaching AI to play their games, allowing it to continuously replay the game and search for bugs. In this project, this framework is instead applied in learning to play Rocket League, interfacing with the game state through the `RLBot` framework, as a proof of concept. The result, in this case an AI named MilleniumFalken, is encouraging enough to pursue, with a lofty goal of using player data from each skill tier as 'expert' demonstration data in order to train AI appropriate to each player skill tier.

## Current Performance

To learn, the `falken` service is first provided with demonstration data containing Actions and Observables which it uses to train a brain. In this case, the provided Actions are the throttle and steering, and the Observables are the car and ball's rotations and positions. The demonstration data is provided by a slightly modified At-Ball agent, wherein the bot is hard-coded to turn until the bot facing direction is toward the ball while continuously holding the throttle. When the car touches the ball, or the training episode reaches a maximum length, the ball and car are reset to random locations and orientations in the arena. After some time, control is given from the At-Ball agent to the brain, and the brain takes in the Observables and outputs the Actions. More training frames are recorded while under brain control, and a model is created. The bot then enters an evaluation session where it creates a summary map of the brain's performance and saves a snapshot of the brain's policy state, which can be reloaded for later use.

#### ATBA Agent Example:

https://user-images.githubusercontent.com/71366779/132915451-9dd456aa-1c2b-46a4-b4d5-cbbaef8bacb2.mp4

#### Switching Control Example:

https://user-images.githubusercontent.com/71366779/132915516-614d0724-0624-4e1c-991d-2d421cb89e49.mp4

#### Brain Success Examples:

https://user-images.githubusercontent.com/71366779/132915659-e9f2ec57-8206-4b4f-8f9c-14ba438d9bab.mp4

## Growth Areas

While there was success in combining the above frameworks, there remains challenges related to the implementation. The `falken` framework is meant for game developers; people who have access to the source game files. Not being a developer at Psyonix, I do not have that access. Thus, I must inject `falken` as a listener through a C++ bot within the RL Bot framework, leading to imperfections in the data provided to the bot due to missed ticks, and crashes during learning, both impeding progress. Further development could yield workarounds and smooth out some of these issues; as the available time for working on this project was under a month, it was not possible to pursue many avenues of improvement.

Yet, the success seen thus far is encouraging. The success states reached above were achieved with minimal training of only a couple minutes at normal game speed. While the player skill cap is much higher than an At-Ball agent can demonstrate, and the bot currently does not consider many of the more complicated aspects of the player's ability to control the car, this proof of concept confirms that this service is capable of learning at least some facets of Rocket League.

