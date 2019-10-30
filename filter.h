#pragma once

#include "vrpoint.h"


/**
 * \brief Filter represents a way of selecting a subset of points to be visible at a time
 */
class Filter {
public:
  Filter(){};
  /**
   *  Checks if a point should be visible or not.
   *  @param p The VRPoint in question
   *  @return true if the point should be shown, false if it should be hidden
   */
  virtual bool checkPoint(VRPoint& p);
};

/**
 *  MotionFilter is a Filter that shows only points which have moved more than a threshold throughout their path.
 */
class MotionFilter : public Filter {
public:
  MotionFilter();
  /**
   * Creates a MotionFilter 
   * @param threshold the distance for a point to have travelled 
   */
  MotionFilter(float threshold);
  virtual bool checkPoint(VRPoint& p);
private:
  float threshold; ///< the threshold for the Filter
};

/**
 * SliceFilter is a Filter that shows points within horizontal slices based on starting location
 */
class SliceFilter : public Filter{
public:
  //SliceFilter();
  SliceFilter(float start = 0, float gap = .04, float size = .005);
  virtual bool checkPoint(VRPoint& p);
  void addStart(float amt); ///<Sets the start value
  void addGap(float amt); ///<Sets the gap value
  void addSize(float amt); ///<Sets the size value

private:
  float start; ///< Starting position for the filter
  float gap; ///< Gap between centers of layers
  float size; ///<Size of each layer.
};
