#include <vrg3d/VRG3D.h>
#include "pointmanager.h"
#include "animationcontroller.h"
#include "filter.h"

#define PROFILING
#undef PROFILING

#ifdef PROFILING
#include "gperftools/profiler.h"
#endif

using namespace G3D;

enum struct Mode { STANDARD, ANIMATION, FILTER, SLICES};

class DinoApp : public VRApp
{
public:
  DinoApp(std::string setup, std::string dataFile, std::string pathFile, std::string surfaceFile, bool showAllPaths = false) : VRApp(),
  ac()
  {
  
    std::string profFile = "/users/jtveite/d/prof/profile-" + setup;
#ifdef PROFILING
    ProfilerStart(profFile.c_str());
#endif

    std::string logName = "logs/dino-log-" + setup;
    Log *dinolog = new Log(logName);
    log = dinolog;
    init(setup, dinolog);
    _mouseToTracker = new MouseToTracker(getCamera(), 2);
    frameTime = clock();

//    pm.ReadFile("/users/jtveite/data/jtveite/slices-130.out");
//    pm.ReadFile("/users/jtveite/dinotrackviewer/test-data")
    dinolog->printf("startish\n");
    pm.ReadFile(dataFile, true);
    dinolog->printf("File read\n");
    pm.SetupDraw(showAllPaths);
    if (pathFile != "" && pathFile != "a"){
      pm.ReadPathlines(pathFile);
    }
    dinolog->printf("pathlines read\n");
    if (surfaceFile != "" && surfaceFile != "a"){
      pm.ReadSurface(surfaceFile);
    }
//    pm.ReadSurface("tris/active");
    dinolog->printf("surface read\n");
    Matrix3 scale = Matrix3::fromDiagonal(Vector3(20, 20, 20));
    CoordinateFrame scaleC (scale);
    CoordinateFrame rotate = CoordinateFrame::fromXYZYPRDegrees(
      2, 0, -6,
      -90, -90, 180);
    _owm = rotate * scale;
    ac.setFrameCount(pm.getLength());
    ac.setSpeed(15);

    //std::cout << GL_NO_ERROR << ' ' << GL_INVALID_ENUM << ' ' << GL_INVALID_VALUE << ' ' << GL_INVALID_OPERATION << ' ' << GL_INVALID_FRAMEBUFFER_OPERATION << std::endl;
    //
    targetTracker = "Wand_Tracker";
    if (setup == "" || setup == "desktop"){
      targetTracker = "Mouse1_Tracker";
    }

    _slicer = new SliceFilter();

  }
  virtual ~DinoApp() {
#ifdef PROFILING
    ProfilerFlush();
    ProfilerStop();
#endif
  }

