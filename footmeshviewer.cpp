#include <string>
#include <iostream>
#include <math.h>

#include <display/VRConsoleNode.h>
#include <main/VRMain.h>
#include <main/VREventHandler.h>
#include <main/VRRenderHandler.h>
#include <math/VRMath.h>


#if defined(WIN32)
#define NOMINMAX
#include <algorithm>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <windows.h>
#include <GL/gl.h>
#include <gl/GLU.h>
#elif defined(__APPLE__)
#include <GL/glew.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "slide.h"
#include "cursor.h"
#include "textboard.h"
#include "vrpoint.h"
#include "vertex.h"
#include "filter.h"
#include "pointmanager.h"
#include "animationcontroller.h"
#include "footmeshviewer.h"
#include "webupdatereader.h"
#include "PathAlignmentSimilarityEvaluator.h"

#include "Config.h"

//#include "GLUtil.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/ext.hpp"


using namespace MinVR;


/*void printMatrix4(VRMatrix4 m){
  for(int i = 0; i < 4; i++){
    for(int j = 0; j < 4; j++){
      printf("%6.2f ", m[i][j]);
    }
    printf("\n");
  }
}*/

void printMat4(glm::mat4 m) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            printf("%6.2f ", m[j][i]);
        }
        printf("\n");
    }
}


enum struct Mode { STANDARD, ANIMATION, FILTER, SLICES, PREDICT, CLUSTER, PATHSIZE, SIMILARITY };

class MyVRApp : public VREventHandler, public VRRenderHandler, public UpdateChecker {
public:
    MyVRApp(int argc, char** argv) : _vrMain(NULL), _quit(false), first(true), _owm(glm::mat4(1.0f)),
        _lastWandPos(glm::mat4(1.0f)), _radius(1.0f) {
        _vrMain = new VRMain();
        _vrMain->initialize(argc, argv);
        _vrMain->addEventHandler(this);
        _vrMain->addRenderHandler(this);

        std::cout << _vrMain->getConfig()->printStructure() << std::endl;

        //_config = std::make_unique<Config>("active.config");
        std::string config_path = "active.config";
        std::cout << "Default config: active.config" << std::endl;
        if (argc >= 3) {
            std::cout << "Loading: " << argv[3] << std::endl;
            config_path = std::string(argv[3]);
        }
        _config = std::make_unique<Config>(config_path);
        _config->Print();

        const unsigned char* s = glGetString(GL_VERSION);
        printf("GL Version %s.\n", s);
        _horizAngle = 0.0;
        _vertAngle = 0.0;
        _incAngle = -0.1f;
        _pm = new PointManager(); //initializes a point manager
        //_pm->ReadFile("data/slices-68-trimmed.out");
        _pm->ReadFile(_config->GetValue("Data", "active-dataset.out"));
        //_pm->ReadDistances(_config->GetValue("Distances", "distances.out"));
        printf("Loaded file with %d timesteps.\n", _pm->getLength());
        std::cout << _pm->getLength() << std::endl;
        //_pm->ReadMoVMFs("paths.movm");
        /*std::string clusterFile = _config->GetValue("Clusters", "active.clusters");
        if (clusterFile != ""){
          _pm->ReadClusters(clusterFile);
        }
        std::string pathlineFile = _config->GetValue("Pathlines", "active.pathlines");
        if (pathlineFile != ""){
          _pm->ReadPathlines(pathlineFile);
        }*/
        _pm->colorByCluster = true;
        _pm->simEval = new PathAlignmentSimilarityEvaluator();
        _pm->direction = std::stoi(_config->GetValue("direction", "0"), NULL);
        _pm->freezeStep = std::stoi(_config->GetValue("freezeStep", "0"), NULL);
        _pm->splitStep = std::stoi(_config->GetValue("splitStep", "0"), NULL);

        presets.push_back(_config->GetValue("preset01", "default01.pre"));
        presets.push_back(_config->GetValue("preset02", "default02.pre"));
        presets.push_back(_config->GetValue("preset03", "default03.pre"));
        presets.push_back(_config->GetValue("preset04", "default04.pre"));
        presets.push_back(_config->GetValue("preset05", "default05.pre"));


        std::string similarityThreshold = _config->GetValue("SimilarityThreshold", "0.007");
        float threshold = std::stod(similarityThreshold);
        _pm->simEval->threshold = threshold;

        std::string scaleSizeString = _config->GetValue("Scale", "10");
        float MetersPerFoot = 1; //Figure out yurt units and correct some other day...
        _scale = std::stod(scaleSizeString) * MetersPerFoot;

        ac.setFrameCount(_pm->getLength());
        ac.setSpeed(15);
        mode = Mode::STANDARD;
        _slicer = new SliceFilter();
    }

    virtual ~MyVRApp() {
        _vrMain->shutdown();
        delete _vrMain;
    }


