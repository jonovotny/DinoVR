#include "pointmanager.h"
#include "vrpoint.h"
#include "SimilarityEvaluator.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <functional>

#ifdef PROFILING
#include "gperftools/profiler.h"
#endif

#include "fast_atof.c"

PointManager::PointManager()
{

}

void PointManager::ReadFile(std::string fileName, bool debug)
{
    
    if (debug){
      std::cout << "Reading file: " << fileName << std::endl;
    }

    clock_t startTime = clock();
    std::ios_base::sync_with_stdio(false);
    std::fstream file;
    std::string line;
    file.open(fileName, std::ios::in);
    int linecount = 0;
    //VERY VERY TEMPORARY

     
    while(std::getline(file, line))
    {
        linecount++;
        //if(debug){
          if (linecount % 10000 == 0){
            printf("Currently reading line %d.\r\n", linecount); 
            //ProfilerFlush();
          }
       // }
        std::istringstream iss(line);
        int id;
        float x,y,z;
        if (iss.peek() == 'B'){
          std::string c;
          iss >> c;
          //Reading in a box coords
          iss >> x >> y >> z;
          glm::vec3 lower(x,y,z);
          iss >> x >> y >> z;
          glm::vec3 upper(x,y,z);
          // REDO THE BOXES
          //AABox box (lower, upper);//think about readding checks for lower/upper
          //printf("Read box %f,%f,%f:%f,%f,%f\n", lower.x, lower.y, lower.z, upper.x, upper.y, upper.z );
          //boxes.push_back(box);
          continue;
        }
        iss >> id;
        VRPoint p (id);
        /*
        while (iss >> x >> y >> z){
            p.AddPoint(glm::vec3(x,y,z));
        }
*/
      
        iss.get();//Clear the leading space left over from grabbing the first one
        char token[20];
        float arr[3];
        int idx = 0;
        while(!(iss.eof())){
          iss.getline(token, 20, ' ');
          arr[idx] = atof(token);
          idx++;
          if(idx == 3){
            idx = 0;
            p.AddPoint(glm::vec3(arr[0], arr[1], arr[2]));
          }
        }
        points.push_back(p);
        
    }
    printf("Time after reading points: %f\n", ((float)(clock() - startTime)) / CLOCKS_PER_SEC);


    timeSteps = 0;
    minV = points[0].positions[0];
    maxV = minV;
    for(int i = 0; i < points.size(); i++){
      //timeSteps = max(timeSteps, points[i].steps());  
      if (points[i].steps() > timeSteps){
        timeSteps = points[i].steps();
      }
      minV = glm::min(minV, points[i].positions[0]);
      maxV = glm::max(maxV, points[i].positions[0]);
    }
    


}

void PointManager::SetupDraw(bool allPaths){
    clock_t startTime = clock();
  


    filter = new Filter();
   
    if (useSeparateBuffers && timeSteps > 0){
      pointBuffers = new GLuint[timeSteps];
      glGenBuffers(timeSteps, pointBuffers);
    }
   
   
    retestVisible();
  

//Why does it crash on CCV systems if there's a vao? I haven't a clue. But it does.
#if defined(__APPLE__)
    GLuint vao;
    GLuint tempVar;
    glGenVertexArrays(1, &tempVar);
    glBindVertexArray(tempVar);
#endif

    glGenBuffers(1, &buffer);
    glGenBuffers(1, &pathBuffer);
    glGenBuffers(1, &tempPathBuffer);
    glGenBuffers(1, &megaClusterBuffer);
    glGenBuffers(1, &clusterBuffer);
    glGenBuffers(1, &particleSimilarityBuffer);
    
    SetShaders();

    if (allPaths){
      for(int i = 0; i < points.size(); i++){
 
        int offset = pathVertices.size();
        pathOffsets.push_back(offset);
        std::vector<Vertex> newVerts = points[i].getPathlineVerts();
        pathCounts.push_back(newVerts.size());
        pathVertices.insert(pathVertices.end(), newVerts.begin(), newVerts.end());

      }
    }
    DoClusterBuffers();
   printf("Time after arranging points: %f\n", ((float)(clock() - startTime)) / CLOCKS_PER_SEC);
}

