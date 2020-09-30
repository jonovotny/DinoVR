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

enum LAYERS
{
	LAYER1 = 1 << 0, // binary 0001
	LAYER2 = 1 << 1, // binary 0010
	LAYER3 = 1 << 2, // binary 0100
	LAYER4 = 1 << 3, // binary 1000
};

enum TRACKPAD_DIRECTION
{
	NONE,
	CENTER,
	UP,
	DOWN,
	LEFT,
	RIGHT
};

enum SLICEMODE {
	NO_SLICE,
	SLICE,
	SLICE_HOLD
};

//Note VRMatrix4 can be used with normal output
// e.g. std::cerr << m ;
//I would remove this function completely
/*void printMatrix4(VRMatrix4 m){
  for(int i = 0; i < 4; i++){
	for(int j = 0; j < 4; j++){
	  printf("%6.2f ", m[i][j]);
	}
	printf("\n");
  }
}*/

//Note there is a function from glm to print a matrix or vector.
//so i would remove the function to print
//#include <glm/gtx/string_cast.hpp>
//glm::to_string(mat)
void printMat4(glm::mat4 m) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			printf("%6.2f ", m[j][i]);
		}
		printf("\n");
	}
}


enum struct Mode { STANDARD, ANIMATION, FILTER, SLICES, PREDICT, CLUSTER, PATHSIZE, SIMILARITY };