    // Callback for event handling, inherited from VREventHandler
    void onVREvent(const VRDataIndex& event) {
        //if (event.getName() != "aimon_13_Change" &&
        //    event.getName() != "aimon_14_Change" &&event.getName() != "aimon_15_Change" && event.getName() != "aimon_16_Change" && event.getName() != "FrameStart"){
        if (event.getName() == "FrameStart") {
            secs = event.getValue("ElapsedSeconds");
            if (forward_hold) {
                if (hold_start + animation_repeat < secs) {
                    ac.stepForward();
                    hold_start = secs;
                }
            }
            else if (reverse_hold) {
                if (hold_start + animation_repeat < secs) {
                    ac.stepBackward();
                    hold_start = secs;
                }
            }
            else {
                hold_start = secs;
            }
        }

        if (event.getName().compare(0, 15, "HTC_Controller_") == 0) {
            //std::cout << event.getName() << std::endl;
            std::string e = event.getName();
            std::string suffix = e.substr(e.rfind('_'));
            e = e.substr(0, e.rfind('_'));
            if (suffix.compare("_Move") != 0 && suffix.compare("_Trigger1") != 0 && suffix.compare("_Trigger2") != 0) {
                e = e.substr(0, e.rfind('_'));
            }

            if (!e.compare(rcontrol) == 0 && !e.compare(lcontrol) == 0) {
                if (!rAssignedLast) {
                    rcontrol = e;
                    rAssignedLast = true;
                }
                else {
                    lcontrol = e;
                    rAssignedLast = false;
                }

                if (lcontrol.find("Right") != std::string::npos) {
                    std::string temp = rcontrol;
                    rcontrol = lcontrol;
                    lcontrol = temp;
                }

                std::string rctrlids = rcontrol.substr(rcontrol.rfind('_') + 1);
                std::string lctrlids = lcontrol.substr(lcontrol.rfind('_') + 1);
                int rctrlid = 0;
                int lctrlid = 0;

                bool parsed = true;
                try {
                    rctrlid = std::stoi(rctrlids);
                    lctrlid = std::stoi(lctrlids);
                }
                catch (const std::exception& e) {
                    parsed = false;
                }

                if (parsed && lctrlid < rctrlid) {
                    std::string temp = rcontrol;
                    rcontrol = lcontrol;
                    lcontrol = temp;
                }

            }
        }
        //std::cout << "Event: " << event.getName() << std::endl;

        if (event.getName() == "Wand0_Move" || event.getName() == rcontrol + "_Move") { //Set up for Wand Movement
            std::vector<float> h = event.getValue("Transform");
            glm::mat4 wandPos = glm::mat4(h[0], h[1], h[2], h[3],
                h[4], h[5], h[6], h[7],
                h[8], h[9], h[10], h[11],
                h[12], h[13], h[14], h[15]);
            //VRMatrix4 wandPosition = event.getValue("Wand0_Move/Transform"); //VRMatrix for the wand position
            //glm::mat4 wandPos = glm::make_mat4(wandPosition.m); //convert to 4 matrix
            if (_moving) {
                _owm = wandPos / _lastWandPos * _owm; // update the model matrix for the data points
            }
            if (_movingSlide) { //when slide moving
                _slideMat = wandPos / _lastWandPos * _slideMat; //update the model matrix for slide
            }
            _cursorMat = wandPos;
            _lastWandPos = wandPos;
        }

        if (event.getName() == "KbdU_Down") {
            _pm->cellSize += 100;
        }
        if (event.getName() == "KbdI_Down") {
            _pm->cellSize -= 100;
        }
        if (event.getName() == "KbdJ_Down") {
            _pm->cellLine += 0.00005;
        }
        if (event.getName() == "KbdK_Down") {
            _pm->cellLine -= 0.00005;
        }
        if (event.getName() == "KbdO_Down") {
            _pm->ripThreshold += 0.5;
        }
        if (event.getName() == "KbdL_Down") {
            _pm->ripThreshold -= 0.5;
        }
        if (event.getName() == "KbdF_Down") {
            _pm->thickinterval += 1;
        }
        if (event.getName() == "KbdG_Down") {
            _pm->thickinterval -= 1;
        }
        if (event.getName() == "KbdP_Down") {
            _pm->surfaceMode = ((_pm->surfaceMode + 1) % 5);
        }
        if (event.getName() == "KbdT_Down") {
            //_pm->surfTrail = !_pm->survfTrail;
            std::ofstream file;
            file.open(presets[currentPreset]);
            if (file.good()) {
                for (int i = 0; i < 4; i++) {
                    for (int j = 0; j < 4; j++) {
                        file << _owm[i][j] << " ";
                    }
                }
                file << std::endl;
                file << _pm->showSurface << _pm->surfaceMode << std::endl;
                file << _pm->pointSize << std::endl;
                file << layer1 << " " << layer2 << " " << layer3 << " " << layer4 << std::endl;
                file << _fmv.showStained << _fmv.sweepMode << std::endl;
                if (_pm->similarityReset) {
                    file << -1 << 0 << std::endl;
                }
                else {
                    file << _pm->similaritySeedPoint << " " << _similarityCount << std::endl;
                }
                file << ac.getFrame();
                file.close();
            }

        }

        if (event.getName() == "KbdX_Down") {
            mode = Mode::STANDARD;
            printf("moving to standard mode\n");
        }
        if (event.getName() == "KbdZ_Down") {
            //_fmv.sweepMode = ((_fmv.sweepMode + 1) % 6);
            mode = Mode::SIMILARITY;
            printf("moving to cluster mode\n");
        }


        if (event.getName() == "Kbd1_Down") {
            if (layer1 > 0) {
                layer1 = 0;
            }
            else {
                layer1 = 1;
            }
            _pm->layers = layer1 + layer2 + layer3 + layer4;
        }

        if (event.getName() == "Kbd2_Down") {
            if (layer2 > 0) {
                layer2 = 0;
            }
            else {
                layer2 = 2;
            }
            _pm->layers = layer1 + layer2 + layer3 + layer4;
        }

        if (event.getName() == "Kbd3_Down") {
            if (layer3 > 0) {
                layer3 = 0;
            }
            else {
                layer3 = 4;
            }
            _pm->layers = layer1 + layer2 + layer3 + layer4;
        }

        if (event.getName() == "Kbd4_Down") {
            if (layer4 > 0) {
                layer4 = 0;
            }
            else {
                layer4 = 8;
            }
            _pm->layers = layer1 + layer2 + layer3 + layer4;
        }

        if (/*event.getName() == "Kbd1_Down" ||*/ event.getName() == "Mouse_Up_Down" || ((event.getName() == lcontrol + "_Axis0Button_Down") && trackPadActive("left") && (left_trackPad0_X * left_trackPad0_X) + (left_trackPad0_Y * left_trackPad0_Y) >= (0.33 * 0.33) && left_trackPad0_Y > 0.0 && abs(left_trackPad0_X) < left_trackPad0_Y)) {
            //mode = Mode::STANDARD;
            //printf("moving to standard\n");

            if (mode == Mode::STANDARD) {
                _fmv.showStained = !_fmv.showStained;
            }
        }
        else if (event.getName() == "Mouse_Down_Down" || ((event.getName() == "HTC_Controller_Left_Axis0Button_Down" || event.getName() == "HTC_Controller_2_Axis0Button_Down") && (left_trackPad0_X * left_trackPad0_X) + (left_trackPad0_Y * left_trackPad0_Y) >= (0.33 * 0.33) && left_trackPad0_Y < 0.0 && abs(left_trackPad0_X) < -left_trackPad0_Y)) {
            //else if (/*event.getName() == "Kbd2_Down" ||*/ event.getName() == "Mouse_Down_Down") {
              //mode = Mode::CLUSTER;
              //mode = Mode::ANIMATION;
              //mode = Mode::PATHSIZE;
              /*_pm->colorByCluster = ! _pm->colorByCluster;
              _pm->colorBySimilarity = !_pm->colorBySimilarity;
              if(_slicing){
                _pm->cutOnOriginal = !_pm->cutOnOriginal;
              }*/

            _fmv.sweepMode = ((_fmv.sweepMode + 1) % 6);
        }
        else if (/*event.getName() == "Kbd3_Down" ||*/ event.getName() == "Mouse_Left_Down" || ((event.getName() == lcontrol + "_Axis0Button_Down") && trackPadActive("left") && (left_trackPad0_X * left_trackPad0_X) + (left_trackPad0_Y * left_trackPad0_Y) >= (0.33 * 0.33) && left_trackPad0_X < 0.0 && abs(left_trackPad0_Y) < -left_trackPad0_X)) {
            //mode = Mode::FILTER;
            //_pm->SetFilter(new MotionFilter(motionThreshold));
            if (mode == Mode::STANDARD) {
                _pm->showSurface = !_pm->showSurface;
            }

            if (mode == Mode::SIMILARITY) {
                _pm->ClearPathlines();
                _pm->ResetPrediction();
                _pm->clustering = false;
                _pm->currentCluster = -1;
                _pm->colorBySimilarity = false;
            }
        }
        else if (/*event.getName() == "Kbd4_Down" ||*/ event.getName() == "Mouse_Right_Down" || ((event.getName() == lcontrol + "_Axis0Button_Down") && trackPadActive("left") && (left_trackPad0_X * left_trackPad0_X) + (left_trackPad0_Y * left_trackPad0_Y) >= (0.33 * 0.33) && left_trackPad0_X > 0.0 && abs(left_trackPad0_Y) < left_trackPad0_X)) {

            //mode = Mode::SIMILARITY;


            _fmv.showOldModel = !_fmv.showOldModel;
            //_pm->clustering = true;
            //_pm->ClearPathlines();
            //mode = Mode::SLICES;
            //_pm->SetFilter(_slicer);
        }
        else if (event.getName() == "Kbd5_Down") {//instacrashes :( || event.getName() == "Mouse_Left_Click_Down"){
          //_pm->SetShaders();

        }
        else if (event.getName() == "Mouse_Left_Click_Down" || event.getName() == "KbdD_Down" || ((event.getName() == lcontrol + "_Axis0Button_Down") && trackPadActive("left") && (left_trackPad0_X * left_trackPad0_X) + (left_trackPad0_Y * left_trackPad0_Y) < (0.33 * 0.33))) {
            //else if (event.getName() == "KbdG_Down" || hasEnding(event.getName(), "ApplicationMenuButton_Down")) {
              //_pm->showSurface = !_pm->showSurface;
        }
        else if (event.getName() == "Kbd6_Down" || event.getName() == "Mouse_Right_Click_Down" || event.getName() == rcontrol + "_GripButton_Down") {
            //_pm->colorPathsBySimilarity = !_pm->colorPathsBySimilarity;
            if (!_slicing) {
                if (!_slicingHold) {
                    _slicing = true;
                }
                else {
                    _slicingHold = false;
                }
            }
            else {
                if (!_slicingHold) {
                    _slicing = false;
                    _slicingHold = true;
                }
                else {
                    _slicing = false;
                    _slicingHold = false;
                }
            }
        }
        else if (event.getName() == "Kbd7_Down") {
            //_pm->colorPathsBySimilarity = !_pm->colorPathsBySimilarity;
            //_slicingHold = !_slicingHold;
            _pm->distMode = ((_pm->distMode + 1) % 4);
        }
        else if (event.getName() == "KbdV_Down") {// || ((event.getName() == rcontrol + "_Axis0Button_Down") && trackPadActive("right") &&(right_trackPad0_X*right_trackPad0_X) + (right_trackPad0_Y*right_trackPad0_Y) < (0.33*0.33))) {
            _fmv.showOldModel = !_fmv.showOldModel;
        }
        else if (event.getName() == "KbdB_Down") {
            //_fmv.showNewModel = !_fmv.showNewModel;
            _fmv.showStained = !_fmv.showStained;
        }
        else if (event.getName() == "KbdN_Down") {
            _pm->distFalloff += 0.002;
        }
        else if (event.getName() == "KbdM_Down") {
            //_fmv.showCloud = !_fmv.showCloud;
            _pm->distFalloff -= 0.002;
        }
        else if (event.getName() == "KbdS_Down") {
            _pm->cutOnOriginal = !_pm->cutOnOriginal;
        }
        else if (event.getName() == rcontrol + "_Trigger1") {
            float analog = event.getValue("AnalogValue");
            if (right_trigger == false && analog == 1.0f) {
                _moving = true;
                right_trigger = true;
            }
            if (right_trigger == true && analog < 1.0f) {
                _moving = false;
                right_trigger = false;
            }
        }
        else if (event.getName() == rcontrol + "_TrackPad0_X" || event.getName() == rcontrol + "_Joystick0_X") {
            right_trackPad0_X = event.getValue("AnalogValue");
            right_trackPad0_time = secs;
        }
        else if (event.getName() == rcontrol + "_TrackPad0_Y" || event.getName() == rcontrol + "_Joystick0_Y") {
            right_trackPad0_Y = event.getValue("AnalogValue");
            right_trackPad0_time = secs;
        }
        else if (event.getName() == lcontrol + "_TrackPad0_X" || event.getName() == lcontrol + "_Joystick0_X") {
            left_trackPad0_X = event.getValue("AnalogValue");
            left_trackPad0_time = secs;
        }
        else if (event.getName() == lcontrol + "_TrackPad0_Y" || event.getName() == lcontrol + "_Joystick0_Y") {
            left_trackPad0_Y = event.getValue("AnalogValue");
            left_trackPad0_time = secs;
        }
        else if (event.getName() == "MouseBtnLeft_Down" || event.getName() == "Wand_Bottom_Trigger_Down" || event.getName() == rcontrol + "_Axis1Button_Down") {
            _moving = true;
        }
        else if (event.getName() == "MouseBtnLeft_Up" || event.getName() == "Wand_Bottom_Trigger_Up" || event.getName() == rcontrol + "_Axis1Button_Up") {
            _moving = false;
        }
        else if (event.getName() == "Wand_Joystick_Press_Down") {
            _movingSlide = true;
        }
        else if (event.getName() == "Wand_Joystick_Press_Up" || event.getName() == rcontrol + "_Axis0Button_Up") {
            _movingSlide = false;
            reverse_hold = false;
            forward_hold = false;
        }
        else if (event.getName() == "Wand_Trigger_Top_Change" || event.getName() == rcontrol + "_ApplicationMenuButton_Down") {
            //_placePathline = true;
        }
        //else if (event.getName() == "KbdDown_Down" || event.getName() == "Wand_Down_Down" || (hasEnding(event.getName(),"Axis0Button_Down") && (right_trackPad0_X*right_trackPad0_X) + (right_trackPad0_Y*right_trackPad0_Y) >= (0.33*0.33) && right_trackPad0_Y<0.0 && abs(right_trackPad0_X) < -right_trackPad0_Y)) {
        else if (event.getName() == "KbdDown_Down" || event.getName() == "Wand_Down_Down" || ((event.getName() == rcontrol + "_Axis0Button_Down") && trackPadActive("right") && (right_trackPad0_X * right_trackPad0_X) + (right_trackPad0_Y * right_trackPad0_Y) >= (0.33 * 0.33) && right_trackPad0_Y < 0.0 && abs(right_trackPad0_X) < -right_trackPad0_Y)) {
            if (mode == Mode::STANDARD) {
                _pm->pointSize /= 1.3;
            }
            if (mode == Mode::ANIMATION) {
                ac.decreaseSpeed();
            }
            if (mode == Mode::SLICES) {
                _slicer->addStart(.001);
                _pm->SetFilter(_slicer);
            }
            if (mode == Mode::FILTER) {
                motionThreshold *= 1.414;
                _pm->SetFilter(new MotionFilter(motionThreshold));
            }
            if (mode == Mode::PREDICT) {
                _placeClusterSeed = false;
            }
            if (mode == Mode::CLUSTER && iterateClusters) {
                _pm->currentCluster--;
                if (_pm->currentCluster < 0) {
                    _pm->currentCluster = _pm->clusters.size() - 1;
                }
                _pm->clustersChanged = true;
            }
            if (mode == Mode::PATHSIZE) {
                _pm->pathlineMin -= 0.04;
            }
            if (mode == Mode::SIMILARITY) {
                _similarityCount /= 1.2;
                _pm->ExpandClosestPoints(_similarityCount);
            }
        }
        else if (event.getName() == "KbdUp_Down" || event.getName() == "Wand_Up_Down" || ((event.getName() == rcontrol + "_Axis0Button_Down") && trackPadActive("right") && (right_trackPad0_X * right_trackPad0_X) + (right_trackPad0_Y * right_trackPad0_Y) >= (0.33 * 0.33) && right_trackPad0_Y > 0.0 && abs(right_trackPad0_X) < right_trackPad0_Y)) {
            //else if (event.getName() == "KbdUp_Down" || event.getName() == "Wand_Up_Down" || (hasEnding(event.getName(), "Axis0Button_Down") && (right_trackPad0_X*right_trackPad0_X) + (right_trackPad0_Y*right_trackPad0_Y) >= (0.33*0.33) && right_trackPad0_Y>0.0 && abs(right_trackPad0_X) < right_trackPad0_Y)) {
            if (mode == Mode::STANDARD) {
                _pm->pointSize *= 1.3;
            }
            if (mode == Mode::ANIMATION) {
                ac.increaseSpeed();
            }
            if (mode == Mode::SLICES) {
                _slicer->addStart(-.001);
                _pm->SetFilter(_slicer);
            }
            if (mode == Mode::FILTER) {
                motionThreshold /= 1.414;
                _pm->SetFilter(new MotionFilter(motionThreshold));
            }
            if (mode == Mode::PREDICT) {
                //_pm->SearchForSeeds();
                _placeClusterSeed = true;
            }
            if (mode == Mode::CLUSTER && iterateClusters) {
                _pm->currentCluster++;
                if (_pm->currentCluster >= _pm->clusters.size()) {
                    _pm->currentCluster = 0;
                }
                _pm->clustersChanged = true;
            }
            if (mode == Mode::PATHSIZE) {
                _pm->pathlineMin += 0.04;
            }
            if (mode == Mode::SIMILARITY) {
                _similarityCount *= 1.2;
                _pm->ExpandClosestPoints(_similarityCount);
            }
        }
        else if (event.getName() == "KbdLeft_Down" || event.getName() == "Wand_Left_Down" || ((event.getName() == rcontrol + "_Axis0Button_Down") && trackPadActive("right") && (right_trackPad0_X * right_trackPad0_X) + (right_trackPad0_Y * right_trackPad0_Y) >= (0.33 * 0.33) && right_trackPad0_X < 0.0 && abs(right_trackPad0_Y) < -right_trackPad0_X)) {
            if (mode == Mode::SLICES) {
                _slicer->addGap(-0.0025);
                _pm->SetFilter(_slicer);
            }
            else if (mode == Mode::PREDICT) {
                //_pm->SearchForSeeds();
            }
            else if (mode == Mode::PATHSIZE) {
                _pm->pathlineMax -= 0.04;
            }
            else if (mode == Mode::SIMILARITY) {
                _similarityGo = true;
                mode = Mode::STANDARD;
            }
            else {
                ac.stepBackward();
                //std::cout << event.getName() << std::endl;
                reverse_hold = true;
            }
        }
        else if (event.getName() == "KbdRight_Down" || event.getName() == "Wand_Right_Down" || ((event.getName() == rcontrol + "_Axis0Button_Down") && trackPadActive("right") && (right_trackPad0_X * right_trackPad0_X) + (right_trackPad0_Y * right_trackPad0_Y) >= (0.33 * 0.33) && right_trackPad0_X > 0.0 && abs(right_trackPad0_Y) < right_trackPad0_X)) {
            if (mode == Mode::SLICES) {
                _slicer->addGap(.0025);
                _pm->SetFilter(_slicer);
            }
            else if (mode == Mode::PATHSIZE) {
                _pm->pathlineMax += 0.04;
            }
            else if (mode == Mode::SIMILARITY) {
                _pm->colorPathsBySimilarity = !_pm->colorPathsBySimilarity;
            }
            else {
                ac.stepForward();
                //std::cout << event.getName() << std::endl;
                forward_hold = true;
            }
        }
        else if (event.getName() == "KbdSpace_Down" || event.getName() == "Wand_Select_Down" || event.getName() == lcontrol + "_GripButton_Down") {
            ac.togglePlay();
            if (mode == Mode::PREDICT) {
                //_pm->SearchForSeeds();
            }
        }
        else if (event.getName() == "KbdI_Down") {
            _pm->pathlineMin += 0.05;
        }
        else if (event.getName() == "KbdK_Down") {
            _pm->pathlineMin -= 0.05;
        }
        else if (event.getName() == "KbdU_Down") {
            _pm->pathlineMax += 0.05;
        }
        else if (event.getName() == "KbdJ_Down") {
            _pm->pathlineMax -= 0.05;
        }
        else if (event.getName() == "KbdR_Down") {
            std::fstream file;
            std::string line;
            file.open(presets[currentPreset], std::ios::in);
            if (file.good()) {

                std::getline(file, line);
                std::istringstream iss(line);
                float m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16;
                iss >> m1 >> m2 >> m3 >> m4 >> m5 >> m6 >> m7 >> m8 >> m9 >> m10 >> m11 >> m12 >> m13 >> m14 >> m15 >> m16;
                _owm = glm::mat4(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16);

                std::getline(file, line);
                std::istringstream iss0(line);
                int showsurf, surfmod;
                iss0 >> showsurf;
                if (showsurf == 1) {
                    _pm->showSurface = true;
                }
                else {
                    _pm->showSurface = false;
                }
                _pm->surfaceMode = surfmod;

                std::getline(file, line);
                std::istringstream iss2(line);
                float pointsize;
                iss2 >> pointsize;
                _pm->pointSize = pointsize;

                std::getline(file, line);
                std::istringstream iss3(line);
                int l1, l2, l3, l4;
                iss3 >> l1 >> l2 >> l3 >> l4;
                layer1 = l1;
                layer2 = l2;
                layer3 = l3;
                layer4 = l4;
                _pm->layers = l1 + l2 + l3 + l4;

                std::getline(file, line);
                std::istringstream iss4(line);
                int showsweep, sweepmod;
                iss4 >> showsweep >> sweepmod;
                if (showsweep == 1) {
                    _fmv.showStained = true;
                }
                else {
                    _fmv.showStained = false;
                }
                _fmv.sweepMode = sweepmod;


                std::getline(file, line);
                std::istringstream iss5(line);
                int id;
                float count;
                iss5 >> id >> count;

                _pm->ClearPathlines();
                _pm->ResetPrediction();
                _pm->clustering = false;
                _pm->currentCluster = -1;
                _pm->drawAllClusters = false;
                _pm->pathlineMin = 0.0;
                _pm->pathlineMax = 1.0;
                _pm->colorBySimilarity = false;

                if (id > -1) {
                    _similarityPreset = true;
                    _similarityId = id;
                    _similarityCount = count;
                }

                std::getline(file, line);
                std::istringstream iss6(line);
                int frame;
                iss6 >> frame;
                if (frame > -1) {
                    ac.setFrame(frame);
                }
                ac.pause();
                file.close();
            }
            else {
                _owm = _owmDefault;
                _pm->showSurface = false;
                _fmv.showStained = false;
                _fmv.sweepMode = 0;
                _pm->pointSize = 0.0004;
                _pm->surfaceMode = 0;

                _pm->ClearPathlines();
                _pm->ResetPrediction();
                _pm->clustering = false;
                _pm->currentCluster = -1;
                _pm->drawAllClusters = false;
                _pm->pathlineMin = 0.0;
                _pm->pathlineMax = 1.0;
                _pm->colorBySimilarity = false;

                _pm->layers = 0;
                layer1 = 0;
                layer2 = 0;
                layer3 = 0;
                layer4 = 0;


                ac.setFrame(0);
                ac.pause();
            }

        }
        else if (event.getName() == "KbdW_Down") {
            if (currentPreset == 0) {
                currentPreset = 4;
            }
            else {
                currentPreset -= 1;
            }
            std::cout << "Preset is now " << currentPreset + 1 << std::endl;
        }
        else if (event.getName() == "KbdE_Down") {
            if (currentPreset == 4) {
                currentPreset = 0;
            }
            else {
                currentPreset += 1;
            }
            std::cout << "Preset is now " << currentPreset + 1 << std::endl;
        }
        else if (event.getName() == "KbdQ_Down") {
            _owm = _owmDefault;
            _pm->showSurface = false;
            _pm->pointSize = 0.0004;

            _pm->ClearPathlines();
            _pm->ResetPrediction();
            _pm->clustering = false;
            _pm->currentCluster = -1;
            _pm->drawAllClusters = false;
            _pm->pathlineMin = 0.0;
            _pm->pathlineMax = 1.0;
            _pm->colorBySimilarity = false;

            _pm->layers = 0;
            layer1 = 0;
            layer2 = 0;
            layer3 = 0;
            layer4 = 0;


            ac.setFrame(0);
            ac.pause();
        }


        if (event.getName() == "KbdEsc_Down") {
            _quit = true;
        }
        else if (event.getName() == "MouseBtnLeft_Down") {
            _radius += 5.0 * _incAngle;
        }
        else if (event.getName() == "MouseBtnRight_Down") {
            _radius -= 5.0 * _incAngle;
        }
        else if ((event.getName() == "KbdLeft_Down") || (event.getName() == "KbdLeft_Repeat")) {
            //_horizAngle -= _incAngle;
        }
        else if ((event.getName() == "KbdRight_Down") || (event.getName() == "KbdRight_Repeat")) {
            //_horizAngle += _incAngle;
        }
        else if ((event.getName() == "KbdUp_Down") || (event.getName() == "KbdUp_Repeat")) {
            //_vertAngle -= _incAngle;
        }
        else if ((event.getName() == "KbdDown_Down") || (event.getName() == "KbdDown_Repeat")) {
            //_vertAngle += _incAngle;
        }

        if (_horizAngle > 6.283185) _horizAngle -= 6.283185;
        if (_horizAngle < 0.0) _horizAngle += 6.283185;

        if (_vertAngle > 6.283185) _vertAngle -= 6.283185;
        if (_vertAngle < 0.0) _vertAngle += 6.283185;
    }