void PointManager::SetShaders(){
    pointShader = new MyShader("shaders/basic.vert", "shaders/basic.geom", "shaders/basic.frag");
    pointShader->checkErrors();
    pointShader->loadTexture("colorMap", "colormap.jpg");
    pointShader->loadTexture("clusterMap", "clusters.png");
    lineShader = new MyShader("shaders/litpath.vert",  "shaders/litpath.frag");
    lineShader->checkErrors();
    lineShader->loadTexture("pathMap", "pathmap.jpg");

}

void PointManager::ReadPathlines(std::string fileName){
  std::fstream file;
  file.open(fileName, std::ios::in);
  int id;
  while (file >> id)
  {
    for(int i = 0; i < points.size(); i++){
      if (points[i].m_id == id){
        AddPathline(points[i]);
        break;
      }
    }
  }

} 

int PointManager::getLength(){
  return timeSteps;
}

void PointManager::computeLocations(){
  pointLocations.clear();
  for(int i = 0; i < timeSteps; i++){
    std::vector<Vertex> pointArray;
    for (int j = 0; j < visiblePoints.size(); j++){
      //VRPoint point = points[visiblePoints[j]];
      pointArray.push_back(points[visiblePoints[j]].Draw(i, minV, maxV));
    }
    pointLocations.push_back(pointArray);
  }

  if(useSeparateBuffers){
    for(int i = 0; i < timeSteps; i++){
      glBindBuffer(GL_ARRAY_BUFFER, pointBuffers[i]);
      std::vector<Vertex> *pointArray = &pointLocations[i];
      int bufferSize = pointArray->size() * sizeof(Vertex);
      Vertex* bufferData = pointArray->data();
      glBufferData(GL_ARRAY_BUFFER, bufferSize, bufferData, GL_STATIC_DRAW);

    }
  }
}

//Returns the index of the closest point
int PointManager::FindPathline(glm::vec3 pos, int time, float minDist){
  int bestID = 0;
  float d;
  //printf("Finding nearest pathline to %f, %f, %f\n", pos.x, pos.y, pos.z);
  for(int i = 0; i < points.size(); i++){
    d = points[i].GetDistance(time, pos);
    if (d < minDist){
      minDist = d;
      bestID = i;
    }
  }
  return bestID;
} 

void PointManager::TempPathline(glm::vec3 pos, int time){
  int bestID = FindPathline(pos, time);
  VRPoint& point = points[bestID];
  tempPath = point.getPathlineVerts();
}

void PointManager::AddPathline(glm::vec3 pos, int time){
  int bestID = FindPathline(pos, time); 
  AddPathline(points[bestID]);
  printf("Best pathline was for point %d near to %f, %f, %f\n", bestID, pos.x, pos.y, pos.z);
  //std::cout << std::endl;
}

void PointManager::AddPathline(VRPoint& point, float fixedY){
  int offset = pathVertices.size();
  pathOffsets.push_back(offset);
  std::vector<Vertex> newVerts;
  if (fixedY > -0.5) {
    newVerts = point.getPathlineVerts(true, fixedY);
  }
  else{
    newVerts = point.getPathlineVerts();
  }
  pathCounts.push_back(newVerts.size());
  pathVertices.insert(pathVertices.end(), newVerts.begin(), newVerts.end());
  updatePaths = true;
}

void PointManager::ClearPathlines(){
  pathOffsets.clear();
  pathCounts.clear();
  pathVertices.clear();
  updatePaths = true;
  similarityReset = true;
}

void PointManager::ReadClusters(std::string fileName){
  clusters.clear();
  std::fstream file;
  file.open(fileName, std::ios::in);
  std::string line;
  while(std::getline(file, line)){
    std::istringstream iss(line);
    std::vector<int> pointIDs;
    int id;
    //Read the points into a vector of ids
    while (iss >> id){
      pointIDs.push_back(id);
    }
    //Process the vector of IDs to get back indices
    //Could do this separately, but 
    std::vector<int> indices;
    for (auto id : pointIDs){
      int idx = FindPointIndex(id);
      if (idx == 0){
        std::cout << "double :(" << std::endl;
      }
      if (idx != -1){
        indices.push_back(idx);
      }
    }
    clusters.push_back(indices);
    std::cout << "Read a cluster of size " << indices.size() << std::endl;
  }
  std::cout << "Read " << clusters.size() << " clusters total." << std::endl;
  for (int i = 0; i < clusters.size(); i++){
    for (int j = 0; j < clusters[i].size(); j++){
      if (clusters[i][j] == 0){
        std::cout << ":(" << std::endl;
      }
    }
  }
}

