#ifndef POINTMANAGER_H
#define POINTMANAGER_H

#if defined(__APPLE__)
#include <GL/glew.h>
#include <OpenGL/gl.h>
#else
#define NOMINMAX
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif


#include "GLUtil.h"
#include <vector>
#include <string>
#include <map>
#include "vrpoint.h"
#include "vertex.h"
#include "filter.h"
#include "shader.h"
#include "glm/glm.hpp"
#include "movm.h"

class SimilarityEvaluator;


/**
 * PointManager manages most of the things dealing with the points, including loading and displaying them.
 * (Currently a little bit monolithic and not particularly necessary. Plan to take most drawing functionality out to separate classes in future.)
 */
class PointManager
{
public:
    PointManager();
    /** 
     * \brief Loads a file consisting of a number of points.
     * 
     * Format is each point on it's own line, with an id and then triple of XYZ
     * @param fileName Name of the file to be loaded
     * @param debug Optionally enable debug output.
     */
    void ReadFile(std::string fileName, bool debug = false);
	void ReadDistances(std::string fileName, bool debug = false);
    /*!
     * \brief Loads a list of VonMisFisher distributions for each point
     * 
     * @param fileName Name of the file to be loaded.
     */
    void ReadMoVMFs(std::string fileName);
    /*!
     * \brief Performs OpenGL initialization functionality.
     * 
     * Should be called after OpenGL instance is initialized.
     * For MinVR apps this happens during the first draw call.
     */
    void SetupDraw(bool allPaths = false);
    /*!
     * \brief Reads pathlines which should be displayed initially
     * 
     * In addition to interactively showing paths close to the viewer, we can show a set of them chosen beforehand.
     * This function lets us load a file and display all the pathlines contained in it.
     * The file should contain a list of particle IDs on separate lines.
     */
    void ReadPathlines(std::string fileName);



	/** 
     * Draws everything to be drawn on a given frame.
	 * @param time The frame to be drawn.
	 * @param mvp the Model-View-Projection matrix to use to draw.
	 */

    void Draw(int time, glm::mat4 mvp, glm::mat4 mv, glm::mat4 p, glm::vec4 cuttingPlane = glm::vec4(0.));

    /**
     * Initializes (or rereads) the shaders from disk.
     */
    void SetShaders();
    void AddPathline(glm::vec3 pos, int time);
    void AddPathline(VRPoint& point, float fixedY = -1.0);
    void ClearPathlines();
    int TempPathline(glm::vec3 pos, int time);
    void ReadSurface(std::string fileName);


    void SetFilter(Filter* f);
    float pointSize = 0.0004;
    int getLength();
    std::vector<VRPoint> points;
	std::vector<VRPoint> distances;
    bool showSurface = false;
    void AddSeedPoint(glm::vec3 pos, int time);
    void SearchForSeeds(int target_count = 10);
    void ResetPrediction();

    void FindClosestPoints(glm::vec3 pos, int t, int numPoints = 50);
	void FindClosestPoints(int bestIdx, int numPoints = 50);
	///< Finds the numPoints closest points to the selected one using simEval as the metric
    void ExpandClosestPoints(int numPoints); ///<Like FindClosestPoints, but works from the last seed point chosen and just changes the number shown. Also clears existing pathlines to make this doable. 
    std::vector<std::pair<int, double> > pathSimilarities;
    std::vector<float> similarities;
    SimilarityEvaluator* simEval;
    bool similarityReset = true;

    int similaritySeedPoint;
	std::vector<int> similarityPositiveSeeds;
	std::vector<int> similarityNegativeSeeds;


    bool clustering = true;
    int currentCluster = -1;
    bool clustersChanged = true;
    void ShowCluster(glm::vec3 pos, int time);
    void ReadClusters(std::string fileName);

    std::unordered_map<int, MoVMF> movmfs;
    MoVMF seeds;
    int seedCount;
    bool seedsChanged = true;
    int seedSearches = 0;
    std::vector<std::pair<double, int> > scores;

    std::vector< std::vector<int> > clusters;

    bool drawAllClusters = false;
    bool firstTimeDrawingAll = true;
    GLuint* megaClusterMegaBuffers;
    GLuint megaClustersBuffer;
    std::vector<int> stupidVertexCount;
    int bufferVertexCount = 0;

    float pathlineMin = 0.0;
    float pathlineMax = 1.0;
    bool colorByCluster = false;
    bool colorBySimilarity = false;
    bool colorPathsBySimilarity = false;
    bool cutOnOriginal = true; ///<Sets whether the slicing plane should work from starting frame positions (true) or current frame (false).

    bool surfTrail = false;
    float cellSize = 500;
    float cellLine = 0.0001;
    float ripThreshold = 4;
    int surfaceMode = 0;
    int layers = 0;
    int direction = 0;
    int freezeStep = 0;
	int splitStep = 0;
	int thickinterval = 5;
	float distFalloff = 0.05;
	int distMode = 0;

private:
    void DrawBoxes();
    void DrawPoints(int time, glm::mat4 mvp, glm::mat4 mv, glm::mat4 p, glm::vec4 cuttingPlane);
    void DrawPaths(int time, glm::mat4 mvp);
    void DrawClusters(int time, glm::mat4 mvp);
    void DrawAllClusters(int time, glm::mat4 mvp);
    void DoClusterBuffers();
    void DrawSurface(int time, glm::mat4 mvp);
    
  
    int FindPointIndex(int pointID);
    
    
    //Returns the index of the pathline closest to the given point
    int FindPathline(glm::vec3 pos, int time, float min = 10000.f);


    std::vector<int> visiblePoints;

    Filter* filter;

    int numFramesSeen;

    int timeSteps;
   
    std::vector<int> surfaceIndices;
    std::vector<int> pathOffsets;
    std::vector<int> pathCounts;
    std::vector<Vertex> pathVertices;
    std::vector<Vertex> tempPath;
    float* surfaceAreas;
    bool updatePaths;

//    std::vector<AABox> boxes;


    std::vector<int> pathlines;
    void computeLocations();
    GLuint buffer;
    GLuint surfaceTexBuffer;
	GLuint surfaceSplitBuffer;
	GLuint distanceBuffer;
    GLuint pathBuffer;
    GLuint tempPathBuffer;
    GLuint clusterBuffer;
    GLuint megaClusterBuffer;

    GLuint particleSimilarityBuffer;
    double maxSimilarityDistance;
    double midSimilarityDistance;

    GLuint particleClusterBuffer;
    int clusterVertCount;
    bool useSeparateBuffers = false;
    GLuint* pointBuffers;
    GLuint vao;

    MyShader* pointShader;
    MyShader* lineShader;
    MyShader* surfaceShader;
    GLuint surfaceIndexBuffer;
    GLuint surfaceAreasBuffer;
    std::vector< std::vector<Vertex> > pointLocations;
    std::vector< std::vector<Vertex> > pointDistances;
	
    glm::vec3 minV;
    glm::vec3 maxV;

    void retestVisible();

	std::map<int, int> idMap;
};

#endif // POINTMANAGER_H
