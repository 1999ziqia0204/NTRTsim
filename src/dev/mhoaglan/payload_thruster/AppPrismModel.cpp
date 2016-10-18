/*
 * Copyright © 2012, United States Government, as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All rights reserved.
 * 
 * The NASA Tensegrity Robotics Toolkit (NTRT) v1 platform is licensed
 * under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0.
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
*/

/**
 * @file AppPrismModel.cpp
 * @brief Contains the definition function main() for the Three strut
 * tensegrity prism example application
 * @author Brian Tietz
 * $Id$
 */

// This application
#include "PrismModel.h"
#include "RPThruster.h"
// This library
//#include "core/terrain/tgBoxGround.h"
 #include "core/terrain/tgImportGround.h"
#include "core/tgModel.h"
#include "core/tgSimViewGraphics.h"
#include "core/tgSimulation.h"
#include "core/tgWorld.h"
// Bullet Physics
#include "LinearMath/btVector3.h"
// The C++ Standard Library
#include <iostream>
// C++ Filestream
#include <fstream>
// C++ String
#include <string>
// C++ Math
#include <math.h>

/**
 * The entry point.
 * @param[in] argc the number of command-line arguments
 * @param[in] argv argv[0] is the executable name
 * @return 0
 */
int main(int argc, char** argv)
{
    std::cout << "**--** Test Hinge Julien Despois **--**" << std::endl;

    // First create the ground and world. Specify ground rotation in radians
    const double yaw = 0.0;
    const double pitch = 0.0;
    const double roll = 0.0;
    //const tgImportGround::Config groundConfig(btVector3(yaw, pitch, roll));
    
    btVector3 orientation = btVector3(yaw, pitch, roll);

    // Set other ground parameters
    const double friction = 0.5;
    const double restitution = 0.0;
    btVector3 origin = btVector3(0.0, 0.0, 0.0);
    const double margin = 0.05;
    const double offset = 0.5;
    const double scalingFactor = 100;

    // Configure ground characteristics
    const tgImportGround::Config groundConfig(orientation, friction, restitution,
        origin, margin, offset, scalingFactor);

    // Get filename from argv
    std::string filename_in = argv[1];

    // Check filename
    if (filename_in.find(".txt") == std::string::npos) {
        std::cout << "Incorrect filetype, input file should be a .txt file" << std::endl;
        exit(EXIT_FAILURE);
    }

    //Create filestream
    std::fstream file_in;

    // Open filestream
    file_in.open(filename_in.c_str(), std::fstream::in);

    // Check if input file opened successfully
    if (!file_in.is_open()) {
        std::cout << "Failed to open input file" << std::endl;
        exit(EXIT_FAILURE);
    }
    else {
        std::cout << "Input file opened successfully" << std::endl;
    }

    // the world will delete this
    //tgBoxGround* ground = new tgBoxGround(groundConfig);
    tgImportGround* ground = new tgImportGround(groundConfig, file_in);

    /*
    // the world will delete this
    tgBoxGround* ground = new tgBoxGround(groundConfig);
    */
    
    const tgWorld::Config config(98.1); // gravity, cm/sec^2
    tgWorld world(config, ground);

    // Second create the view
    const double timestep_physics = 0.001; // seconds
    const double timestep_graphics = 1.f/60.f; // seconds
    tgSimViewGraphics view(world, timestep_physics, timestep_graphics); //turn on graphics
    //tgSimView view(world, timestep_physics, timestep_graphics); // turn off graphics for faster data logging

    // Third create the simulation
    tgSimulation simulation(view);

    // Fourth create the models with their controllers and add the models to the
    // simulation
    PrismModel* const myModel = new PrismModel();

    //Create Active Thruster
    RPThruster* const thrust_control = new RPThruster();
    myModel->attach(thrust_control);
    
    // Add the model to the world
    simulation.addModel(myModel);
    
    simulation.run(35000);

    //Teardown is handled by delete, so that should be automatic
    return 0;
}