    virtual void onVRRenderContext(VRDataIndex* renderState, VRDisplayNode* callingNode) {
        if (!renderState->exists("IsConsole", "/")) {
        }
        _wur->checkForUpdates();
        time = ac.getFrame(); //update time frame
    }

    int count;

    // Callback for rendering, inherited from VRRenderHandler
    void onVRRenderScene(const VRDataIndex& stateData) {
        time = ac.getFrame(); //update time frame
        glCheckError();
        if (stateData.exists("IsConsole", "/")) {
            //VRConsoleNode *console = dynamic_cast<VRConsoleNode*>(callingNode);
            //console->println("Console output...");
        }
        else {

            if (first) { //setting up
                glewExperimental = GL_TRUE;
                glewInit();
                const unsigned char* s = glGetString(GL_VERSION);
                printf("GL Version %s.\n", s);
                _pm->SetupDraw();
                _cursor.Initialize(1.0f);
                //  _board.Initialize("numbers.png", glm::vec3(2,-2,0), glm::vec3(0, 0,-0.55*2), glm::vec3(0,0.65*2,0));
                _board.Initialize("numbers.png", glm::vec3(-5, 2, -3), glm::vec3(0.55 * 0.7, 0, 0), glm::vec3(0, 0.65 * 0.7, 0));
                std::string slideFile = _config->GetValue("Slide");
                if (slideFile != "") {
                    _slide.Initialize(slideFile, glm::vec3(-5, 2, -3), glm::vec3(0, 2, 0), glm::vec3(0, 0, 2));
                }
                std::string feetFile = _config->GetValue("Foot", "feet.feet");
                if (feetFile != "") {
                    _fmv.ReadFiles(feetFile);
                }
                std::string surfaceFile = _config->GetValue("Surface");
                if (surfaceFile != "") {
                    _pm->ReadSurface(surfaceFile);
                }
                first = false;
            }
            GLuint test;
            glCheckError();
            glGenBuffers(1, &test);
            glCheckError();
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glClearDepth(1.0f);
            count++;
            glClearColor(0.0, 0.0, 0.0, 1.f);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            //glClear(GL_DEPTH_BUFFER_BIT);
            glCheckError();

            glm::mat4 M(1.0f), V, P;
            glm::mat4 MVP;
            // std::cout << stateData.printStructure() << std::endl;
            if (stateData.exists("ProjectionMatrix", "/")) {
                //std::cout << stateData << std::endl;
                          // This is the typical case where the MinVR DisplayGraph contains
                          // an OffAxisProjectionNode or similar node, which sets the
                          // ProjectionMatrix and ViewMatrix based on head tracking data.


                          //VRMatrix4 PP = stateData.getValue("ProjectionMatrix", "");
                std::vector<float> PP = stateData.getValue("ProjectionMatrix");

                // In the original adaptee.cpp program, keyboard and mouse commands
                // were used to adjust the camera.  Now that we are creating a VR
                // program, we want the camera (Projection and View matrices) to
                // be controlled by head tracking.  So, we switch to having the
                // keyboard and mouse control the Model matrix.  Guideline: In VR,
                // put all of the "scene navigation" into the Model matrix and
                // leave the Projection and View matrices for head tracking.


                //VRMatrix4 MM = VRMatrix4::translation(VRVector3(0.0, 0.0, -_radius)) * VRMatrix4::rotationX(_vertAngle) * VRMatrix4::rotationY(_horizAngle);

                //VRMatrix4 VV = stateData.getValue("ViewMatrix", "/");
                std::vector<float> VV = stateData.getValue("ViewMatrix");
                V = glm::mat4(VV[0], VV[1], VV[2], VV[3],
                    VV[4], VV[5], VV[6], VV[7],
                    VV[8], VV[9], VV[10], VV[11],
                    VV[12], VV[13], VV[14], VV[15]);
                //	  std::cout << "V " << glm::to_string(V) << std::endl;
                          //glLoadMatrixd((VV*MM).m);

                          //V = glm::make_mat4(VV.m);
                          //P = glm::make_mat4(PP.m);
                          //M = glm::make_mat4(MM.m);

                P = glm::mat4(PP[0], PP[1], PP[2], PP[3],
                    PP[4], PP[5], PP[6], PP[7],
                    PP[8], PP[9], PP[10], PP[11],
                    PP[12], PP[13], PP[14], PP[15]);
                //	  std::cout << "P " << glm::to_string(P) << std::endl;

                glm::mat4 MMtrans = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -_radius));
                glm::mat4 MMrotX = glm::rotate(MMtrans, _vertAngle, glm::vec3(1.0f, 0.0f, 0.0f));
                //  Original line
                // glm::mat4 M = glm::rotate(MMrotX, _horizAngle, glm::vec3(0.0f, 1.0f, 0.0f)); 
                M = glm::rotate(MMrotX, _horizAngle, glm::vec3(0.0f, 1.0f, 0.0f));
                //	  std::cout << "M " << glm::to_string(M) << std::endl;

                V = glm::mat4(VV[0], VV[1], VV[2], VV[3],
                    VV[4], VV[5], VV[6], VV[7],
                    VV[8], VV[9], VV[10], VV[11],
                    VV[12], VV[13], VV[14], VV[15]);
            }
            else {
                // If the DisplayGraph does not contain a node that sets the
                // ProjectionMatrix and ViewMatrix, then we must be in a non-headtracked
                // simple desktop mode.  We can just set the projection and modelview
                // matrices the same way as in adaptee.cpp.


                P = glm::perspective(1.6f * 45.f, 1.f, 0.1f, 100.0f);

                glm::vec3 pos = glm::vec3(_radius * cos(_horizAngle) * cos(_vertAngle),
                    -_radius * sin(_vertAngle),
                    _radius * sin(_horizAngle) * cos(_vertAngle));
                glm::vec3 up = glm::vec3(cos(_horizAngle) * sin(_vertAngle),
                    cos(_vertAngle),
                    sin(_horizAngle) * sin(_vertAngle));

                V = glm::lookAt(pos, glm::vec3(0.f), up);

            }

            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(_scale, _scale, _scale));
            glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(1, -1, 0));


            //in desktop mode, +x is away from camera, +z is right, +y is up 
            //M = translate * scale;
            glm::mat4 slideM = _slideMat * M;
            glm::mat4 cursorM = _cursorMat;
            //        std::cout << "cursor" << std::endl;
            //        printMat4(cursorM);
            //        std::cout << "slide" << std::endl;
            //        printMat4(slideM);
            M = _owm * M * translate * scale;
            //M =  M * translate * scale;

           //V = glm::transpose(V);
            MVP = P * V * M;
            glm::mat4 t = V;

            glm::mat4 newWandPos = glm::translate(_lastWandPos, glm::vec3(0.0f, 0.0f, -1.0f));
            glm::vec3 location = glm::vec3(newWandPos[3]);
            glm::vec4 modelPos = glm::inverse(M) * glm::vec4(location, 1.0);


            int id = _pm->TempPathline(glm::vec3(modelPos), time);
            /*printf("World space location: %f, %f, %f\n", location.x, location.y, location.z);
            printf("Model space location: %f, %f, %f\n", modelPos.x, modelPos.y, modelPos.z);
            printMat4(glm::inverse(M));
            printMat4(M);*/
            _board.LoadID(id);
            _board.LoadNumber(_similarityCount);
            if (mode == Mode::SIMILARITY) {
                _board.LoadID(id);
                _board.LoadNumber(_similarityCount);
            }
            if (_placePathline) {
                _placePathline = false;
                if (mode == Mode::PREDICT) {
                    _pm->ShowCluster(glm::vec3(modelPos), time);
                }
                else {
                    _pm->AddPathline(glm::vec3(modelPos), time);
                }
            }
            if (_placeClusterSeed) {
                _pm->ShowCluster(glm::vec3(modelPos), time);
            }
            if (_similarityGo) {
                _similarityGo = false;
                _pm->FindClosestPoints(glm::vec3(modelPos), time, (int)_similarityCount);
            }
            if (_similarityPreset) {
                _similarityPreset = false;
                _pm->FindClosestPoints(_similarityId, (int)_similarityCount);
            }

            if (!_slicingHold) {
                cuttingPlane = glm::vec4(0.0);
            }

            if (_slicing) {
                glm::vec3 up = glm::vec3(_lastWandPos[1]);
                up = glm::normalize(up);
                glm::vec3 modelUp = glm::inverse(M) * glm::vec4(up, 0.0);
                float offset = -glm::dot(glm::vec3(modelPos), modelUp);
                cuttingPlane = glm::vec4(modelUp.x, modelUp.y, modelUp.z, offset);
                //std::cout << "Cutting plane: " << glm::to_string(cuttingPlane) << std::endl;
            }



            glCheckError();

            _pm->Draw(time, MVP, V * M, P, cuttingPlane);

            if (showFoot) {
                _fmv.Draw(time, MVP);
            }
            glCheckError();

            glm::mat4 p;
            glm::mat4 m = P * V * cursorM;
            //_board.Draw(P * V * slideM);
            _cursor.Draw(m);
            //        _cursor.Draw(p);
                    //_slide.Draw(P * V * slideM);
            glCheckError();
        }
    }

    bool trackPadActive(std::string pad) {
        if (pad == "left" && (secs - left_trackPad0_time) < trackpad_delay) {
            return true;
        }
        if (pad == "right" && (secs - right_trackPad0_time) < trackpad_delay) {
            return true;
        }
        return false;
    }

    void run() {
        while (!_quit) {
            //glCheckError();
            _vrMain->mainloop();
        }
    }