class MyVRApp : public VREventHandler, public VRRenderHandler, public UpdateChecker, public VRInputDevice {
public:
	MyVRApp(int argc, char** argv) : _vrMain(NULL), _quit(false) {
		_vrMain = new VRMain();
		_vrMain->initialize(argc, argv);
		_vrMain->addEventHandler(this);
		_vrMain->addRenderHandler(this);
		_vrMain->addInputDevice(this);

		std::cout << _vrMain->getConfig()->printStructure() << std::endl;

		//_config = std::make_unique<Config>("active.config");
		std::string config_path = "active.config";
		std::cout << "Default config: active.config" << std::endl;
		if (argc >= 3 && argv[3]) {
			std::cout << "Loading: " << argv[3] << std::endl;
			config_path = std::string(argv[3]);
		}
		_config = std::make_unique<Config>(config_path);
		_config->Print();

		const unsigned char* s = glGetString(GL_VERSION);
		printf("GL Version %s.\n", s);

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

	//The events are used to syncronize frames between multiple users when pausing and playing the animation
	void addAnimationEvent(int pause) {
		VRDataIndex di("Animation");
		di.addData("Frame", ac.getCurrentFrame());
		di.addData("Pause", pause);
		_events_out.push_back(di);
		std::cerr << "Add animation event " << ac.getCurrentFrame() << std::endl;
	}

	void addTransformEvent(std::string eventname, float * transform) {
		VRDataIndex di(eventname);
		std::vector<float> t{ transform, transform + 16 };
		di.addData("Transform", t);
	}

	void addVectorEvent(std::string eventname, float * vec) {
		VRDataIndex di(eventname);
		std::vector<float> t{ vec, vec + 4 };
		di.addData("Vector", t);
	}

	void addKeyEvent(std::string name) {
		VRDataIndex di(name);
		_events_out.push_back(di);
	}

	void checkControllers(std::string eventname) {
		if (eventname.compare(0, 15, "HTC_Controller_") == 0) {
			std::string e = eventname;
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
	}

	void loadStuff1() {
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
			file << (_pm->layers & LAYER1) << " " << (_pm->layers & LAYER2) << " " << (_pm->layers & LAYER3) << " " << (_pm->layers & LAYER4) << std::endl;
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

	void loadStuff2() {
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

			ac.setFrame(0);
			ac.pause();
		}
	}

	void setPreset(int preset) {
		if (preset >= 5) {
			currentPreset = 0;
		}
		else if (preset < 0) {
			preset = 4;
		}
		else {
			currentPreset = preset;
		}
		std::cout << "Preset is now " << currentPreset + 1 << std::endl;
	}

	void reset() {
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

		ac.setFrame(0);
		ac.pause();
	}

	void handleLocalEvents(const VRDataIndex &event) {
		//These events should be syncronized but we remap to syncronized ones 
		//This makes it easier as in some cases it requires more local states 
		//and it also helps to keep the whitelist smaller
		replaceEventsForWandAndController(event.getName());

		std::string eventname = event.getName();

		//instead of syncing the space key event we add an extra event to make sure it stops at the same frame
		if (eventname == "KbdSpace_Down" || eventname == "Wand_Select_Down" || eventname == lcontrol + "_GripButton_Down") {
			if (ac.isPlaying()) {
				addAnimationEvent(1);
			}
			else {
				addAnimationEvent(0);
			}
		}

		//checking if left - right controller names have to be switched
		checkControllers(eventname);

		if (eventname == "KbdEsc_Down") {
			_quit = true;
		}
		else if (eventname == "Kbd6_Down") {
			if (_slice_mode == NO_SLICE) {
				_slice_mode = SLICE;
			}
			else if (_slice_mode == SLICE_HOLD) {
				_slice_mode = NO_SLICE;
				glm::vec4 tmp = glm::vec4(0.0f);
				addVectorEvent("CuttingPlane", glm::value_ptr(tmp));
			}
			else if (_slice_mode == SLICE) {
				_slice_mode = SLICE_HOLD;
			}
		}
		else if (eventname == "MouseBtnLeft_Down" || eventname == "Wand_Bottom_Trigger_Down" || eventname == rcontrol + "_Axis1Button_Down") {
			_moving = true;
		}
		else if (eventname == "MouseBtnLeft_Up" || eventname == "Wand_Bottom_Trigger_Up" || eventname == rcontrol + "_Axis1Button_Up") {
			_moving = false;
		}
		else if (eventname == "Wand_Joystick_Press_Down") {
			_movingSlide = true;
		}
		else if (eventname == "Wand_Joystick_Press_Up") {
			_movingSlide = false;
			addKeyEvent("ReleaseHolds");
		}
		//These are local trigger events and only affect the trigger
		else if (eventname == rcontrol + "_Trigger1") {
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

		//Trackpads are only important for getting the right button press. They do not need to be syncronized
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
	}

	void handleGlobalEvents(const VRDataIndex &event) {
		std::string eventname = event.getName();
		if (eventname == "KbdU_Down") {
			_pm->cellSize += 100;
		}
		else if (eventname == "KbdI_Down") {
			_pm->cellSize -= 100;
		}
		else if (eventname == "KbdJ_Down") {
			_pm->cellLine += 0.00005;
		}
		else if (eventname == "KbdK_Down") {
			_pm->cellLine -= 0.00005;
		}
		else if (eventname == "KbdO_Down") {
			_pm->ripThreshold += 0.5;
		}
		else if (eventname == "KbdL_Down") {
			_pm->ripThreshold -= 0.5;
		}
		else if (eventname == "KbdF_Down") {
			_pm->thickinterval += 1;
		}
		else if (eventname == "KbdG_Down") {
			_pm->thickinterval -= 1;
		}
		else if (eventname == "KbdP_Down") {
			_pm->surfaceMode = ((_pm->surfaceMode + 1) % 5);
		}
		if (eventname == "KbdX_Down") {
			mode = Mode::STANDARD;
			printf("moving to standard mode\n");
		}
		if (eventname == "KbdZ_Down") {
			mode = Mode::SIMILARITY;
			printf("moving to cluster mode\n");
		}
		else if (eventname == "Kbd1_Down") {
			_pm->layers ^= LAYER1;
		}
		else if (eventname == "Kbd2_Down") {
			_pm->layers ^= LAYER2;
		}
		else if (eventname == "Kbd3_Down") {
			_pm->layers ^= LAYER3;
		}
		else if (eventname == "Kbd4_Down") {
			_pm->layers ^= LAYER4;
		}	
		else if (eventname == "Kbd7_Down") {
			_pm->distMode = ((_pm->distMode + 1) % 4);
		}
		else if (eventname == "KbdV_Down") {
			_fmv.showOldModel = !_fmv.showOldModel;
		}
		else if (eventname == "KbdB_Down") {
			_fmv.showStained = !_fmv.showStained;
		}
		else if (eventname == "KbdN_Down") {
			_pm->distFalloff += 0.002;
		}
		else if (eventname == "KbdM_Down") {
			_pm->distFalloff -= 0.002;
		}
		else if (eventname == "KbdS_Down") {
			_pm->cutOnOriginal = !_pm->cutOnOriginal;
		}
		else if (eventname == "KbdT_Down") {
			loadStuff1();
		}
		else if (eventname == "KbdI_Down") {
			_pm->pathlineMin += 0.05;
		}
		else if (eventname == "KbdK_Down") {
			_pm->pathlineMin -= 0.05;
		}
		else if (eventname == "KbdU_Down") {
			_pm->pathlineMax += 0.05;
		}
		else if (eventname == "KbdJ_Down") {
			_pm->pathlineMax -= 0.05;
		}
		else if (eventname == "KbdR_Down") {
			loadStuff2();
		}
		else if (eventname == "KbdW_Down") {
			setPreset(currentPreset - 1);
		}
		else if (eventname == "KbdE_Down") {
			setPreset(currentPreset + 1);
		}
		else if (eventname == "KbdQ_Down") {
			reset();
		}
		else if (eventname == "Mouse_Up_Down") {
			if (mode == Mode::STANDARD) {
				_fmv.showStained = !_fmv.showStained;
			}
		}
		else if (eventname == "Mouse_Down_Down") {
			_fmv.sweepMode = ((_fmv.sweepMode + 1) % 6);
		}
		else if (eventname == "Mouse_Left_Down") {
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
		else if (eventname == "Mouse_Right_Down") {
			_fmv.showOldModel = !_fmv.showOldModel;
		}	
		else if (eventname == "KbdDown_Down" ) {
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
		else if (eventname == "KbdUp_Down") {
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
		else if (eventname == "KbdLeft_Down") {
			if (mode == Mode::SLICES) {
				_slicer->addGap(-0.0025);
				_pm->SetFilter(_slicer);
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
				reverse_hold = true;
			}
		}
		else if (eventname == "KbdRight_Down" ) {
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
				forward_hold = true;
			}
		}
		else if (eventname == "ReleaseHolds") {
			reverse_hold = false;
			forward_hold = false;
		}

		//Events to start and stop the animation - these are syncronized
		else if (eventname == "Animation") {
			int frame = event.getValue("Frame");
			int pausing = event.getValue("Pause");
			ac.setFrame(frame);
			if (pausing) {
				ac.pause();
			}
			else {
				ac.play();
			}
		}
		//_omw. This is being updated if someone moves the omw locally
		else if (eventname == "OMWPose") {
			const std::vector<float> *v = event.getValue("Transform");
			_owm = glm::make_mat4(&(v->front()));
		}
		//_slideMat. This is being updated if someone moves the _slideMat locally
		else if (eventname == "SlideMat") {
			const std::vector<float> *v = event.getValue("Transform");
			_slideMat = glm::make_mat4(&(v->front()));
		}
		//cuttingPlane. This is being updated if someone moves the cuttingPlane locally
		else if (eventname == "CuttingPlane") {
			const std::vector<float> *v = event.getValue("Vector");
			cuttingPlane = glm::make_vec4(&(v->front()));
		}
	}

	void setElapsedSeconds(float elapsedSeconds) {
		secs = elapsedSeconds;
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

	void replaceEventsForWandAndController(std::string eventname) {
		if (trackPadPressed(eventname,true, UP)) {
			addKeyEvent("Mouse_Up_Down");
		}
		else if (trackPadPressed(eventname, true, DOWN)) {
			addKeyEvent("Mouse_Down_Down");
		}
		else if (trackPadPressed(eventname, true, LEFT)) {
			addKeyEvent("Mouse_Left_Down");
		}
		else if (trackPadPressed(eventname, true, RIGHT)) {
			addKeyEvent("Mouse_Right_Down");
		}
		else if (eventname == "Wand_Down_Down" || trackPadPressed(eventname, false, DOWN)) {
			addKeyEvent("KbdDown_Down");
		}
		else if (eventname == "Wand_Up_Down" || trackPadPressed(eventname, false, UP)) {
			addKeyEvent("KbdUp_Down");
		}
		else if (eventname == "Wand_Left_Down" || trackPadPressed(eventname, false, LEFT)) {
			addKeyEvent("KbdLeft_Down");
		}
		else if (eventname == "Wand_Right_Down" || trackPadPressed(eventname, false, RIGHT)) {
			addKeyEvent("KbdRight_Down");
		}
		else if (eventname == "Mouse_Right_Click_Down" || eventname == rcontrol + "_GripButton_Down") {
			addKeyEvent("Kbd6_Down");
		}
		else if (eventname == "Wand_Joystick_Press_Up" || eventname == rcontrol + "_Axis0Button_Up") {
			addKeyEvent("Wand_Joystick_Press_Up");
		}
	}

	// Up == 1 , Down == 2 , Left == 3, Right == 4
	bool trackPadPressed(std::string eventname, bool left, TRACKPAD_DIRECTION direction) {
		float x, y,x2,y2, trackPad_time;

		if (!((left && eventname == lcontrol + "_Axis0Button_Down") 
			||(!left && eventname == rcontrol + "_Axis0Button_Down")))
			return false;

		if (left) {
			x = left_trackPad0_X;
			y = left_trackPad0_Y;	
			trackPad_time = left_trackPad0_time;
		}
		else {
			x = right_trackPad0_X;
			y = right_trackPad0_Y;
			trackPad_time = right_trackPad0_time;
		}

		if (secs - right_trackPad0_time >= trackpad_delay)
			return false;

		x2 = x * x;
		y2 = y * y;

		if (x2 + y2 < (0.33*0.33))
			return CENTER == direction;

		//up - down
		if (x2 < y2) {
			if (y > 0){
				return UP == direction;
			}
			else {
				return DOWN == direction;
			}
		}
		else { //left - right
			if (x < 0) {
				return LEFT == direction;
			}
			else {
				return RIGHT == direction;
			}
		}
	
	}

	// Callback for event handling, inherited from VREventHandler
	void onVREvent(const VRDataIndex &event) {
		//checking Frame timing - not syncronized
		if (event.getName() == "FrameStart") {
			setElapsedSeconds(event.getValue("ElapsedSeconds"));
		}

		//Global events - these are syncronized and should be whitelisted for sending all users
		handleGlobalEvents(event);

		//These are events which determine rendering settings or enable or disable some modes which set the matrices or vertices
		//They should not be whitelisted
		//e.g. _moving, _movingSlide or _slice_mode .In this way they can be set be multiple participants.
		//The matrices are then seperately set in the SlideMat, OMWPose or CuttingPlane events to update it for all participants if required
		handleLocalEvents(event);

		//event to set the different matrices - Syncronized
		//_currWandPos. This can so far only be set by one user and is sent out every loop
		//The reason is that it is not clear which events need it and who should control it
		if (event.getName() == "WandPose") {
			const std::vector<float> *v = event.getValue("Transform");
			_currWandPos = glm::make_mat4(&(v->front()));
		}
		//local wand event - does not need to be syncronized, but emits an events if it does require syncronization :  _moving, _movingSlide,_slice_mode
		//it currently emits WandPose for every loop. This should only be whitelisted for one user
		else if (event.getName() == "Wand0_Move" || event.getName() == rcontrol + "_Move") { //Set up for Wand Movement
			std::vector<float> h = event.getValue("Transform");
			glm::mat4 wandPos = glm::mat4(h[0], h[1], h[2], h[3],
				h[4], h[5], h[6], h[7],
				h[8], h[9], h[10], h[11],
				h[12], h[13], h[14], h[15]);
			if (_moving) {
				_owm = wandPos / _lastWandPos * _owm; // update the model matrix for the data points
				addTransformEvent("OMWPose", glm::value_ptr(_lastWandPos));
				//event to update _owm
			}
			if (_movingSlide) {
				_slideMat = wandPos / _lastWandPos * _slideMat; //update the model matrix for slide
				//event to update _slideMat
				addTransformEvent("SlideMat", glm::value_ptr(_lastWandPos));
			}
			_cursorMat = wandPos;
			_lastWandPos = wandPos;

			if (_slice_mode == SLICE) {
				glm::vec3 up = glm::vec3(_currWandPos[1]);
				up = glm::normalize(up);
				glm::vec3 modelUp = glm::inverse(getModelMatrix()) * glm::vec4(up, 0.0);
				float offset = -glm::dot(glm::vec3(getModelPos()), modelUp);
				glm::vec4 cuttingPlane_tmp = glm::vec4(modelUp.x, modelUp.y, modelUp.z, offset);
				addVectorEvent("CuttingPlane", glm::value_ptr(cuttingPlane_tmp));
			}
			
			//This is the biggest problem so far - when and who is allowed to send the WandPose
			//or when should it be local?
			addTransformEvent("WandPose", glm::value_ptr(_lastWandPos));
		}
	}

	virtual void onVRRenderContext(const VRDataIndex &renderState) {
		time = ac.getFrame(); //update time frame

		if ((int)renderState.getValue("InitRender") == 1) {
			//setting up
			glewExperimental = GL_TRUE;
			glewInit();
			const unsigned char* s = glGetString(GL_VERSION);
			printf("GL Version %s.\n", s);
			_pm->SetupDraw();
			_cursor.Initialize(1.0f);
			//  _board.Initialize("numbers.png", glm::vec3(2,-2,0), glm::vec3(0, 0,-0.55*2), glm::vec3(0,0.65*2,0));
			_board.Initialize("numbers.png", glm::vec3(-5, 2, -3), glm::vec3(0.55*0.7, 0, 0), glm::vec3(0, 0.65*0.7, 0));
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
		}
	}

	int count;

	glm::mat4 getModelMatrix() {
		glm::mat4 scale = glm::scale(glm::mat4(), glm::vec3(_scale, _scale, _scale));
		glm::mat4 translate = glm::translate(glm::mat4(), glm::vec3(1, -1, 0));
		//M is identity matrix. Or even worse depending on glm version not defined.
		//I am removing it.
		//glm::mat4 M = _owm * M * translate * scale;
		//glm::mat4 MVP = P * V * M;
		glm::mat4 M = _owm * translate * scale;
		return M;
	}

	//This is not the ModelPos even if it is named like this
	//I think this is the Wandposition in Modelcioordinates
	glm::vec4 getModelPos() {
		glm::mat4 newWandPos = glm::translate(_currWandPos, glm::vec3(0.0f, 0.0f, -1.0f));
		glm::vec3 location = glm::vec3(newWandPos[3]);
		glm::vec4 modelPos = glm::inverse(getModelMatrix()) * glm::vec4(location, 1.0);
		return modelPos;
	}


	// Callback for rendering, inherited from VRRenderHandler
	void onVRRenderScene(const VRDataIndex &stateData) {
		glCheckError();
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
		glCheckError();

		glm::mat4 V, P;
		
		// NOTE: I removed the normal camera as it was not usable. 
		// For desktop it is recommended to use the minvr way to control the camera 
		//In that way it always contains a ProjectionMatrix and a ViewMatrix

		// This is the typical case where the MinVR DisplayGraph contains
		// an OffAxisProjectionNode or similar node, which sets the
		// ProjectionMatrix and ViewMatrix based on head tracking data.
		const std::vector<float> *p = stateData.getValue("ProjectionMatrix");
		P = glm::make_mat4(&(p->front()));

		const std::vector<float> *v = stateData.getValue("ViewMatrix");
		V = glm::make_mat4(&(v->front()));

		// In the original adaptee.cpp program, keyboard and mouse commands
		// were used to adjust the camera.  Now that we are creating a VR
		// program, we want the camera (Projection and View matrices) to
		// be controlled by head tracking.  So, we switch to having the
		// keyboard and mouse control the Model matrix.  Guideline: In VR,
		// put all of the "scene navigation" into the Model matrix and
		// leave the Projection and View matrices for head tracking.	

		glm::mat4 MVP = P * V *  getModelMatrix();
		glm::mat4 t = V;

		//These are basically updates of the scene and should not be here as
		//this is not dependent on the eyeposition. These should either go to the OnRenderContext or even to the events
		{
			//These parts are all dependent on the modelPos
			//It should be thought of when these need updating. Who can control it - Global state vs local visualization
			//right now this limits the control of these to 1 user as it needs to resend the WandPose for every loop

			glm::vec3 modelPos = getModelPos();
			int id = _pm->TempPathline(modelPos, time);

			_board.LoadID(id);
			_board.LoadNumber(_similarityCount);
			if (mode == Mode::SIMILARITY) {
				_board.LoadID(id);
				_board.LoadNumber(_similarityCount);
			}
			//this code will never be reached as _placePathline is always false
			//if (_placePathline) {
			//	_placePathline = false;
			//	if (mode == Mode::PREDICT) {
			//		_pm->ShowCluster(modelPos, time);
			//	}
			//	else {
			//		_pm->AddPathline(modelPos, time);
			//	}
			//}
			if (_placeClusterSeed) {
				_pm->ShowCluster(modelPos, time);
			}
			//These are basically actions which are executed once. 
			//It would be better if they are triggered somewhere else when the action starts
			if (_similarityGo) {
				_similarityGo = false;
				_pm->FindClosestPoints(glm::vec3(modelPos), time, (int)_similarityCount);
			}
			if (_similarityPreset) {
				_similarityPreset = false;
				_pm->FindClosestPoints(_similarityId, (int)_similarityCount);
			}
		}

		//Basic Drawing
		glCheckError();

		_pm->Draw(time, MVP, V*getModelMatrix(), P, cuttingPlane);

		if (showFoot) {
			_fmv.Draw(time, MVP);
		}
		glCheckError();

		glm::mat4 m = P * V * _cursorMat; // _cursorMat is basically the last_wandPos
		_cursor.Draw(m);

		glCheckError();
	}

	void run() {
		while (!_quit) {
			_vrMain->mainloop();
		}
	}

	void appendNewInputEventsSinceLastCall(VRDataQueue* queue) {
		for (int f = 0; f < _events_out.size(); f++)
		{
			queue->push(_events_out[f]);
		}
		_events_out.clear();
	}


protected:
	VRMain *_vrMain;
	bool _quit;
	PointManager* _pm;
	int frame;
	Mode mode;
	float motionThreshold = 0.01;
	AnimationController ac;
	bool _moving = false;
	bool _placePathline = false;
	bool _placeClusterSeed = false;
	SliceFilter* _slicer;
	glm::mat4 _owm, _owmDefault;
	glm::mat4 _lastWandPos;
	glm::mat4 _currWandPos;
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

	SLICEMODE _slice_mode = NO_SLICE;

	std::unique_ptr<Config> _config;
	float _scale = 40;
	glm::vec4 cuttingPlane;
	float left_trackPad0_X = 0.0f, left_trackPad0_Y = 0.0f, left_trackPad0_time = 0.0f, right_trackPad0_X = 0.0f, right_trackPad0_Y = 0.0f, right_trackPad0_time = 0.0f;
	float secs;
	float trackpad_delay = 0.2f;
	bool rAssignedLast = false;

	std::vector<std::string> presets;
	int currentPreset = 0;

	std::vector<VRDataIndex> _events_out;
};



int main(int argc, char **argv) {
	MyVRApp app(argc, argv);
	app.run();

	return 0;
}

