#include "footmeshviewer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>


void FootMeshViewer::ReadFiles(std::string fileName){
  printf("READING FILES FOR FMV\n");
  std::fstream file;
  file.open(fileName, std::ios::in);
  std::string prefix;
  std::string line;
  std::vector<std::string> fileNames;

  std::getline(file, prefix);
  printf("prefix is %s\n", prefix.c_str());
  printf("[");



  while (std::getline(file, line)){
    fileNames.push_back(prefix + line); 
  }
  
  int timeSteps = fileNames.size();
  if (timeSteps == 0){
    active = false;
    return;
  }
  GLuint* vbos = new GLuint[timeSteps];
  glGenBuffers(timeSteps, vbos);
  bufferIds.assign(vbos, vbos+timeSteps);

  std::vector<glm::vec3> lines;
  
  
  bool first = true;
  for (int i = 0; i < fileNames.size(); ++i){
    //Get a list of vertices to send
    const std::vector<glm::vec3>& tris = ReadSingleFile(fileNames[i]);
    //Bind the correct buffer
    glBindBuffer(GL_ARRAY_BUFFER, bufferIds[i]);
    int numElems = tris.size();
    int bufferSize = numElems * sizeof(glm::vec3);
    numElements.push_back(numElems);
    //Send the data to the buffer
    glBufferData(GL_ARRAY_BUFFER, bufferSize, tris.data(), GL_STATIC_DRAW);
    glCheckError();
    
    //Unbind or else opengl gets mad
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    if(first) {
      lines = std::vector<glm::vec3>(numElems*timeSteps,glm::vec3(0));
      first=false;
    }

    
    for (int j = 0; j < tris.size(); j++) {
      lines[(j*timeSteps)+i] = tris[j];
    }glEnableVertexAttribArray(0);
    
  }

  printf("]");

  glGenBuffers(1, &bufferWhole);
  glBindBuffer(GL_ARRAY_BUFFER, bufferWhole);
  glBufferData(GL_ARRAY_BUFFER, lines.size()*sizeof(glm::vec3), lines.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  //printf("finished model load");
  //Initialize the shader
  shader = std::make_unique<MyShader>("shaders/foot.vert", "shaders/foot.geom",  "shaders/foot.frag");
  //shader = std::make_unique<MyShader>("shaders/foot.vert",  "shaders/foot.frag");
  shader->checkErrors();
  
  lineShader = std::make_unique<MyShader>("shaders/litpath.vert",  "shaders/litpath.frag");
  lineShader->checkErrors();
  lineShader->loadTexture("pathMap", "pathmap.jpg");
  lineShader->checkErrors();
  
  sweepShader =  std::make_unique<MyShader>("shaders/sweep.vert", "shaders/sweep.geom", "shaders/sweep.frag");
  sweepShader->checkErrors();

  const std::vector<glm::vec3>& objTris = ReadSingleObjFile("footmodels/decimated.obj");
  glGenBuffers(1, &meta);
  
  glBindBuffer(GL_ARRAY_BUFFER, meta);
  glBufferData(GL_ARRAY_BUFFER, objTris.size()*sizeof(glm::vec3), objTris.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  metaNumElems = objTris.size();

  //std::vector<glm::vec3> p1 = ReadSingleJointFile("footmodels/dip3.csv");
  //std::vector<glm::vec3> p2 = ReadSingleJointFile("footmodels/tip3.csv");

  std::vector<std::vector<glm::vec3>> trails = ReadJointFiles("footmodels/trails.csv");

  transforms = ReadSingleMatrixFile("footmodels/meta.csv");

  stainedIds = new GLuint[trails.size()];
  numStainedElems = trails.size();
  glGenBuffers(trails.size(), stainedIds);

  for (int j =0; j < trails.size()/2; j++) { 

  std::vector<glm::vec3> p1 = trails[j*2];
  std::vector<glm::vec3> p2 = trails[(j*2)+1];

  //glm::vec4 p1 = glm::vec4(-0.0011359, -0.0059762, -0.0022148, 1.0);
  //glm::vec4 p2 = glm::vec4(0.0780232, -0.0046106, -0.0003588, 1.0);
  //glm::vec4 temp;
  std::vector<glm::vec3> sweep;
  std::vector<glm::vec2> sweepTex;
  for (int i=0; i < p1.size(); i++) {
    //sweep.push_back(glm::vec3(transforms[i]*p1));
    //sweep.push_back(glm::vec3(transforms[i]*p2));
    sweep.push_back(p1[i]);
    sweep.push_back(p2[i]);
	sweepTex.push_back(glm::vec2(0,0.05*i/2));
	sweepTex.push_back(glm::vec2(1,0.05*i/2));
  }
  
  glBindBuffer(GL_ARRAY_BUFFER, stainedIds[j*2]);
  glBufferData(GL_ARRAY_BUFFER, sweep.size()*sizeof(glm::vec3), sweep.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  glBindBuffer(GL_ARRAY_BUFFER, stainedIds[(j*2)+1]);
  glBufferData(GL_ARRAY_BUFFER, sweepTex.size()*sizeof(glm::vec2), sweepTex.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  stainedNumElements.push_back(sweep.size());
}
}

#define SKIP(N) for (int iii = 0; iii < N; ++iii) { std::getline(file, line); }

std::vector<glm::vec3> FootMeshViewer::ReadSingleFile(std::string fileName){
	std::vector<glm::vec3> finalVertices;
	
	std::ifstream binfile;
  binfile.open(fileName + ".bin", std::ios::binary);


  if (binfile.is_open()) {
	  float x, y, z;
	  //std::cout << fileName + ".bin, " << "loading start" << std::endl;
	  while (binfile.read(reinterpret_cast<char*>(&x), sizeof(float))) {
		  binfile.read(reinterpret_cast<char*>(&y), sizeof(float));
		  binfile.read(reinterpret_cast<char*>(&z), sizeof(float));
		  finalVertices.push_back(glm::vec3(x,y,z));
	  }
	  binfile.close();
	  //std::cout << fileName + ".bin, " << "loading done" << std::endl;
	  printf("#");

	 /* if (fileName.compare("C:\\temp\\footmodels2\\footcomp2_648000.vtk") == 0) {
		  std::ofstream ot(fileName + ".obj", std::ofstream::out | std::ofstream::app);
		  for (const glm::vec3& v : finalVertices) {
			  ot << "v " << v.x << " " << v.y << " " << v.z << std::endl;
		  }
		  ot.close();

	  }*/

	  return finalVertices;
  } 

  std::fstream file;
  file.open(fileName, std::ios::in);
  
  std::string line;
  std::vector<glm::vec3> locations;
  std::vector<int> indices;

  SKIP(4);
  std::getline(file, line);//Should be "POINTS nnnnnn float"
  std::istringstream counter(line);
  std::string dummy;
  int numPoints;
  counter >> dummy >> numPoints;
  for (int i = 0; i < numPoints; ++i) {
	  std::getline(file, line);
	  float x, y, z;
	  char dum;
	  std::istringstream iss(line);
	  iss >> x >> y >> z;
	  locations.push_back(glm::vec3(x, y, z));
	  if (!iss.eof() && i < numPoints) {
		  iss >> x >> y >> z;
		  locations.push_back(glm::vec3(x, y, z));
		  ++i;
		  iss >> x >> y >> z;
		  locations.push_back(glm::vec3(x, y, z));
		  ++i;
	  }
  }
  
  SKIP(1);
  while (line.empty()) {
	  std::getline(file, line);
  }
  std::istringstream tricounter(line);
  int numTris;
  tricounter >> dummy >>  numTris;
  

  for (int i = 0; i < numTris; ++i){
    std::getline(file, line);
    int f,a,b,c;
    std::istringstream iss(line);
    iss >> f >> a >> b >> c;
    indices.push_back(a);
    indices.push_back(b);
    indices.push_back(c);
  }

  file.close();

  for (int i = 0; i < indices.size(); ++i){
    finalVertices.push_back(locations[indices[i]]);
  }
  std::cout << fileName + ".bin, " << finalVertices.size() << std::endl;
  std::ofstream of(fileName+".bin", std::ios::binary);
  for (const glm::vec3& v : finalVertices) {
	  of.write((char *)&v.x, sizeof(float));
	  of.write((char *)&v.y, sizeof(float));
	  of.write((char *)&v.z, sizeof(float));
  }
  of.close();


  return finalVertices;
}

std::vector<glm::vec3> FootMeshViewer::ReadSingleObjFile(std::string fileName){
  std::fstream file;
  file.open(fileName, std::ios::in);
  
  std::string line;
  std::vector<glm::vec3> locations;
  std::vector<int> indices;

  std::string type;
  while(!file.eof()) {
    type = "";
    std::getline(file, line);
   sweepShader->setInt("sweepMode", sweepMode);
    std::istringstream iss(line);
    iss >> type;
    if (type.compare("v") == 0) {
        float x,y,z;
	iss >> x >> y >> z;
	locations.push_back(glm::vec3(x/100.f,y/100.f,z/100.f));
	//std::cout << "v " << x/100.f << ", " << y/100.f << ", " << z/100.f << std::endl; 
    }
    if (type.compare("f") == 0) {
        int a,b,c;
	std::string sa, sb, sc;
	iss >> sa >> sb >> sc;
	std::string::size_type pos = sa.find('/');
        a = std::stoi(sa.substr(0, pos));
	pos = sb.find('/');
        b = std::stoi(sb.substr(0, pos));
	pos = sc.find('/');
        c = std::stoi(sc.substr(0, pos));
	indices.push_back(a-1);
    	indices.push_back(b-1);
    	indices.push_back(c-1);
        //std::cout << "f " << a << ", " << b << ", " << c << std::endl; 
    } 
  }
  std::vector<glm::vec3> finalVertices;
  for (int i = 0; i < indices.size(); ++i){
    finalVertices.push_back(locations[indices[i]]);
  }

  return finalVertices;
}

std::vector<std::vector<glm::vec3>> FootMeshViewer::ReadJointFiles(std::string fileName){

printf("READING FILES FOR FMV\n");
  std::fstream file;
  file.open(fileName, std::ios::in);
  std::string prefix;
  std::string line;
  std::vector<std::string> fileNames;
  std::vector<std::vector<glm::vec3>> trails;

  std::getline(file, prefix);

  while (std::getline(file, line)){
    fileNames.push_back(prefix + line); 
  }

  for (int i = 0; i < fileNames.size(); ++i){
    //Get a list of vertices to send
    trails.push_back(ReadSingleJointFile(fileNames[i]));
  }

  return trails;
}



std::vector<glm::vec3> FootMeshViewer::ReadSingleJointFile(std::string fileName){
  std::fstream file;
  file.open(fileName, std::ios::in);
  
  std::string line;
  std::vector<glm::vec3> transformations;
  std::vector<int> indices;

  SKIP(1);
  int frame;
  float vx, vy, vz;
  while(!file.eof()) {
    std::getline(file, line);
    std::istringstream iss(line);
    char comma;
    iss >> frame >> comma >> vx >> comma >> vy >> comma >> vz;
    transformations.push_back(glm::vec3(vx/100.f, vy/100.f, vz/100.f));
//	std::cout << "Frame " << frame << std::endl;
//        std::cout << vx/100.f << ", " << vy/100.f << ", " << vz/100.f << std::endl;
    }

  return transformations;
}


std::vector<glm::mat4> FootMeshViewer::ReadSingleMatrixFile(std::string fileName){
  std::fstream file;
  file.open(fileName, std::ios::in);
  
  std::string line;
  std::vector<glm::mat4> transformations;
  std::vector<int> indices;

  SKIP(1);
  int frame;
  float m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16;
  while(!file.eof()) {
    std::getline(file, line);
    std::istringstream iss(line);
    char comma;
    iss >> frame >> comma >> m1 >> comma >> m5 >> comma >> m9 >> comma >> m13 >> comma >> m2 >> comma >> m6 >> comma >> m10 >> comma >> m14 >> comma >> m3 >> comma >> m7 >> comma >> m11 >> comma >> m15 >> comma >> m4 >> comma >> m8 >> comma >> m12 >> comma >> m16;
    transformations.push_back(glm::mat4(m1, m5, m9, m13, m2, m6, m10, m14, m3, m7, m11, m15, m4/100.f, m8/100.f, m12/100.f, m16));
//transformations.push_back(glm::mat4(m1, m2, m3, m4/100.f, m5, m6, m7, m8/100.f, m9, m10, m11, m12/100.f, m13, m14, m15, m16));
//	std::cout << "Frame " << frame << std::endl;
//        std::cout << m1 << ", " << m2 << ", " << m3 << ", " << m4/100.f << std::endl;
//	std::cout << m5 << ", " << m6 << ", " << m7 << ", " << m8/100.f << std::endl; 
//	std::cout << m9 << ", " << m10 << ", " << m11 << ", " << m12/100.f << std::endl; 
//	std::cout << m13 << ", " << m14 << ", " << m15 << ", " << m16 << std::endl;  
    }

  return transformations;
}


void FootMeshViewer::Draw(int time, glm::mat4 mvp){
  if (!active){
    return;
  }
  if (time < 0 || time >= bufferIds.size()){
    return;
  }
  if (showOldModel) {
	  glCheckError();
	  shader->bindShader();
	  glCheckError();
	  shader->setMatrix4("mvp", mvp);
	  shader->setMatrix4("transform", glm::mat4());
          shader->setVector4("col", glm::vec4(1.0, 0.7, 0.7, 1.0));
	  glCheckError();

	  glBindBuffer(GL_ARRAY_BUFFER, bufferIds[time]);
	  glCheckError();

	  glEnableVertexAttribArray(0);
	  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	  glCheckError();

	  glDrawArrays(GL_TRIANGLES, 0, numElements[time]);
	  glCheckError();
	  glBindBuffer(GL_ARRAY_BUFFER, 0);
	  shader->unbindShader();
  }

  if (showNewModel) {
	  glCheckError();
	  shader->bindShader();
	  glBindBuffer(GL_ARRAY_BUFFER, meta);
	  glEnableVertexAttribArray(0);
	  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	  glCheckError();

	  shader->setMatrix4("mvp", mvp);
	  shader->setMatrix4("transform", glm::mat4());// transforms[time]);
          shader->setVector4("col", glm::vec4(1.0, 1.0, 1.0, 1.0));
	  glDrawArrays(GL_TRIANGLES, 0, metaNumElems);
	  glCheckError();

	  glBindBuffer(GL_ARRAY_BUFFER, 0);
	  shader->unbindShader();
	  glCheckError();
  }

  if (showCloud) {
	  lineShader->bindShader();
	  glCheckError();
	  lineShader->setMatrix4("mvp", mvp);
	  glCheckError();

	  glBindBuffer(GL_ARRAY_BUFFER, bufferWhole);
	  glCheckError();

	  glEnableVertexAttribArray(0);
	  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	  glCheckError();

	  for (int i = 0; i < numElements[0]; i+=1){
	  glDrawArrays(GL_POINTS, i*bufferIds.size(), bufferIds.size());
	  }
	  glCheckError();

	  glBindBuffer(GL_ARRAY_BUFFER, 0);
	  lineShader->unbindShader();
	  glCheckError();
  }

  if (showStained) {
	  sweepShader->bindShader();
	  glCheckError();
	  sweepShader->setMatrix4("mvp", mvp);
          sweepShader->setInt("sweepMode", sweepMode);
          sweepShader->setInt("frame", time);
	  glCheckError();

          for (int i = 0; i < numStainedElems/2; i++) {
    
	  glBindBuffer(GL_ARRAY_BUFFER, stainedIds[i*2]);
	  glCheckError();

	  glEnableVertexAttribArray(0);
	  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	  glCheckError();
	  
	  glBindBuffer(GL_ARRAY_BUFFER, stainedIds[(i*2)+1]);
	  glCheckError();

	  glEnableVertexAttribArray(1);
	  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	  glCheckError();

	  
	  glDrawArrays(GL_TRIANGLE_STRIP, 0, stainedNumElements[i]);
	  glCheckError();

	  glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	  sweepShader->unbindShader();
	  glCheckError();
	 
  }
}
