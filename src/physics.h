#ifndef PHYSICS
#define PHYSICS

#include "raylib.h"
#include "raymath.h"

class Plane {
    public:
    Vector3 position;
    Vector3 velocity;
    Vector3 front;
    Vector3 up;
    
    void update() {
        position = position + velocity;
    }
};


#endif