  //general skeleton of user input from demo app
  void doUserInput(Array<VRG3D::EventRef> &events)
  {

    Array<VRG3D::EventRef> newEvents;
    _mouseToTracker->doUserInput(events, newEvents);
    events.append(newEvents);

    for (int i = 0; i < events.size(); i++) {

      std::string eventName = events[i]->getName();
      if (endsWith(events[i]->getName(), "_Tracker")) {
        if (_trackerFrames.containsKey(events[i]->getName())){
          _trackerFrames[events[i]->getName()] = events[i]->getCoordinateFrameData(); 
        }
        else {
          _trackerFrames.set(events[i]->getName(), events[i]->getCoordinateFrameData());
        }
      }
      else if (eventName == "B09_down" || eventName == "kbd_1_down"){
        mode = Mode::STANDARD;
      }
      else if (eventName == "B10_down" || eventName == "kbd_2_down"){
        //mode = Mode::ANIMATION;
        pm.showSurface = !pm.showSurface;
      }
      else if (eventName == "B11_down" || eventName == "kbd_3_down"){
        mode = Mode::FILTER;
        pm.SetFilter(new MotionFilter(motionThreshold));
      }
      else if (eventName == "B12_down" || eventName == "kbd_4_down"){
        //mode = Mode::SLICES;
        //pm.SetFilter(_slicer);
        _slicing = !_slicing;
      }
      else if(eventName == "Mouse_Left_Btn_down"){
        _moving = true;
      }
      else if(eventName == "Mouse_Left_Btn_up"){
        _moving = false;
      }
      else if (eventName == "Wand_Right_Btn_down"){
        _moving = true;
      }
      else if (eventName == "Wand_Right_Btn_up"){
        _moving = false;
      }
      else if (eventName == "Wand_Left_Btn_down"){
        _placePathline = true; 
      }
      else if (eventName == "kbd_Q_down"){
        _placePathline = true;
        printf("QQQQQQQ\n");
      }
      else if (eventName == "kbd_DOWN_down"){
        ac.decreaseSpeed();
      }
      else if (eventName == "kbd_UP_down"){
        ac.increaseSpeed();
      }
      else if (eventName == "B04_down"){
        // ac.decreaseSpeed();
        if (mode == Mode::STANDARD){
          pm.pointSize /= 1.3;
        }
        if (mode == Mode::ANIMATION){
          ac.decreaseSpeed();
        }
        if (mode == Mode::SLICES){
          _slicer->addStart(.001);
          pm.SetFilter(_slicer);
        }
        if (mode == Mode::FILTER){
          motionThreshold *= 1.414;
          pm.SetFilter(new MotionFilter(motionThreshold));
        }
      }
      else if (eventName == "B03_down"){
        if (mode == Mode::STANDARD){
          pm.pointSize *= 1.3;
        }
        if (mode == Mode::ANIMATION){
          ac.increaseSpeed();
        }
        if (mode == Mode::SLICES){
          _slicer->addStart(-.001);
          pm.SetFilter(_slicer);
        }
        if (mode == Mode::FILTER){
          motionThreshold /= 1.414;
          pm.SetFilter(new MotionFilter(motionThreshold));
        }
      }
      
      else if (eventName == "B05_down"){
        if (mode == Mode::STANDARD || mode == Mode::ANIMATION || mode == Mode::FILTER){
          ac.stepBackward();
        }
        if (mode == Mode::SLICES){
          _slicer->addGap(-.0025);
          pm.SetFilter(_slicer);
        }
      }
      else if (eventName == "B06_down"){
        if (mode == Mode::STANDARD || mode == Mode::ANIMATION || mode == Mode::FILTER){
          ac.stepForward();
        }
        if (mode == Mode::SLICES){
          _slicer->addGap(.0025);
          pm.SetFilter(_slicer);
        }
      }
      else if (eventName == "B07_down"){
        ac.togglePlay();
      }
      else if (eventName == "kbd_SPACE_down"){
        ac.togglePlay();
      }
      else if (eventName == "kbd_LEFT_down"){
        ac.stepBackward();
      }
      else if (eventName == "kbd_RIGHT_down"){
        ac.stepForward();
      }
      else if (eventName == "kbd_C_down"){
        pm.pointSize *= 1.3;
      }
      else if (eventName == "kbd_V_down"){
        pm.pointSize /= 1.3;
      }
      
/*
      else if (eventName == "B09_down"){
        pm.SetFilter(new Filter());
      }
      else if (eventName == "B10_down"){
        pm.SetFilter(new MotionFilter(motionThreshold));
      }
      else if (eventName == "B11_down" || eventName == "kbd_J_down"){
        motionThreshold *= 1.414;
        pm.SetFilter(new MotionFilter(motionThreshold));
      }
      else if (eventName == "B12_down" || eventName == "kbd_K_down"){
        motionThreshold /= 1.414;
        pm.SetFilter(new MotionFilter(motionThreshold));
      }
*/
      else if (eventName == "kbd_T_down"){
        pm.SetFilter(_slicer);
      }

      else if (eventName == "kbd_R_down"){
        _slicer->addStart(0.0025);
        pm.SetFilter(_slicer);
      }

      else if (eventName == "kbd_F_down"){
        _slicer->addGap(0.0025);
        pm.SetFilter(_slicer);
      }

      else if (eventName == "kbd_G_down"){
        _slicer->addSize(0.0025);
        pm.SetFilter(_slicer);
      }

      else if (beginsWith(eventName, "aimo")){}
      else if (eventName == "SynchedTime"){}
      else{
        //std::cout << eventName << std::endl;
      }



    }
    
  }
 

