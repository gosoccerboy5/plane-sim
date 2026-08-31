#ifndef PHYSICS
#define PHYSICS

#include "raylib.h"
#include "raymath.h"
#include <random>
#include <iostream>

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
    Model* cockpitModel;
    float elevatorDeflection = 0;
    float aileronDeflection = 0; 
    float rudderDeflection = 0;
    
    Plane(Model* model, Model* cockpitModel): model(model), cockpitModel(cockpitModel) {
        velocity = {0, 0, 3.0f};
        position = (Vector3){ 0.0f, 20.0f, 0.0f };
    }
    
    void normalizeOrientationVectors() {
        front = Vector3Normalize(front);
        up = Vector3Normalize(up);
    }
    void update() {
        velocity = Vector3Scale(front, Vector3Length(velocity));
        position = position + velocity;
        pitch(elevatorDeflection * 0.02);
        roll(aileronDeflection * .1);
        yaw(rudderDeflection * 0.0075);
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

template<int N>
class PerlinNoise {
    std::vector<std::vector<Vector2>> GVA {};//gradient vector angles
    public:
    PerlinNoise() {
        std::random_device rd;
        std::mt19937 gen(rd());
        
        std::uniform_real_distribution<float> dis(0.0f, 2.0*PI);
        
        for (int i = 0; i < N; ++i) {
            GVA.push_back(std::vector<Vector2>{});
            for (int j = 0; j < N; ++j) {
                float angle = dis(gen);
                GVA[i].push_back(Vector2{cos(angle), sin(angle)});
            }
        }
    }
    float value(float x, float y) {
        int left = static_cast<int>(floor(x)) % N, right = static_cast<int>((floor(x)+1)) % N, top = static_cast<int>(floor(y)) % N, bottom = static_cast<int>((floor(y)+1)) % N;
        x -= floor(x);
        y -= floor(y);
        float topLeftInfluence = Vector2DotProduct(GVA[top][left], {x, y});
        float topRightInfluence = Vector2DotProduct(GVA[top][right], {x-1.0f, y});
        float bottomLeftInfluence = Vector2DotProduct(GVA[bottom][left], {x, y-1.0f});
        float bottomRightInfluence = Vector2DotProduct(GVA[bottom][right], {x-1.0f, y-1.0f});
        float topInfluence = Lerp(topLeftInfluence, topRightInfluence, x);
        float bottomInfluence = Lerp(bottomLeftInfluence, bottomRightInfluence, x);
        float finalValue = Lerp(topInfluence, bottomInfluence, y) * sqrt(2.0);
        return finalValue;
    }
};

#endif
