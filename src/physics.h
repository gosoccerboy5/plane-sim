#ifndef PHYSICS
#define PHYSICS

#include "raylib.h"
#include "raymath.h"

Vector3 rotateVectorAroundAxis(const Vector3& vector, Vector3 axis, double radians) {
    axis = Vector3Normalize(axis);
    double theta = Vector3Angle(axis, vector);
    double radius = Vector3Length(vector);
    Vector3 vertical = Vector3Scale(axis, radius * cos(theta));
    Vector3 flat = vector - vertical;
    Vector3 flatPerpendicular = Vector3CrossProduct(axis, flat);
    Vector3 resultant = Vector3Scale(flat, cos(radians)) + Vector3Scale(flatPerpendicular, sin(radians)) + vertical;
    return resultant;
}

class Plane {
    public:
    Vector3 position;
    Vector3 velocity;
    Vector3 front {0, 0, 1};
    Vector3 up {0, 1, 0};
    Model* model;
    float elevatorDeflection = 0;
    float aileronDeflection = 0; 
    float rudderDeflection = 0;
    
    Plane(Model* model): model(model) {
        velocity = {0, 0, 5};
        position = (Vector3){ 0.0f, 20.0f, 0.0f };
    }
    
    void normalizeOrientationVectors() {
        front = Vector3Normalize(front);
        up = Vector3Normalize(up);
    }
    void update() {
        velocity = Vector3Scale(front, Vector3Length(velocity));
        position = position + velocity;
        pitch(elevatorDeflection * 0.03);
        roll(aileronDeflection * .2);
        yaw(rudderDeflection * 0.01);
    }
    Vector3 right() const {
        return Vector3CrossProduct(front, up);
    }
    void roll(double angle) {
        up = rotateVectorAroundAxis(up, front, angle);
        normalizeOrientationVectors();
    }
    void pitch(double angle) {
        Vector3 rightVector = right();
        front = rotateVectorAroundAxis(front, rightVector, angle);
        up = rotateVectorAroundAxis(up, rightVector, angle);
        normalizeOrientationVectors();
    }
    void yaw(double angle) {
        front = rotateVectorAroundAxis(front, up, -angle);
        normalizeOrientationVectors();
    }
};

#endif