void PointManager::ShowCluster(glm::vec3 pos, int time){
  std::cout << "Searching for a cluster." << std::endl;
  int closestPoint = FindPathline(pos, time);
  std::cout << "Best point found was " << closestPoint << std::endl;
  int foundCluster = -1;
  for (int i = 0; i < clusters.size(); i++){
    for (int j = 0; j < clusters[i].size(); j++){
      if (clusters[i][j] == closestPoint){
        foundCluster = i;
      }
    }
  }
  if (foundCluster != -1){
    currentCluster = foundCluster;
    clustersChanged = true;
    std::cout << "Found a point in cluster! " << currentCluster << std::endl;
  }
  
}

void PointManager::DoClusterBuffers(){
  std::vector<int> clusterIDs (points.size(), -1);
  for(int cluster = 0; cluster < clusters.size(); cluster++){
    for (int j = 0; j < clusters[cluster].size(); j++){
      clusterIDs[clusters[cluster][j]] = cluster;
    }
  }
  
  glGenBuffers(1, &particleClusterBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, particleClusterBuffer);
  glBufferData(GL_ARRAY_BUFFER, clusterIDs.size() * sizeof(int), clusterIDs.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

}

void PointManager::ReadMoVMFs(std::string fileName){
  movmfs = readMoVMFs(fileName);
}

void PointManager::ResetPrediction(){
  seeds = MoVMF();
  seedCount = 0;
  seedsChanged = true;
  seedSearches = 0;
}

void PointManager::AddSeedPoint(glm::vec3 pos, int time){
  int bestIdx = FindPathline(pos, time);
  int bestID = points[bestIdx].m_id;
  auto it = movmfs.find(bestID);
  if (it == movmfs.end()){
    std::cout << "Failed to add point" << std::endl;
    return;//fail silently because eh
  }
  seeds = combineMoVMFs(seeds, seedCount, it->second, 1);
  std::cout << "Added point, now have length" << seeds.distributions.size() << std::endl;
  AddPathline(points[bestIdx]);
  seedsChanged = true;
}


void PointManager::SearchForSeeds(int target_count){
  std::cout << "Beginning scoring!" << std::endl;
  std::unordered_map<int, double> results;
  std::vector<int> wanted;
  //std::vector<std::pair<double, int>> scores;
  if (seedsChanged){
    scores.clear();
    for(int i = 0; i < points.size(); i++){
      int id = points[i].m_id;
      double score = seeds.compare(movmfs[id]);
      printf("Point ID %d with score %f\n", i, score);
      results.insert(std::make_pair(i, score));
      scores.push_back(std::make_pair(1-score, i));
      if (score > 0.5){
        //wanted.push_back(i);
        //AddPathline(points[i]);
      }
    }
  }
  for (auto it : wanted){
    printf("Looking for point id %d. Found at index %d.\n", it, FindPointIndex(it));
    //AddPathline(points[FindPointIndex(it)]);
  }
  int numParticles = target_count * (seedSearches + 1);
  std::nth_element(scores.begin(), scores.begin() + numParticles, scores.end());
  for(int i = 0; i < numParticles ; i++){
    printf("Element %d with score %f.\n", scores[i].second, 1-scores[i].first);
    AddPathline(points[scores[i].second]);
  }
  
  std::cout << std::endl;
  seedsChanged = false;
  seedSearches++;
}

void PointManager::FindClosestPoints(glm::vec3 pos, int t, int numPoints){
  clock_t startTime = clock();
  int bestIdx = FindPathline(pos, t);
  similaritySeedPoint = bestIdx;
  pathSimilarities = simEval->getAllPathSimilarities(this, bestIdx);//Keep this around and find a way to recognize it to interactively increase # paths shown.
  std::vector<float> sims(points.size(), -1);
  similarities = sims;
  //I'm sure I could find a better way to do this...
  maxSimilarityDistance = 0;
  for (auto pair : pathSimilarities){
    similarities[pair.first] = pair.second;
    maxSimilarityDistance = std::max(maxSimilarityDistance, pair.second);
  }
  printf("Farthest Point: %f.\n", maxSimilarityDistance);
  
  glBindBuffer(GL_ARRAY_BUFFER, particleSimilarityBuffer);
  int bufferSize = similarities.size() * sizeof(float);
  glBufferData(GL_ARRAY_BUFFER, bufferSize, similarities.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  


  std::vector<int> bestPaths = simEval->getMostSimilarPathsFromSimilarities(pathSimilarities, numPoints);
  for (auto idx : bestPaths){
    AddPathline(points[idx], similarities[idx]);
  }
  AddPathline(points[bestIdx], 0.0);

  midSimilarityDistance = similarities[bestPaths[bestPaths.size() - 1]];
  std::cout << "Edge of cluster:" << midSimilarityDistance << std::endl;;

  std::cout << "Time to find similarities: " << ((float)(clock() - startTime) / CLOCKS_PER_SEC) << " seconds. " << std::endl;
  colorBySimilarity = true;
  colorPathsBySimilarity = true;
  similarityReset = false;
}

void PointManager::ExpandClosestPoints(int numPoints){
  if (similarityReset){
    printf("BREAKING EARLY FROM EXPAND CLOSEST POINTS BECAUSE NO POINTS TO EXPAND.\n");
    return;
  }
  clock_t startTime = clock();
  ClearPathlines();
  std::vector<int> bestPaths = simEval->getMostSimilarPathsFromSimilarities(pathSimilarities, numPoints);
  for (auto idx : bestPaths){
    AddPathline(points[idx], similarities[idx]);
  }
  AddPathline(points[similaritySeedPoint], 0.0);
  midSimilarityDistance = similarities[bestPaths[bestPaths.size() - 1]];
  std::cout << "Edge of cluster: " << midSimilarityDistance<< std::endl;

  std::cout << "Time to find similarities: " << ((float)(clock() - startTime) / CLOCKS_PER_SEC) << " seconds. " << std::endl;
  colorBySimilarity = true;
  colorPathsBySimilarity = true;

  similarityReset = false;
}

int PointManager::FindPointIndex(int pointID){
  for(int i = 0; i < points.size(); i++){
    if(points[i].m_id == pointID){
      return i;
    }
  }
  return -1;
}


void PointManager::SetFilter(Filter* f){
  filter = f;
  retestVisible();
}

void PointManager::retestVisible(){
  visiblePoints.clear();
  for(int i = 0; i < points.size(); i++){
    if (filter->checkPoint(points[i])){
      visiblePoints.push_back(i);
    }
  }
  computeLocations();
}


void PointManager::DrawBoxes(){
  return;
/*  for(int i = 0; i < boxes.size(); i++){
    //AABox box = boxes[i];
    float gray = 0.4;
    //Draw::box(box, rd, Color4::clear(), Color4(gray, gray, gray, 1.));
  }*/
}

void PointManager::DrawPoints(int time, glm::mat4 mvp){
    int err;
    glCheckError();
    //Bind shader and set args
    pointShader->bindShader();
    glCheckError();
    pointShader->setMatrix4("mvp", mvp);
    pointShader->setFloat("rad", pointSize);
    glCheckError();
    //Bind and resend buffer data
    int numElements;
    if (useSeparateBuffers){
      if (time < timeSteps){
        glBindBuffer(GL_ARRAY_BUFFER, pointBuffers[time]);
        numElements = pointLocations[time].size();
      }
      else{
        return;
      }
    }
    else{
      glBindBuffer(GL_ARRAY_BUFFER, buffer);
      std::vector<Vertex> *pointArray = &pointLocations[time];
      numElements = pointArray->size();
      int bufferSize = pointArray->size() * sizeof(Vertex);
      Vertex* bufferData = pointArray->data();
      glBufferData(GL_ARRAY_BUFFER, bufferSize, bufferData, GL_DYNAMIC_DRAW);
    }

   glCheckError();

    //set vertex attributes
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glCheckError();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,  sizeof(Vertex), NULL);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) sizeof(glm::vec3));
 
 
    if (colorByCluster){
      glBindBuffer(GL_ARRAY_BUFFER, particleClusterBuffer);
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 1, GL_INT, GL_FALSE, 0, 0);
      pointShader->setFloat("numClusters", clusters.size());
      glBindBuffer(GL_ARRAY_BUFFER, 0);

    }
    else{
      pointShader->setFloat("numClusters", 0);
    }
    if (colorBySimilarity){
      glBindBuffer(GL_ARRAY_BUFFER, particleSimilarityBuffer);
      glEnableVertexAttribArray(3);
      glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, 0);
      pointShader->setFloat("maxDistance", maxSimilarityDistance);
      pointShader->setFloat("clusterDistance", midSimilarityDistance);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    else{
      pointShader->setFloat("maxDistance", -1.0);
      pointShader->setFloat("clusterDistance", -1.0);
    }

    
    glCheckError();

    //Draw the points
    glDrawArrays(GL_POINTS, 0, numElements);
    glCheckError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //Unbind shader
    pointShader->unbindShader();
}

 
void PointManager::DrawPaths(int time, glm::mat4 mvp){
    int err;
    glCheckError();
    //Bind shader and set args
    lineShader->bindShader();
    lineShader->setMatrix4("mvp", mvp);
    lineShader->setFloat("minCutoff", pathlineMin);
    lineShader->setFloat("maxCutoff", pathlineMax);
    float t = time * 1.0 / timeSteps;
    //printf("%f\n", t);
    lineShader->setFloat("time", t);
    glCheckError();

    //Bind buffer, resend data if needed
    glBindBuffer(GL_ARRAY_BUFFER, pathBuffer);
    //updatePaths ensures that we only write new paths to the gpu when needed
    int bufferSize = pathVertices.size() * sizeof(Vertex);
    if (updatePaths){
      updatePaths = false;
      Vertex* bufferData = pathVertices.data();
      glBufferData(GL_ARRAY_BUFFER, bufferSize, bufferData, GL_DYNAMIC_DRAW);
    }
    glCheckError();

    //set vertex attributes
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,  sizeof(Vertex), NULL);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) sizeof(glm::vec3));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (sizeof(glm::vec3) + sizeof(glm::vec2)));
    glCheckError();
    
    
    if (colorPathsBySimilarity){
     /* glBindBuffer(GL_ARRAY_BUFFER, particleSimilarityBuffer);
      glEnableVertexAttribArray(3);
      glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, 0);*/
      lineShader->setFloat("maxDistance", maxSimilarityDistance);
      lineShader->setFloat("clusterDistance", midSimilarityDistance);
      //glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    else{
      lineShader->setFloat("maxDistance", -1.0);
      lineShader->setFloat("clusterDistance", -1.0);
    }

    //Draw points
    glDrawArrays(GL_TRIANGLES,  0, bufferSize);
    glCheckError();



    //Draw temporary paths
    if (tempPath.size() > 0){
      glBindBuffer(GL_ARRAY_BUFFER, tempPathBuffer);
      int bufferSize = tempPath.size() * sizeof(Vertex);
      Vertex* bufferData = tempPath.data();
      glBufferData(GL_ARRAY_BUFFER, bufferSize, bufferData, GL_DYNAMIC_DRAW);
      
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,  sizeof(Vertex), NULL);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) sizeof(glm::vec3));
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (sizeof(glm::vec3) + sizeof(glm::vec2)));


      glDrawArrays(GL_TRIANGLES, 0, tempPath.size());
    }
    glCheckError();
    //unbind shader
    lineShader->unbindShader();
}

