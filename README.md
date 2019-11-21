# DinoVR
 A VR Viewer for fluid simulations of dinosaur track morphology.

## Quick Start Guide

### Prereqs
DinoVR is based on the MinVR framework (https://github.com/MinVR/MinVR) and requires it to be built and installed on your system. Detailed instructions on how to build MinVR for your platform can b4e found on the github page of the repository.

DinoVR is built using the cross-platform CMake tool, which is also used in the MinVR build process.

### Download, Configure, Build, and Run a particle visualization

Here's a complete annotated list of steps to display your first DinoVR visualization.

#### 1. Download the DinoVR Source Tree into the parent directory of your MinVR directory
```
git clone http://github.com/jonovotny/DinoVR
```

#### 2. Create an Initial CMake Build Configuration

```
cd DinoVR
mkdir build
cd build
cmake-gui ..
```

Press the 'Configure' button. Then, select the Generator you would like to use from the list provided (e.g., Xcode, Visual Studio, Unix Makefiles).  Click Done.

Wait for CMake to do an initial configuration. As part of this process Cmake will download and build the OpenGL Mathematics library (https://github.com/g-truc/glm), which might take a few minutes. 

#### 3. Generate the Build System / Project Files

After the configuration is completed, press the Generate button.  This is the step that will actually generate the Unix Makefiles, Visual Studio Solution File, or Xcode Project File needed to build DinoVR.  

#### 4. Build DinoVR with the Specified Options

Click Open Project if you generated project files for an IDE, or if you generated Unix Makefiles return to your shell and the build directory.  Now, build the project as you normally would in these enviornments.  

For Xcode or Visual Studio ```click the triangle button```.

For Unix Makefiles ```type make```.

#### 5. Run a Test Program in Desktop Mode

#### 6. Run in VR Mode