  void doGraphics(RenderDevice *rd)
  {
    log->print("start of graphics");
    _lastFrame = ac.getFrame();
    float t = _lastFrame;
    //std::cout << "Rendering frame: " << _frameCounter << std::endl;

    //Do rotation of trackers if needed
    //std::string targetTracker = "Mouse1_Tracker";
    //std::string targetTracker = "Wand_Tracker";
    Array<std::string> trackerNames = _trackerFrames.getKeys();
    Vector4 cuttingPlane;
    for (int i = 0; i < trackerNames.size(); i++){
      CoordinateFrame trackerFrame = _trackerFrames[trackerNames[i]];
      if (trackerNames[i] == targetTracker){
        if (_moving){
          CoordinateFrame amountMoved = trackerFrame * _lastTrackerLocation.inverse();
          _owm = amountMoved * _owm;
        }
        _lastTrackerLocation = trackerFrame;
        

        Vector3 location = trackerFrame.translation;
        Vector3 up = trackerFrame.upVector();
        //Draw::arrow(  location, up, rd, Color3::orange(), .1);

        Matrix4 owm = _owm.toMatrix4();
        Matrix4 transform = owm.inverse();
        location = (transform * Vector4(location, 1.0)).xyz();
        up = (transform * Vector4(up, 0.0)).xyz();

        float offset = -(up.x * location.x + up.y * location.y + up.z * location.z);
        if (_slicing){
          cuttingPlane = Vector4(up.x, up.y, up.z, offset);
        }

      }

    }
    if (_placePathline){
      _placePathline = false;
    //  CoordinateFrame oldSpace = _lastTrackerLocation * _owm.inverse();
     // Vector3 location = oldSpace.translation;
      Vector3 location = _lastTrackerLocation.translation;
      //location = _lastTrackerLocation.pointToWorldSpace(Vector3(0,0,0));
      Matrix4 owm = _owm.toMatrix4();
      Vector4 l = owm.inverse() * Vector4(location, 1.0);
      pm.AddPathline(l.xyz(), t);
      //std::cout << l.xyz() << std::endl;
      //printf("placing a pathline at %f, %f, %f\n", location.x, location.y, location.z);
      //printf("placing a pathline at %f, %f, %f\n", location.x, location.y, location.z);
    }

      Vector3 location = _lastTrackerLocation.translation;
      //location = _lastTrackerLocation.pointToWorldSpace(Vector3(0,0,0));
      Matrix4 owm = _owm.toMatrix4();
      Vector4 l = owm.inverse() * Vector4(location, 1.0);
      pm.TempPathline(l.xyz(), t);
    

    rd->pushState();
    //rd->setObjectToWorldMatrix(_virtualToRoomSpace);
    int gl_error;
    while((gl_error = glGetError()) != GL_NO_ERROR)
    {

      std::cout << "Flushing gl errors " << gl_error << std::endl;
    }
   
    rd->pushState();

    rd->setObjectToWorldMatrix(_owm);
    Matrix4 mvp = rd->invertYMatrix() * rd->modelViewProjectionMatrix();
    
    //std::cout << t << std::endl;
    log->print("Before draw\n");
    pm.Draw(rd, t, mvp, cuttingPlane);
    log->print("after draw\n");
    rd->popState();

    Vector3 hp = getCamera()->getHeadPos();
   // printf("Head position: %f, %f, %f\n", hp.x, hp.y, hp.z);
    Vector3 lv = getCamera()->getLookVec();
 
    //Draw a small ball at the tracker location.
    //
    rd->pushState();
    rd->setObjectToWorldMatrix(CoordinateFrame());
    Sphere s (_lastTrackerLocation.translation, 0.02);
    Draw::sphere(s, rd, Color3::red());
    rd->popState();

 //  printf("Look Vector: %f, %f, %f\n", lv.x, lv.y, lv.z);

    rd->popState();
    clock_t newTime = clock();
    frameTime = newTime;
  }

protected:
  PointManager pm;
  MouseToTrackerRef _mouseToTracker;
  CoordinateFrame _virtualToRoomSpace;
  CoordinateFrame _owm;
  CoordinateFrame _lastTrackerLocation;
  Table<std::string, CoordinateFrame> _trackerFrames;
  bool _moving;
  clock_t frameTime;
  AnimationController ac;
  int _lastFrame;
  bool _placePathline;
  std::string targetTracker;
  float motionThreshold = 0.01;
  SliceFilter* _slicer;
  Mode mode = Mode::STANDARD;
  bool _slicing;
  Log* log;
};

int main(int argc, char **argv )
{
  std::string setup;
  std::string dataFile;
  std::string pathsFile;
  std::string surfaceFile;
  bool showAllPaths = false;
  if (argc >= 2)
  {
    setup = std::string(argv[1]);
  }
  if(argc >= 3)
  {
    dataFile = std::string(argv[2]);
  }
  else{
    dataFile = "/users/jtveite/data/jtveite/dino/slices-68-trimmed.out";
  }
  if (argc >= 4)
  {
    pathsFile = std::string(argv[3]);
  }
  else{
    pathsFile = "";
  }
  if (argc >= 5)
  {
    surfaceFile = std::string(argv[4]); 
  }
  if (argc >= 6)
  {
    showAllPaths = true;
  }
    
  DinoApp *app = new DinoApp(setup, dataFile, pathsFile, surfaceFile, showAllPaths);
  app->run();
  return 0;

}
