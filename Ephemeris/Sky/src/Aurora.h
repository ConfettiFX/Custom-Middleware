/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Ephemeris.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#pragma once

#include "../../../../The-Forge/Common_3/Utilities/ThirdParty/OpenSource/Nothings/stb_ds.h"

#include "../../../../The-Forge/Common_3/Graphics/Interfaces/IGraphics.h"

#include "../../../../The-Forge/Common_3/Utilities/Math/MathTypes.h"

/* Some physics constants */
const float DAMPING = 0.01f;            // how much to damp the cloth simulation each frame
const uint  CONSTRAINT_ITERATIONS = 15; // how many iterations of constraint satisfaction each frame (more is rigid, less is soft)

class AuroraParticle
{
public:
    AuroraParticle(vec3 position):
        position(position), prevPosition(position), acceleration(vec3(0.0f, 0.0f, 0.0f)), mass(1.0f), IsMovable(true)
    {
    }
    AuroraParticle():
        position(vec3(0.0f, 0.0f, 0.0f)), prevPosition(vec3(0.0f, 0.0f, 0.0f)), acceleration(vec3(0.0f, 0.0f, 0.0f)), mass(1.0f),
        IsMovable(true)
    {
    }

    void addForce(const vec3& force) { acceleration += force / mass; }

    void update(float deltaTime)
    {
        if (IsMovable)
        {
            vec3 temporaryPosition = position;
            position = position + (position - prevPosition) * (1.0f - DAMPING) + acceleration * deltaTime;
            prevPosition = temporaryPosition;
        }

        acceleration =
            vec3(0.0f, 0.0f,
                 0.0f); // acceleration is reset since it HAS been translated into a change in position (and implicitely into velocity)
    }

    inline void resetAcceleration() { acceleration = vec3(0.0f, 0.0f, 0.0f); }
    inline void offsetPosition(const vec3& v)
    {
        if (IsMovable)
            position += v;
    }
    inline void makeUnmovable() { IsMovable = false; }

    /*
    void addToNormal(vec3 normal)
    {
      accumulated_normal += normalize(normal);
    }
    */

    vec3  position;     // the current position of the particle in 3D space
    vec3  prevPosition; // the position of the particle in the previous time step, used as part of the verlet numerical integration scheme
    vec3  acceleration; // a vector representing the current acceleration of the particle
    float mass;         // the mass of the particle (is always 1 in this example)
    bool  IsMovable;
    // vec4 accumulated_normal; // an accumulated normal (i.e. non normalized), used for OpenGL soft shading
};

typedef AuroraParticle* AuroraParticleStbDsArray;

class AuroraConstraint
{
private:
    float rest_distance; // the length between particle p1 and p2 in rest configuration

public:
    AuroraParticle *p1, *p2; // the two particles that are connected through this constraint

    AuroraConstraint(AuroraParticle* p1, AuroraParticle* p2): p1(p1), p2(p2)
    {
        vec3 gap = vec3(p1->position - p2->position);
        rest_distance = length(gap);
    }

    /* This is one of the important methods, where a single constraint between two particles p1 and p2 is solved
    the method is called by Cloth.time_step() many times per frame*/
    void satisfyConstraint()
    {
        vec3  gap = p2->position - p1->position;
        float current_distance = length(gap);
        vec3  correctionVector = gap * (1.0f - rest_distance / current_distance) *
                                0.5f; // The offset vector that could moves p1 into a distance of rest_distance to p2

        p1->offsetPosition(correctionVector);  // correctionVectorHalf is pointing from p1 to p2, so the length should move p1 half the
                                               // length needed to satisfy the constraint.
        p2->offsetPosition(-correctionVector); // we must move p2 the negative direction of correctionVectorHalf since it points from p2 to
                                               // p1, and not p1 to p2.
    }
};

typedef AuroraConstraint* AuroraConstraintStbDsArray;

class Aurora
{
public:
    Aurora(): numOfWidth(0), numOfHeight(0) {}

