#pragma once


#include <chrono>


/**
 * AnimationController controls the timing of the animation.
 * The primary purpose is to keep the animation synchronized between the various nodes.
 * The secondary purpose (currently disabled) is to keep a constant animation rate despite changing framerate.
 * To operate the getFrame function should be called once per graphics frame to get the animation frame to be rendered.
 * It is critical to either use the constructor which specifies the number of frames or to manually set it using setFrameCount.
 */
class AnimationController
{

public:
  AnimationController(int numFrames);
  AnimationController();
  void setFrameCount(int numFrames);
  void setFrame(int frame);

  int getFrame();

  //we need to add an extra function for retrieving the current frame as getFrame advances the currentFrame.
  int getCurrentFrame() {
	  return currFrame;
  }
  //we also need to know when toggle pauses the animation to sync the frame
  bool isPlaying() {
	  return playing;
  }
  void setSpeed(float speed);
  float getSpeed();
  void increaseSpeed();
  void decreaseSpeed();

  void play();
  void pause();
  void togglePlay();

  void stepForward();
  void stepBackward();


private:
  const float speedMultiplier = 1.05;
  float speed;
  int frameCount;
  int currFrame;
  bool playing = false;
  double startTime;
  double lastTime;

  void ValidateSpeed();
};