void PointManager::DrawClusters(int time, glm::mat4 mvp){
    if (!clustering || currentCluster == -1){
      return;
    }

    glCheckError();
    //Bind shader and set args
    lineShader->bindShader();
    lineShader->setMatrix4("mvp", mvp);
    lineShader->setFloat("minCutoff", pathlineMin);
    lineShader->setFloat("maxCutoff", pathlineMax);
    glCheckError();

    //Bind buffer, resend data if needed
    glBindBuffer(GL_ARRAY_BUFFER, clusterBuffer);
    int bufferSize = clusterVertCount * sizeof(Vertex);
    //updatePaths ensures that we only write new paths to the gpu when needed
    if (clustersChanged){
      clustersChanged = false;
      std::vector<Vertex> clusterVerts;
      for(int i = 0; i < clusters[currentCluster].size(); i++){
        std::vector<Vertex> newVerts = points[clusters[currentCluster][i]].getPathlineVerts();
        clusterVerts.insert(clusterVerts.end(), newVerts.begin(), newVerts.end());
      }
      Vertex* bufferData = clusterVerts.data();
      clusterVertCount = clusterVerts.size();
      bufferSize = clusterVertCount * sizeof(Vertex);
      glBufferData(GL_ARRAY_BUFFER, bufferSize, bufferData, GL_DYNAMIC_DRAW);
    }
    glCheckError();

    //set vertex attributes
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,  sizeof(Vertex), NULL);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) sizeof(glm::vec3));
    glCheckError();
    //Draw points
    glDrawArrays(GL_TRIANGLES,  0, bufferSize);
    glCheckError();
}

