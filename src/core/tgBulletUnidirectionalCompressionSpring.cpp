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
 * @file tgBulletUnidirectionalCompressionSpring.cpp
 * @brief Definitions of members of class tgBulletUnidirectionalCompressionSpring
 * @author Drew Sabelhaus, Brian Mirletz, et al.
 * @copyright Copyright (C) 2016 NASA Ames Research Center
 * $Id$
 */

// This module
#include "tgBulletUnidirectionalCompressionSpring.h"
#include "tgBulletSpringCableAnchor.h"
#include "tgCast.h"
// The BulletPhysics library
#include "BulletDynamics/Dynamics/btRigidBody.h"

#include <iostream>
#include <stdexcept>

/**
 * The main constructor for this class
 * In comparison to the tgSpringCable vs. tgBulletSpringCable class inheritance,
 * there is no need for the tgCast here, since the same type of anchor is used
 * in both tgBulletCompressionSpring and tgBulletUnidirectionalCompressionSpring.
 */
tgBulletUnidirectionalCompressionSpring::tgBulletUnidirectionalCompressionSpring(
		const std::vector<tgBulletSpringCableAnchor*>& anchors,
		bool isFreeEndAttached,
                double coefK,
                double coefD,
                double restLength,
		btVector3 * direction) :
tgBulletCompressionSpring(anchors, isFreeEndAttached, coefK, coefD, restLength),
m_direction(direction)
{
    // Since tgBulletCompressionSpring takes care of everything else,
    // just need to check that direction is valid, and then check the invariant.

  // @TO-DO: checks on the btVector3 direction.

    #if (1)
    std::cout << "tgBulletUnidirectionalCompressionSpring constructor, ";
    std::cout << "direction is: ";
    std::cout << "(" << m_direction->x() << ",";
    std::cout << m_direction->y() << ",";
    std::cout << m_direction->z() << ")" << std::endl;
    #endif
    
    assert(invariant());
}

// Destructor has to the responsibility of destroying the anchors also,
// as well as the btVector3 direction.
tgBulletUnidirectionalCompressionSpring::~tgBulletUnidirectionalCompressionSpring()
{
    #if (0)
    std::cout << "Destroying tgBulletUnidirectionalCompressionSpring" << std::endl;
    #endif
}

// The step function is what's called from other places in NTRT.
void tgBulletUnidirectionalCompressionSpring::step(double dt)
{
    if (dt <= 0.0)
    {
        throw std::invalid_argument("dt is not positive!");
    }

    calculateAndApplyForce(dt);

    // If the spring distance has gone negative, crash the simulator on purpose.
    // TO-DO: find a way to apply a hard stop here instead.
    if( getCurrentSpringLength() <= 0.0)
    {
      throw std::runtime_error("Compression spring has negative length, simulation stopping. Increase your stiffness coefficient.");
    }
    
    assert(invariant());
}

/**
 * Dot getCurrentAnchorDistance with m_direction.
 */
virtual const double getCurrentAnchorDistanceAlongDirection() const
{
  // btVector3 between the two anchors
  const btVector3 dist =
    anchor2->getWorldPosition() - anchor1->getWorldPosition();

  // Dot it with the direction of this spring, should return a double.
  double currAnchDistAlongDir = dist * m_direction;
  return currAnchDistAlongDir;
}

/**
 * Returns the current length of the spring. If isFreeEndAttached,
 * this can be either greater or less than m_restLength. If not, then spring
 * can only exist in compression (less than m_restLength).
 * This only calculates the distance in the stated direction.
 * TO-DO: start here on 2016-08-05.
 */
const double tgBulletUnidirectionalCompressionSpring::getCurrentSpringLength() const
{
    // initialize to the default value.
    // if the distance between the two anchors is larger
    // than the rest length of the spring, it means (intuitively) that
    // one end of the spring is not touching a rigid body.
    double springLength = getRestLength();

    // store this variable so we don't have multiple calls to the same function.
    double currAnchorDist = getCurrentAnchorDistance();

    if( isFreeEndAttached() )
    {
      // The spring length is always the distance between anchors, projected along
      // the direction vector.
      springLength = currAnchorDist;
    }
    else if( currAnchorDist < getRestLength() )
    {
      // the spring is not attached at its free end, but is in compression,
      // so there is a length change.
      springLength = currAnchorDist;
    }
    
    return springLength;
}

