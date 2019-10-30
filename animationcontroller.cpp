#include "animationcontroller.h"
#include <cstdio>

AnimationController::AnimationController(){
  speed = 1;
  frameCount = 50;
  currFrame = 0;
}

AnimationController::AnimationController(int numFrames){
  speed = 1;
  frameCount = numFrames;
  currFrame = 0;
}

void AnimationController::setFrameCount(int numFrames){
  if (currFrame >= numFrames){
    currFrame = numFrames - 1;
  }
  frameCount = numFrames;
}

void AnimationController::setFrame(int frame)
{
	if (frame < 0) {
		currFrame = 0;
	}
	else if (frame > frameCount) {
		currFrame = frameCount;
	}
	else {
		currFrame = frame;
	}
}

int AnimationController::getFrame(){
  double currentTime = (double) clock();
  if (playing){
    currFrame++;
    double time_passed =0;//TEMP: JUST ALWAYS GO TO NEXT FRAME (currentTime - startTime)/CLOCKS_PER_SEC;
    if (time_passed > (1.0/speed)){
      currFrame++;
      printf("st %f, ct %f, ft %f\n", startTime, currentTime, 1.0/speed);
      startTime = currentTime;
    }
    if (currFrame >= frameCount){
      currFrame = 0;
    }
  }
  //printf("Frame time : %f \n", currentTime - lastTime);
  lastTime = currentTime;
  return currFrame;
}

void AnimationController::setSpeed(float newSpeed){
  speed = newSpeed;
  ValidateSpeed();
}

float AnimationController::getSpeed(){
  return speed;
}


void AnimationController::increaseSpeed(){
  speed *= speedMultiplier;
  ValidateSpeed();
}

void AnimationController::decreaseSpeed(){
  speed /= speedMultiplier;
  ValidateSpeed();
}

void AnimationController::ValidateSpeed(){
  if (speed <= 0){
    speed = 0.01;
  }
}

void AnimationController::play(){
  playing = true;
}

void AnimationController::pause(){
  playing = false;
}

void AnimationController::togglePlay(){
  playing = !playing;
}

void AnimationController::stepForward(){
  currFrame++;
  if (currFrame >= frameCount){
    currFrame = 0;
  }
}


void AnimationController::stepBackward(){
  currFrame--;
  if (currFrame < 0){
    currFrame = frameCount - 1;
  }
}