void PointManager::DrawAllClusters(int time, glm::mat4 mvp){
    printf("DRAWING THEM ALL!!!!!!!");
    glCheckError();
    //Bind shader and set args
    lineShader->bindShader();
    lineShader->setMatrix4("mvp", mvp);
    lineShader->setFloat("minCutoff", pathlineMin);
    lineShader->setFloat("maxCutoff", pathlineMax);
   
    int bufferSize;
    if (firstTimeDrawingAll){
      
      megaClusterMegaBuffers = new GLuint[clusters.size()];
      glGenBuffers(timeSteps, megaClusterMegaBuffers);

      glGenBuffers(1, &megaClustersBuffer);
      
      std::vector<Vertex> verts;

      for (int cluster = 0; cluster < clusters.size(); cluster++){
        
        float clusterDist = cluster * 1.0 / clusters.size();
        for (int i = 0; i < clusters[cluster].size(); i++){
          std::vector<Vertex> newVerts = points[clusters[cluster][i]].getPathlineVerts(true, clusterDist);
          verts.insert(verts.end(), newVerts.begin(), newVerts.end());
        }
      }
      Vertex* bufferData = verts.data();
      bufferVertexCount = verts.size();
      bufferSize = bufferVertexCount * sizeof(Vertex);
      
      glBindBuffer(GL_ARRAY_BUFFER, megaClustersBuffer);
      glBufferData(GL_ARRAY_BUFFER, bufferSize, bufferData, GL_STATIC_DRAW);
      glCheckError();

    }
    bufferSize = bufferVertexCount * sizeof(Vertex);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,  sizeof(Vertex), NULL);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) sizeof(glm::vec3));
    glCheckError();
    //Draw points
    glDrawArrays(GL_TRIANGLES,  0, bufferSize);
    glCheckError();