    void Init(float width, float height, uint32_t num_particles_width, uint32_t num_particles_height)
    {
        numOfWidth = num_particles_width;
        numOfHeight = num_particles_height;

        arrsetlen(particles, num_particles_width * num_particles_height); // I am essentially using this vector as an array with room for
                                                                          // num_particles_width*num_particles_height particles

        // creating particles in a grid of particles from (0,0,0) to (width,-height,0)
        for (uint32_t x = 0; x < num_particles_width; x++)
        {
            for (uint32_t y = 0; y < num_particles_height; y++)
            {
                vec3 pos = vec3(width * ((float)x / (float)num_particles_width), height * ((float)y / (float)num_particles_height), 0.0f);
                particles[y * num_particles_width + x] = AuroraParticle(pos); // insert particle in column x at y'th row
            }
        }

        // Connecting immediate neighbor particles with constraints (distance 1 and sqrt(2) in the grid)
        for (uint32_t x = 0; x < num_particles_width; x++)
        {
            for (uint32_t y = 0; y < num_particles_height; y++)
            {
                if (x < num_particles_width - 1)
                    makeConstraint(getParticle(x, y), getParticle(x + 1, y)); //  ---
                                                                              /*
                                                                              if (y < num_particles_height - 1)
                                                                                 makeConstraint(getParticle(x, y), getParticle(x, y + 1));     //   |
                                                                              if (x < num_particles_width - 1 && y < num_particles_height - 1)
                                                                                 makeConstraint(getParticle(x, y), getParticle(x + 1, y + 1)); //   \
                                                                              if (x < num_particles_width - 1 && y < num_particles_height - 1)
                                                                                 makeConstraint(getParticle(x + 1, y), getParticle(x, y + 1)); //   /
                                                                              */
            }
        }

        // Connecting secondary neighbors with constraints (distance 2 and sqrt(4) in the grid)
        /*
        for (uint32_t x = 0; x < num_particles_width; x++)
        {
          for (uint32_t y = 0; y < num_particles_height; y++)
          {
            if (x < num_particles_width - 2)
               makeConstraint(getParticle(x, y), getParticle(x + 2, y));
            if (y < num_particles_height - 2)
               makeConstraint(getParticle(x, y), getParticle(x, y + 2));
            if (x < num_particles_width - 2 && y < num_particles_height - 2)
               makeConstraint(getParticle(x, y), getParticle(x + 2, y + 2));
            if (x < num_particles_width - 2 && y < num_particles_height - 2)
               makeConstraint(getParticle(x + 2, y), getParticle(x, y + 2));
          }
        }
        */

        // making the upper left most three and right most three particles unmovable
        /*
        for (uint32_t i = 0; i < 3; i++)
        {
          getParticle(0 + i, 0)->offsetPosition(vec3(0.5f, 0.0f, 0.0f)); // moving the particle a bit towards the center, to make it hang
        more natural - because I like it ;) getParticle(0 + i, 0)->makeUnmovable();

          getParticle(num_particles_width - 1 - i, 0)->offsetPosition(vec3(-0.5f, 0.0f, 0.0f)); // moving the particle a bit towards the
        center, to make it hang more natural - because I like it ;) getParticle(num_particles_width - 1 - i, 0)->makeUnmovable();
        }
        */
    }

    AuroraParticle* getParticle(uint32_t x, uint32_t y) { return &particles[y * numOfWidth + x]; }

    void makeConstraint(AuroraParticle* p1, AuroraParticle* p2) { arrpush(constraints, AuroraConstraint(p1, p2)); }

    void update(float deltaTime)
    {
        for (uint32_t i = 0; i < CONSTRAINT_ITERATIONS; i++) // iterate over all constraints several times
        {
            for (size_t i = 0; i < arrlenu(constraints); ++i)
            {
                constraints[i].satisfyConstraint(); // satisfy constraint.
            }
        }

        for (size_t i = 0; i < arrlenu(particles); ++i)
        {
            particles[i].update(deltaTime); // calculate the position of each particle at the next time step.
        }
    }

    /* used to add gravity (or any other arbitrary vector) to all particles*/
    void addForce(const vec3& direction)
    {
        for (size_t i = 0; i < arrlenu(particles); ++i)
        {
            particles[i].addForce(direction); // add the forces to each particle
        }
    }

    vec3 calcTriangleNormal(AuroraParticle* p1, AuroraParticle* p2, AuroraParticle* p3)
    {
        vec3 pos1 = p1->position;
        vec3 pos2 = p2->position;
        vec3 pos3 = p3->position;

        vec3 v1 = pos2 - pos1;
        vec3 v2 = pos3 - pos1;

        return normalize(cross(v1, v2));
    }

    /* A private method used by windForce() to calcualte the wind force for a single triangle
      defined by p1,p2,p3*/
    void addWindForcesForTriangle(AuroraParticle* p1, AuroraParticle* p2, AuroraParticle* p3, const vec3& direction)
    {
        vec3 normal = calcTriangleNormal(p1, p2, p3);
        vec3 d = normalize(normal);
        vec3 force = normal * (dot(d, direction));
        p1->addForce(force);
        p2->addForce(force);
        p3->addForce(force);
    }

    void addWindForces(AuroraParticle* p1, const vec3& direction) { p1->addForce(direction); }

    /* used to add wind forces to all particles, is added for each triangle since the final force is proportional to the triangle area as
     * seen from the wind direction*/
    void windForce(const vec3& direction)
    {
        for (uint32_t x = 0; x < numOfWidth; x++)
        {
            for (uint32_t y = 0; y < numOfHeight; y++)
            {
                addWindForces(getParticle(x, y), direction);
                // addWindForcesForTriangle(getParticle(x + 1, y), getParticle(x, y), getParticle(x, y + 1), direction);
                // addWindForcesForTriangle(getParticle(x + 1, y + 1), getParticle(x + 1, y), getParticle(x, y + 1), direction);
            }
        }
    }

    uint32_t numOfWidth;
    uint32_t numOfHeight;

    AuroraParticleStbDsArray   particles = NULL;   // all particles that are part of this cloth
    AuroraConstraintStbDsArray constraints = NULL; // alle constraints between particles as part of this cloth
};