/**
 * Returns the current force in the spring.
 * If ~isFreeEndAttached, 
 * this is zero if the distance between anchors is larger than restLength.
 * Note that this does NOT include the force due to the damper, since that force
 * is only temporary and isn't reallyt "the force in the spring" per se.
 */
const double tgBulletUnidirectionalCompressionSpring::getSpringForce() const
{
    // Since the getCurrentSpringLength function already includes the check
    // against m_isFreeEndAttached:
    double springForce = - getCoefK() * (getCurrentSpringLength() - getRestLength());

    // Debugging
    #if (0)
    std::cout << "getCoefK: " << getCoefK() << " getCurrentSpringLength(): "
	      << getCurrentSpringLength() << " getRestLength: "
	      << getRestLength() <<std::endl;
    #endif
    
    // A negative delta_X should result in a positive force.
    // note that if m_isFreeEndAttached == false, then springForce >= 0 always,
    // since then the calculation above will have a (restlength - restLength) term.
    // quick check to be sure of that:
    if( !isFreeEndAttached())
    {
        assert(springForce >= 0.0);
    }
    
    return springForce;
}

/**
 * This is the method that does the heavy lifting in this class.
 * Called by step, it calculates what force the spring is experiencing,
 * and applies that force to the rigid bodies it connects to.
 */
void tgBulletUnidirectionalCompressionSpring::calculateAndApplyForce(double dt)
{

    // Create variables to hold the results of these computations
    btVector3 force(0.0, 0.0, 0.0);
    // the following ONLY includes forces due to K, not due to damping.
    double magnitude = getSpringForce();

    // hold this variable so we don't have to call the accessor function twice.
    const double currLength = getCurrentSpringLength();

    // Get the unit vector for the direction of the force, this is needed for
    const btVector3 unitVector = getAnchorDirectionUnitVector();

    // Calculate the damping force for this timestep.
    // Take an approximated derivative to estimate the velocity of the
    // tip of the compression spring:
    const double changeInDeltaX = currLength - m_prevLength;
    m_velocity = changeInDeltaX / dt;

    // The damping force for this timestep
    // Like with spring force, a positive velocity should result in a
    // force acting against the movement of the tip of the spring.
    m_dampingForce = - getCoefD() * m_velocity;

    // Debugging
    #if (0)
      std::cout << "tgBulletUnidirectionalCompressionSpring::calculateAndApplyForce  " << std::endl;
      std::cout << "Length: " << getCurrentSpringLength() << " rl: " << getRestLength() <<std::endl;
      std::cout << "SpringForce: " << magnitude << " DampingForce: " << m_dampingForce <<std::endl;
    #endif

    // Add the damping force to the force from the spring.
    magnitude += m_dampingForce;
    
    // Project the force into the direction of the line between the two anchors
    force = unitVector * magnitude; 
    
    // Store the calculated length as the previous length
    m_prevLength = currLength;
    
    //Now Apply it to the connected two bodies
    btVector3 point1 = this->anchor1->getRelativePosition();
    this->anchor1->attachedBody->activate();
    this->anchor1->attachedBody->applyImpulse(force*dt,point1);
    
    btVector3 point2 = this->anchor2->getRelativePosition();
    this->anchor2->attachedBody->activate();
    this->anchor2->attachedBody->applyImpulse(-force*dt,point2);
}

bool tgBulletUnidirectionalCompressionSpring::invariant(void) const
{
    return (m_coefK > 0.0 &&
    m_coefD >= 0.0 &&
    m_prevLength >= 0.0 &&
    m_restLength >= 0.0 &&
    anchor1 != NULL &&
    anchor2 != NULL &&
    m_direction != NULL &&
    m_anchors.size() >= 2);
}
