#pragma once

#include "glm/glm.hpp"
#include <vector>
#include <utility>
#include <unordered_map>
#include <string>

//Represents a Von Mises Fisher distribution in 3 dimensions
class VMF{
public:
  double kappa;
  glm::vec3 center;

  //Computes the Bhattacharyya distance between two distributions
  double compare(VMF& other);
  VMF(double k, glm::vec3 c){
    kappa = k;
    center = c;
  }
};

//Represents a mixture of VMF distribution (in 3 dimensions)
class MoVMF{
public:
  std::vector<std::pair<double, VMF> > distributions;

  void addComponent(double weight, VMF distribution);
  double compare(MoVMF& other);

  MoVMF(){}
  MoVMF(std::vector<std::pair<double, VMF> > d){
    distributions = d;
  }

};

std::unordered_map<int, MoVMF> readMoVMFs(std::string fileName);
  
MoVMF combineMoVMFs(MoVMF& a, double aw, MoVMF& b, double bw);

double bessel(double x);