protected:
    VRMain* _vrMain;
    bool _quit;
    float _horizAngle, _vertAngle, _radius, _incAngle;
    PointManager* _pm;
    bool first;
    int frame;
    Mode mode;
    float motionThreshold = 0.01;
    AnimationController ac;
    bool _moving = false;
    bool _placePathline;
    bool _placeClusterSeed = false;
    SliceFilter* _slicer;
    glm::mat4 _owm, _owmDefault;
    glm::mat4 _lastWandPos;
    FootMeshViewer _fmv;
    bool showFoot = true;
    WebUpdateReader* _wur;
    bool iterateClusters = false;
    float time = 0;
    Slide _slide;
    Textboard _board;
    Cursor _cursor;
    glm::mat4 _slideMat;
    glm::mat4 _cursorMat;
    bool _movingSlide = false;
    double _similarityCount = 50;
    bool _similarityGo = false;
    bool _similarityPreset = false;
    int _similarityId = -1;
    std::string rcontrol = "";
    std::string lcontrol = "";
    bool left_trigger = false, right_trigger = false;
    bool forward_hold = false, reverse_hold = false;
    float hold_start;
    float animation_repeat = 0.1;

    bool _slicing = false;
    bool _slicingHold = false;
    std::unique_ptr<Config> _config;
    float _scale = 40;
    glm::vec4 cuttingPlane;
    float left_trackPad0_X = 0.0f, left_trackPad0_Y = 0.0f, left_trackPad0_time = 0.0f, right_trackPad0_X = 0.0f, right_trackPad0_Y = 0.0f, right_trackPad0_time = 0.0f;
    float secs;
    float trackpad_delay = 0.2f;
    int layer1 = 0;
    int layer2 = 0;
    int layer3 = 0;
    int layer4 = 0;
    bool rAssignedLast = false;

    std::vector<std::string> presets;
    int currentPreset = 0;
};



int main(int argc, char** argv) {
    MyVRApp app(argc, argv);
    app.run();

    return 0;
}

