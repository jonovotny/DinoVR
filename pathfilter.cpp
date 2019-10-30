#include "pathfilter.h"
#include <fstream>


void PathFilter::run(std::string pointFile, std::string outFile, int numPaths, double gap, double requiredLength)
{
  printf("Running pathfilter with %d paths, %f gap, %f length.\n", numPaths, gap, requiredLength);
  pm.ReadFile(pointFile, true);

  //runFilter();
  filterPoisson(numPaths, gap, requiredLength);
  std::ofstream of;
  of.open(outFile);

  for(int i = 0; i < paths.size(); i++){
    int id = paths[i];
    of << id << std::endl;
  }
  of.close();


}


void PathFilter::runFilter()
{

  for (auto &point : pm.points){
    if ( (rand() % 5) == 0){
      paths.push_back(point.m_id);
    }

  }

}

bool checkDistanceTotal(VRPoint& p, VRPoint& q, double gap){
  return p.withinDistance(q, gap);
}


bool checkDistanceTime(VRPoint& p, VRPoint& q, double gap){
  int length = std::min(p.positions.size(), q.positions.size());
  for(int i = 0; i < length; i++){
    if (glm::distance(p.positions[i],  q.positions[i]) < gap){
      return true;
    }
  }
  return false;
}

//Checks if two paths are within a given distance of each other
bool checkDistance(VRPoint& p, VRPoint& q, double gap){
  return checkDistanceTime(p, q, gap);
}

void PathFilter::filterPoisson(int number, double gap, double requiredLength)
{
  std::vector<int> pathIDs;
  std::vector<int> options;
  for(int i = 0; i < pm.points.size(); i++){
    if (pm.points[i].totalPathLength() > requiredLength){
      options.push_back(i);
    }
  }

  printf("%d paths found of suitable length.\n", options.size());

  for (int i = 0; i < number; i++){
    int oid;
    int id;
    bool valid;
    int tries = 0;
    do{
      if (options.size() == 0){
        break;
      }
      tries++;
      oid = rand() % options.size();
      id = options[oid];
      options.erase( options.begin() + oid);
      valid = true;
      for(auto &other : pathIDs){
        if (pm.points[id].withinDistance(pm.points[other], gap)){
          valid = false;
          continue;
        }
      }
    }
    while(!valid);
    if (options.size() == 0){
      break;
    }
    //Repeat the cycle above of trying new points until we find one that works.
    pathIDs.push_back(id);
    printf("Adding path %4d took %3d tries with %d options left..\n", i,  tries, options.size());
    
  }

  for(int i = 0; i < pathIDs.size(); i++){
    paths.push_back(pm.points[pathIDs[i]].m_id);
  }
}


int main(int argc, char **argv){

  std::string dataFile = "data/slices-68.out";
  std::string outFile = "test.paths";
  int numPaths = 200;
  double gap = .005;
  double cutoff = .003;
  PathFilter pf;
  
  if (argc >= 2){
    dataFile = argv[1];
  }
  if (argc >= 3){
    outFile = argv[2];
  }
  if (argc >= 4){
    numPaths = atoi(argv[3]);
  }
  if (argc >= 5){
    gap = atof(argv[4]);
  }
  if (argc >= 6){
    cutoff = atof(argv[5]);
  }

  pf.run(dataFile, outFile, numPaths, gap, cutoff);
  return 0;
}
