# This file is part of the DinoYURT project.  
# See LICENSE.txt for full authors, copyright, and license info.
#
# Original Author(s) of this File: 
#   Daniel Keefe, 2017, University of Minnesota
#  
# Author(s) of Significant Updates/Modifications to the File:
#   Johannes Novotny, 2018, Brown University

list(APPEND CMAKE_PREFIX_PATH ../MinVR/build/install)



# Using 3.9 to get a modern version of FindOpenGL.cmake
cmake_minimum_required (VERSION 3.9) 

# Dependencies that are auto-downloaded, built, and installed for you will go here.  So, you 
# may use this even if not planning to install this particular project on your system.
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR "${CMAKE_INSTALL_PREFIX}" STREQUAL "")
    set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "default install path" FORCE )
endif()

list(APPEND CMAKE_MODULE_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_INSTALL_PREFIX})
list(INSERT CMAKE_PREFIX_PATH 0 ${CMAKE_INSTALL_PREFIX})



#---------------------- Source for this Project ----------------------

project(DinoVR)

include(MessageMacros)
h1("Building ${PROJECT_NAME}")
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")



set(source_files
  newmain.cpp
  cursor.cpp
  slide.cpp
  textboard.cpp
  pointmanager.cpp
  vrpoint.cpp
  shader.cpp
  animationcontroller.cpp
  filter.cpp
  movm.cpp
  footmeshviewer.cpp
  webupdatereader.cpp
  SimilarityEvaluator.cpp
  PathAlignmentSimilarityEvaluator.cpp

  Config.cpp

  GLUtil.cpp
)

set(test_source_files
  slide.cpp
  pointmanager.cpp
  vrpoint.cpp
  shader.cpp
  animationcontroller.cpp
  filter.cpp
  movm.cpp
  footmeshviewer.cpp
  webupdatereader.cpp
  SimilarityEvaluator.cpp
  PathAlignmentSimilarityEvaluator.cpp
  GLUtil.cpp
)



set(filter_source_files
  pathfilter.cpp
  pointmanager.cpp
  vrpoint.cpp
  shader.cpp
  animationcontroller.cpp
  filter.cpp
  movm.cpp
  GLUtil.cpp
)

set(header_files
  pointmanager.h
  vrpoint.h
  slide.h
  cursor.h
  textboard.h
  shader.h
  animationcontroller.h
  filter.h
  pathfilter.h
  vertex.h
  footmeshviewer.h
  webupdatereader.h
  SimilarityEvaluator.h
  PathAlignmentSimilarityEvaluator.h

  Config.h
  GLUtil.h
)

set(resource_files
  numbers.png
  colormap.jpg
  pathmap.jpg
  desktop.xml
)

source_group("Header Files" FILES ${header_files})
source_group("Source Files" FILES ${source_files})

#set_source_files_properties(${extra_files} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(vertex.h GLUtil.h PROPERTIES HEADER_FILE_ONLY TRUE)

#---------------------- Define the Target ----------------------
#set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++14)

add_executable(${PROJECT_NAME} ${header_files} ${source_files})



#---------------------- Find & Add Dependencies ----------------------

set(EXTERNAL_DIR_NAME external)
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR_NAME})
set(EXTERNAL_CMAKE_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# MinVR (linked with an imported cmake target so no need to specify include dirs)
find_package(MinVR REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC MinVR::MinVR)


# OpenGL (cmake's FindOpenGL.cmake in version 3.9+ provides imported targets for OpenGL
# see FindOpenGL.cmake on cmake's website for details)
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenGL::GL)
if (${OPENGL_GLU_FOUND})
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenGL::GLU)
endif()


# GLM (GLM's installation provides a cmake config package with the imported target 'glm')
find_package(glm CONFIG)
if (NOT ${glm_FOUND})
    message(STATUS "Package not found (glm): Ok, will download, build, and install for you.")
    message(STATUS "Download dir: ${EXTERNAL_DIR}.")
    message(STATUS "Install dir: ${CMAKE_INSTALL_PREFIX}.")

    include(ExternalProjectConfig)
    ExternalProject_Download(
        glm 
        GIT_REPOSITORY "https://github.com/g-truc/glm.git" 
        GIT_TAG 0.9.9.6
    )
    ExternalProject_BuildAndInstallNow(
        glm
        src
    )

    message(STATUS "Searching (again, right after autobuilding) for GLM...")
    find_package(glm CONFIG)

    if (NOT ${glm_FOUND})
        message(FATAL_ERROR "GLM was autobuilt for you, and it should now be installed at the prefix ${CMAKE_INSTALL_PREFIX}, but cmake is still unable to find it with find_package().")
    endif()
endif()



# At this point, one or the other version of find_package() succeeded.
message(STATUS ${GLM_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC glm)
target_include_directories(${PROJECT_NAME} PUBLIC ${GLM_INCLUDE_DIRS})


if (NOT APPLE)
    # GLEW (GLEW's installation provides a cmake config package with the imported target GLEW::GLEW)
    find_package(GLEW)
    if (NOT ${GLEW_FOUND})
        message(STATUS "Package not found (GLEW): Ok, will download, build, and install for you.")
        message(STATUS "Download dir: ${EXTERNAL_DIR}.")
        message(STATUS "Install dir: ${CMAKE_INSTALL_PREFIX}.")

        include(ExternalProjectConfig)
        ExternalProject_Download(
            GLEW
            URL https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0.zip
        )
        ExternalProject_BuildAndInstallNow(
            GLEW
            src/build/cmake
        )

        message(STATUS "Searching (again, right after autobuilding) for GLEW...")
        find_package(GLEW)

        if (NOT ${GLEW_FOUND})
            message(FATAL_ERROR "GLEW was autobuilt for you, and it should now be installed at the prefix ${CMAKE_INSTALL_PREFIX}, but cmake is still unable to find it with find_package().")
        endif()
    endif()

    # At this point, one or the other version of find_package() succeeded.
    target_link_libraries(${PROJECT_NAME} PUBLIC GLEW::GLEW)
endif()

#file(COPY ${resource_files} DESTINATION ${CMAKE_BINARY_DIR}/bin)
#file(COPY data/active-dataset.out DESTINATION ${CMAKE_BINARY_DIR}/bin)
