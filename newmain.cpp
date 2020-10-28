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

//The only tested and used modes for collaborative mode are STANDARD and SIMILARITY
//For the others I added the code back in and tried to add as much support for it. 
//But as i do not know how it should work and if it is still used code I did not test its integration

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
		_events_out.push_back(di);
	}

	void addVectorEvent(std::string eventname, float * vec) {
		VRDataIndex di(eventname);
		std::vector<float> t{ vec, vec + 4 };
		di.addData("Vector", t);
		_events_out.push_back(di);
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

	//This function was put back on Davids request and I am not sure if it still works as there was no data available to test
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
	//This function was put back on Davids request and I am not sure if it still works as there was no data available to test
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

			if (id > -1) {
				//I moved this function here as it is not dependent on any parameter in the drawing function
				_pm->FindClosestPoints(_similarityId, (int)_similarityCount);
			}
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
		std::string eventname = event.getName();

		//These events should be syncronized but we remap to syncronized ones 
		//This makes it easier as in some cases it requires more local states 
		//and it also helps to keep the whitelist smaller
		replaceEventsForWandAndController(eventname);

		//checking if left - right controller names have to be switched
		checkControllers(eventname);

		//instead of syncing the space key event we add an extra event to make sure it stops at the same frame
		if (eventname == "KbdSpace_Down" || eventname == "Wand_Select_Down" || eventname == lcontrol + "_GripButton_Down") {
			if (ac.isPlaying()) {
				addAnimationEvent(1);
			}
			else {
				addAnimationEvent(0);
			}
		}

		//This is the event to trigger the selection of the closest points.
		//It needed to be remapped from KbdLeft_Down
		if (mode == Mode::SIMILARITY && event.getName() == "KbdC_Down") {
			VRDataIndex di("FindClosestPoints");
			glm::vec4 wt = getWandtipInModel();
			float* vec = glm::value_ptr(wt);
			std::vector<float> t{ vec, vec + 4 };
			di.addData("wand", t);
			di.addData("time", time);
			di.addData("similarityCount", (int) _similarityCount);
			_events_out.push_back(di);
		}

		//Not tested. Not sure when this should be sent out
		if (event.getName() == "NOTSET") {
			VRDataIndex di("ShowCluster");
			glm::vec4 wt = getWandtipInModel();
			float* vec = glm::value_ptr(wt);
			std::vector<float> t{ vec, vec + 4 };
			di.addData("wand", t);
			di.addData("time", time);
			_events_out.push_back(di);
		}

		//Not tested. Not sure when this should be sent out
		if (event.getName() == "NOTSET") {
			VRDataIndex di("AddPathline");
			glm::vec4 wt = getWandtipInModel();
			float* vec = glm::value_ptr(wt);
			std::vector<float> t{ vec, vec + 4 };
			di.addData("wand", t);
			di.addData("time", time);
			_events_out.push_back(di);
		}

		//This is how someone can propagate its cursor and its wandtip
		if (event.getName() == rcontrol + "_ApplicationMenuButton_Down") {
			controlCursor = true;
			addKeyEvent("drawCursorOn");
		}
		else if (event.getName() == rcontrol + "_ApplicationMenuButton_Up") {
			controlCursor = false;
			addKeyEvent("drawCursorOff");
		}

		//This controls the wand movement and updates different matrices depending on the current mode.
		if (event.getName() == "Wand0_Move" || event.getName() == rcontrol + "_Move") { //Set up for Wand Movement
			std::vector<float> h = event.getValue("Transform");
			glm::mat4 new_wandPos = glm::mat4(h[0], h[1], h[2], h[3],
				h[4], h[5], h[6], h[7],
				h[8], h[9], h[10], h[11],
				h[12], h[13], h[14], h[15]);

			//if in moving mode we update the local view of the model (owm)
			if (_moving) {
				_owm = new_wandPos / _wandPos * _owm;
			}
			//This is not tested
			if (_movingSlide) {
				glm::mat4 sm = glm::inverse(_owm) * (new_wandPos / _wandPos * _slideMat); //update the model matrix for slide
				//event to update _slideMat
				addTransformEvent("SlideMat", glm::value_ptr(sm));
			}

			//if in slicing mode we update the slice transformation in model coordinates
			if (_slice_mode == SLICE) {
				glm::vec3 up = glm::vec3(glm::inverse(_owm) * new_wandPos[1]);
				up = glm::normalize(up);
				glm::vec3 modelUp = glm::inverse(getModelMatrix()) * glm::vec4(up, 0.0);
				float offset = -glm::dot(glm::vec3(getWandtipInModel()), modelUp);
				glm::vec4 cuttingPlane_tmp = glm::vec4(modelUp.x, modelUp.y, modelUp.z, offset);
				addVectorEvent("CuttingPlane", glm::value_ptr(cuttingPlane_tmp));
			}

			//we compute the wand position in model coordinates. This is the only time it needs to be computed as it only updates when the wand moves
			glm::vec4 wt = getWandtipInModel();

			//in case the cursor is not drawn by another user we keep our wandtip. Otherwise it will be set by the event WandTip
			if (!drawCursor) {
				_wandTip = wt;
			}
			//this user is controling the wand for all other users
			if (controlCursor) {
				//we therefore propagate the wandtip through an event
				addVectorEvent("WandTip", glm::value_ptr(wt));
				//and we also propagate the cursormatrix so that it can be drawn for all users
				glm::mat4 cp = glm::inverse(_owm) * _wandPos;
				addTransformEvent("CursorMat", glm::value_ptr(cp));
			}

			//we also update the wandpos for the current user
			_wandPos = new_wandPos;
		}

		//exiting the applicatiopn
		if (eventname == "KbdEsc_Down") {
			_quit = true;
		}
		//changing the slice_mode. This is a local mode.
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
		//moving the model. This is local only and does not move the model for other users
		else if (eventname == "MouseBtnLeft_Down" || eventname == "Wand_Bottom_Trigger_Down" || eventname == rcontrol + "_Axis1Button_Down") {
			_moving = true;
		}
		else if (eventname == "MouseBtnLeft_Up" || eventname == "Wand_Bottom_Trigger_Up" || eventname == rcontrol + "_Axis1Button_Up") {
			_moving = false;
		}
		//This is used to release the holds for playback. Playback should be implemented differently
		else if (eventname == "Wand_Joystick_Press_Down") {
			_movingSlide = true;
		}
		else if (eventname == "Wand_Joystick_Press_Up") {
			_movingSlide = false;
			addKeyEvent("ReleaseHolds");
		}
		//These are local trigger events and only affect the trigger
		//not sure what right_trigger is used for. Seems kind of complicated to toggle the _moving flag
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
		//Also not sure what they are used for. Seems kind of complicated as well.
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
			//_fmv.sweepMode = ((_fmv.sweepMode + 1) % 6);
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
		else if (eventname == "Kbd5_Down") {
			//_pm->SetShaders();
		}
		else if (eventname == "Kbd7_Down") {
			//_pm->colorPathsBySimilarity = !_pm->colorPathsBySimilarity;
			//_slicingHold = !_slicingHold;
			_pm->distMode = ((_pm->distMode + 1) % 4);
		}
		else if (eventname == "KbdV_Down") {
			//mode = Mode::SIMILARITY;
			_fmv.showOldModel = !_fmv.showOldModel;
			//_pm->clustering = true;
			//_pm->ClearPathlines();
			//mode = Mode::SLICES;
			//_pm->SetFilter(_slicer);
		}
		else if (eventname == "KbdB_Down") {
			//_fmv.showNewModel = !_fmv.showNewModel;
			_fmv.showStained = !_fmv.showStained;
		}
		else if (eventname == "KbdN_Down") {
			_pm->distFalloff += 0.002;
		}
		else if (eventname == "KbdM_Down") {
			//_fmv.showCloud = !_fmv.showCloud;
			_pm->distFalloff -= 0.002;
		}
		else if (eventname == "KbdS_Down") {
			_pm->cutOnOriginal = !_pm->cutOnOriginal;
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
		else if (eventname == "KbdW_Down") {
			setPreset(currentPreset - 1);
		}
		else if (eventname == "KbdE_Down") {
			setPreset(currentPreset + 1);
		}
		else if (eventname == "KbdQ_Down") {
			reset();
		}
		//Not sure if this works
		else if (eventname == "KbdT_Down") {
			//_pm->surfTrail = !_pm->surfTrail;
			loadStuff1();
		}
		//not sure if this works
		else if (eventname == "KbdR_Down") {
			loadStuff2();
		}
		else if (eventname == "Mouse_Up_Down") {
			//mode = Mode::STANDARD;
			//printf("moving to standard\n");
			if (mode == Mode::STANDARD) {
				_fmv.showStained = !_fmv.showStained;
			}
		}
		else if (eventname == "Mouse_Down_Down") {
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
		else if (eventname == "Mouse_Left_Down") {
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
				//_pm->SearchForSeeds();
				//This was removed by the events for ShowCluster and AddPathLine. It should be decided when they are being sent as a local event
				
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
			//This was removed by the events for ShowCluster and AddPathLine. It should be decided when they are being sent as a local event
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
				//This was replaced with the FindClosestPoints event
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

		//cuttingPlane. This is being updated if someone moves the cuttingPlane
		else if (eventname == "CuttingPlane") {
			const std::vector<float> *v = event.getValue("Vector");
			cuttingPlane = glm::make_vec4(&(v->front()));
		}

		//CursorMat. This is being updated when someone takes control over the cursor
		else if (eventname == "CursorMat") {
			const std::vector<float> *v = event.getValue("Transform");
			_cursorMat = glm::make_mat4(&(v->front()));
		}
		//WandTip. This is being updated when someone control over the cursor. If not the wandtip is updated through the local wand movement
		else if (event.getName() == "WandTip") {
			const std::vector<float> *v = event.getValue("Vector");
			_wandTip = glm::make_vec4(&(v->front()));
		}
		//FindClosestPoints. This is being used to find the closest points with the same input for all user
		else if (event.getName() == "FindClosestPoints") {
			const std::vector<float> *v = event.getValue("wand");
			glm::vec4 wt = glm::make_vec4(&(v->front()));
			float t = event.getValue("time");
			int count = event.getValue("similarityCount");
			_pm->FindClosestPoints(glm::vec3(wt), t, count);
			mode = Mode::STANDARD;
		}
		//This is not tested. It should be decided when the events are being sent out
		else if (event.getName() == "ShowCluster") {
			const std::vector<float> *v = event.getValue("wand");
			glm::vec4 wt = glm::make_vec4(&(v->front()));
			float t = event.getValue("time");
			_pm->ShowCluster(wt, t);
		}
		//This is not tested. It should be decided when the events are being sent out
		else if (event.getName() == "AddPathline") {
			const std::vector<float> *v = event.getValue("wand");
			glm::vec4 wt = glm::make_vec4(&(v->front()));
			float t = event.getValue("time");
			_pm->AddPathline(wt, t);
		}
		//This is not tested
		else if (eventname == "SlideMat") {
			const std::vector<float> *v = event.getValue("Transform");
			_slideMat = glm::make_mat4(&(v->front()));
		}
		//This signals that someone took control over the cursor and draws it for all users
		else if (event.getName() == "drawCursorOn") {
			drawCursor = true;
		}

		else if (event.getName() == "drawCursorOff") {
			drawCursor = false;
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

		//Global events - these events are received by all users and should be syncronized. They therefore should be whitelisted for the Photonplugin
		handleGlobalEvents(event);

		//These are events which determine local rendering settings or enable or disable modes, e.g. _moving, _movingSlide or _slice_mode. 
		//The matrices are then seperately set in the different events to update it for all participants if required
		handleLocalEvents(event);
	}

	virtual void onVRRenderContext(const VRDataIndex &renderState) {
		//This is called once per render cycle so we update the animation here
		time = ac.getFrame(); //update time frame

		//in case it runs the first time initialize glew and other things
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

	//This is just static and hardcided and sets a fixed translation and scale. 
	//This is globally the same for all users
	glm::mat4 getModelMatrix() {
		glm::mat4 scale = glm::scale(glm::mat4(), glm::vec3(_scale, _scale, _scale));
		glm::mat4 translate = glm::translate(glm::mat4(), glm::vec3(1, -1, 0));
		glm::mat4 M = translate * scale;
		return M;
	}

	//This returns the wand position for the current user in model coordinates
	glm::vec4 getWandtipInModel(){
		glm::mat4 newWandPos = glm::translate(glm::inverse(_owm) *_wandPos, glm::vec3(0.0f, 0.0f, -1.0f));
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

		// This is the typical case where the MinVR DisplayGraph contains
		// an OffAxisProjectionNode or similar node, which sets the
		// ProjectionMatrix and ViewMatrix based on head tracking data.
		const std::vector<float> *p = stateData.getValue("ProjectionMatrix");
		P = glm::make_mat4(&(p->front()));

		const std::vector<float> *v = stateData.getValue("ViewMatrix");
		V = glm::make_mat4(&(v->front()));

		//Here some info on the matrices
		//P = Projection
		//V = View in HMD coordinate system, e.g. due to head movement
		//_omw = position and orientation for the local user. Not the same for all users
		//getModelMatrix = global scale and translation for all users the same 
		glm::mat4 MVP = P * V * _owm * getModelMatrix();
		{
			//It should be reconsidered how the interaction works and how points are selected. Right now it works through the wandpos, but it might be better to just send the id.
			int id = _pm->TempPathline(_wandTip, time);

			//JN 20201019: We should consider to remove the _board visualization entirely.
			//It's the big number board that we implemented instead of adding actual 3D text rendering. It was a requested feature, but I don't think Steve and Morgan have ever used it to actually look up a particle ID during exploration.
			_board.LoadID(id);
			_board.LoadNumber(_similarityCount);

			if (mode == Mode::SIMILARITY) {
				_board.LoadID(id);
				_board.LoadNumber(_similarityCount);
			}

			//this code was never reached and is not needed anymore. 
			//It was removed by the events for ShowCluster and AddPathLine. 
			//It should be decided when they are being sent as a local event
			//Until then i leave the code in here as a clue. Do not uncomment
			/*if (_placePathline) {
				_placePathline = false;
				if (mode == Mode::PREDICT) {
					_pm->ShowCluster(modelPos, time);
				}
				else {
					_pm->AddPathline(modelPos, time);
				}
			}
			if (_placeClusterSeed) {
				_pm->ShowCluster(modelPos, time);
			}*/
		}

		glCheckError();

		//Here the model is drawn
		_pm->Draw(time, MVP, V * _owm * getModelMatrix(), P, cuttingPlane);

		if (showFoot) {
			_fmv.Draw(time, MVP);
		}
		glCheckError();

		//This draws the local cursor
		glm::mat4 m = P * V * _wandPos; 
		_cursor.Draw(m);

		//Not tested as i do not know how it is supposed to be used
		//glm::mat4 slideM = _slideMat * M;
		//_slide.Draw(P * V * _owm * _slideMat * getModelMatrix());

		//This draws the cursor in case another user took control over it
		if (drawCursor) {
			m = P * V * _owm *  _cursorMat; 
			_cursor.Draw(m);
		}

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
	SliceFilter* _slicer;
	glm::mat4 _owm, _owmDefault;
	glm::mat4 _wandPos;
	glm::mat4 _cursorMat = glm::mat4(1.0);
	bool _movingSlide = false;
	glm::mat4 _slideMat;
	bool drawCursor = false;
	bool controlCursor = false;
	FootMeshViewer _fmv;
	bool showFoot = true;
	WebUpdateReader* _wur;
	bool iterateClusters = false;
	float time = 0;
	Slide _slide;
	Textboard _board;
	Cursor _cursor;
	glm::vec3 _wandTip;
	double _similarityCount = 50;
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

