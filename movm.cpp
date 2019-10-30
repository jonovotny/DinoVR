#include "movm.h"
#include <cstdio>
#include <cmath>

#include <iostream>
#include <fstream>
#include <sstream>


//closed form taken from Calderara et al 2011 (equation 9)
//I should probably do some extra checks that this actually applies as well as I hope
double VMF::compare(VMF& other){
  double leadingCoefficient = 1 / sqrt(bessel(kappa) * bessel(other.kappa));
  double cosine = glm::dot(center, other.center);
  double interior = sqrt( kappa * kappa + other.kappa * other.kappa + 2 * kappa * other.kappa * cosine);
  return leadingCoefficient * bessel(interior / 2);
}


void MoVMF::addComponent(double weight, VMF distribution){
  distributions.push_back(std::pair<double, VMF>(weight, distribution));
}

double MoVMF::compare(MoVMF& other){
  double similarity = 0;
  for( int i = 0; i < distributions.size(); i++){
    for (int j = 0; j < other.distributions.size(); j++){
      double weight = distributions[i].first * other.distributions[j].first;
      double sim = distributions[i].second.compare(other.distributions[j].second);
      if (sim > 1.0){
        sim = 1.0;
      }
      //printf("Pair %d, %d has weight %f and similarity %f\n", i, j, weight, sim);
      similarity += weight * sim;
    }
  }
  return similarity;
}


std::unordered_map<int, MoVMF> readMoVMFs(std::string fileName){
  std::unordered_map<int, MoVMF> map;
  
  std::fstream file;
  file.open(fileName, std::ios::in);
  std::string line;

  while(std::getline(file, line)){
    std::istringstream iss(line);
    int id;
    float w, k, x, y, z;
    iss >> id;
    MoVMF m;
    while (iss >> w >> k >> x >> y >> z){
      m.addComponent(w, VMF(k, glm::vec3(x, y, z)));
    }
    map.insert(std::make_pair(id, m));

  }
  return map;
}

MoVMF combineMoVMFs(MoVMF& a, double  aw, MoVMF& b, double bw){
  MoVMF r;
  for (int i = 0; i < a.distributions.size(); i++){
    r.addComponent(a.distributions[i].first * (aw / (aw + bw)), a.distributions[i].second);
  }
  for (int i = 0; i < b.distributions.size(); i++){
    r.addComponent(b.distributions[i].first * (bw / (aw + bw)), b.distributions[i].second);
  }
  return r;
}


//Returns the factorial squared
double fac2(int k){
  double f = 1;
  for(int i = k; i > 0; i--){
    f *= i;
  }
  return f * f;
}

//Returns modified bessel function of the first order 0th or something (I_0(x))
double bessel(double x){
  double sum = 0;
  double num, denom;
  for(int k = 0; k < 10; k++){//5 is about infinity
    num = pow(4, -k) * pow(x, 2 * k);
    denom = fac2(k);
    sum += num/denom;
  }
  return sum;
}

void test(double x){
  printf("I_0(%f) = %f\n", x, bessel(x));
}

int mains(){
  printf("Starting program\n.");
  test(0);
  test(-1);
  test(1);
  test(2);
  test(10);
  test(5);
  VMF a (1, glm::vec3(1, 0, 0));
  VMF b (10, glm::vec3(0, 1, 0));
  VMF c (5, glm::vec3(0, 0, 1));
  
  printf("a & a %f\n", a.compare(a)); 
  printf("a & b %f\n", a.compare(b)); 
  printf("b & a %f\n", b.compare(a)); 
  printf("b & b %f\n", b.compare(b)); 

  MoVMF d,e,f;
  d.addComponent(1, a);
  e.addComponent(0.3, a);
  e.addComponent(0.5, b);
  e.addComponent(0.2, c);
  f.addComponent(0.3, b);
  f.addComponent(0.7, a);

  printf("d & d %f\n", d.compare(d));
  printf("d & e %f\n", d.compare(e));
  printf("d & f %f\n", d.compare(f));

  printf("e & d %f\n", e.compare(d));
  printf("e & e %f\n", e.compare(e));
  printf("e & f %f\n", e.compare(f));

  printf("f & d %f\n", f.compare(d));
  printf("f & e %f\n", f.compare(e));
  printf("f & f %f\n", f.compare(f));

  auto m = readMoVMFs("test.movms");
  printf("Map size: %d\n", m.size());
  printf("2038091 &3839055 : %f\n", m[2038091].compare(m[3839055]));

  return 0;
}