/*    glCheckError();
    for (int cluster = 0; cluster < clusters.size(); cluster++){
      //Bind buffer, resend data if needed
      lineShader->setFloat("clusterId", cluster * 1.0 / clusters.size());
      glBindBuffer(GL_ARRAY_BUFFER, megaClusterMegaBuffers[cluster]);

      int bufferSize = 0;
      if (firstTimeDrawingAll){
        std::vector<Vertex> clusterVerts;
        for(int i = 0; i < clusters[cluster].size(); i++){
          std::vector<Vertex> newVerts = points[clusters[cluster][i]].getPathlineVerts();
          clusterVerts.insert(clusterVerts.end(), newVerts.begin(), newVerts.end());
        }
        Vertex* bufferData = clusterVerts.data();
        stupidVertexCount.push_back( clusterVerts.size());
        bufferSize = clusterVerts.size() * sizeof(Vertex);
        glBufferData(GL_ARRAY_BUFFER, bufferSize, bufferData, GL_DYNAMIC_DRAW);
        glCheckError();
      }
      bufferSize = stupidVertexCount[cluster] * sizeof(Vertex);
      //set vertex attributes
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,  sizeof(Vertex), NULL);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) sizeof(glm::vec3));
      glCheckError();
      //Draw points
      glDrawArrays(GL_TRIANGLES,  0, bufferSize);
      glCheckError();
    }
*/
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void PointManager::Draw(int time, glm::mat4 mvp){
    numFramesSeen++;
    clock_t startTime = clock();
    int err;
    glCheckError();
    DrawBoxes();
    //printf("Time after setting points: %f\n", ((float)(clock() - startTime)) / CLOCKS_PER_SEC);
    glCheckError();
    DrawPoints(time, mvp);
    glCheckError();
    DrawPaths(time, mvp);
    glCheckError();
    DrawClusters(time, mvp);
    glCheckError();
    if (drawAllClusters){
      DrawAllClusters(time, mvp);
    }

    //printf("Time end of frame: %f\n", ((float)(clock() - startTime)) / CLOCKS_PER_SEC);
   }
