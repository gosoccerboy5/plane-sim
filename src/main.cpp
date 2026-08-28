#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "rcamera.h"

#include "physics.h"

#include <random>
#include <iostream>

//`CameraMode` is already defined in raylib but not quite what we want
enum CameraType {
    THIRD_PERSON,
    THIRD_PERSON_LOCKED,
    FIRST_PERSON,
};

float random() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    return dis(gen);
}

void resetToZero(float& input, float step) {
    if (std::abs(input) < step) input = 0;
    else if (input < 0) input += step;
    else input -= step;
}

int main() {
    const int screenWidth = 1600;
    const int screenHeight = 900;

    //SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(screenWidth, screenHeight, "Raylib Plane Sim");
    rlDisableBackfaceCulling();


    Camera camera = { 0 };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    Vector2 camAngle {0, 0};
    CameraType cameraMode = CameraType::THIRD_PERSON;
    
    float mouseWheelMomentum = 0;

    Model model = LoadModel("src/assets/plane.obj");
    Model map = LoadModel("src/assets/landscape.obj"); 
    Model skybox = LoadModel("src/assets/skybox.obj");
    
    Plane plane(&model);
    
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        Vector2 mouseMovement = GetMouseDelta();
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            camAngle = camAngle + Vector2Scale(mouseMovement, 0.01);
        }
        camAngle.y = Clamp(camAngle.y, -3.14/2, 3.14/2);

        mouseWheelMomentum += GetMouseWheelMove() * 0.02;
        camera.fovy = Clamp(camera.fovy * (1-mouseWheelMomentum), 10, 120);
        mouseWheelMomentum *= 0.9;
        
        
        bool pitching = false, rolling = false, yawing = false;
        float elevatorPitchRate = 0.01, aileronPitchRate = 0.02, rudderPitchRate = 0.01;
        
        if (IsKeyDown(KEY_A)) {
            plane.rudderDeflection -= rudderPitchRate;
            yawing = true;
        }
        if (IsKeyDown(KEY_D)) {
            plane.rudderDeflection += rudderPitchRate;
            yawing = true;
        }
        if (!yawing) {
            resetToZero(plane.rudderDeflection, rudderPitchRate);
        } else plane.rudderDeflection = Clamp(plane.rudderDeflection, -DEG2RAD*20, DEG2RAD*20);
        
        if (IsKeyDown(KEY_S)) {
            plane.elevatorDeflection += elevatorPitchRate;
            pitching = true;
        }
        if (IsKeyDown(KEY_W)) {
            plane.elevatorDeflection -= elevatorPitchRate;
            pitching = true;
        }
        if (!pitching) {
            resetToZero(plane.elevatorDeflection, elevatorPitchRate);
        } else plane.elevatorDeflection = Clamp(plane.elevatorDeflection, -DEG2RAD*10, DEG2RAD*20);
        
        if (IsKeyDown(KEY_E)) {
            plane.aileronDeflection += aileronPitchRate;
            rolling = true;
        }
        if (IsKeyDown(KEY_Q)) {
            plane.aileronDeflection -= aileronPitchRate;
            rolling = true;
        } 
        if (!rolling) {
            resetToZero(plane.aileronDeflection, aileronPitchRate);
        } else plane.aileronDeflection = Clamp(plane.aileronDeflection, -DEG2RAD*20, DEG2RAD*20);
        
        plane.update();
        
        
        if (IsKeyPressed(KEY_C)) {
            if (cameraMode == CameraType::THIRD_PERSON) {
                //cameraMode = CameraType::THIRD_PERSON_LOCKED;
                cameraMode = CameraType::FIRST_PERSON;
                camAngle = {0, 0};
            } else if (cameraMode == CameraType::THIRD_PERSON_LOCKED) {
                cameraMode = CameraType::FIRST_PERSON;
                camAngle = {0, 0};
            } else if (cameraMode == CameraType::FIRST_PERSON) {
                cameraMode = CameraType::THIRD_PERSON;
            }
        }
    
        model.transform = {
            plane.right().x, plane.up.x, plane.front.x, 0.0f,
            plane.right().y, plane.up.y, plane.front.y, 0.0f,
            plane.right().z, plane.up.z, plane.front.z, 0.0f,
            0.0f,            0.0f,       0.0f,          1.0f,
        };
        //transform matrix can be formed directly from orientation vectors as columns (orientation vectors form x, y, z axes)
        //as opposed to using MatrixRotateXYZ from euler angles (suffers from gimbal lock)
        
        float cameraDistance = 10;
        
        if (cameraMode == CameraType::THIRD_PERSON) {
            camera.target = plane.position;
            camera.up = {0, 1.0f, 0};
            camera.position = Vector3Add(plane.position, Vector3Scale({sin(-camAngle.x)*cos(-camAngle.y), sin(-camAngle.y), cos(-camAngle.x)*cos(-camAngle.y)}, -cameraDistance));
        } else if (cameraMode == CameraType::THIRD_PERSON_LOCKED) {
            camera.target = plane.position;
            camera.position = plane.position + Vector3Scale(
                Vector3Scale(plane.right(), sin(camAngle.x)*cos(-camAngle.y)) + 
                Vector3Scale(plane.up, sin(-camAngle.y)) + 
                Vector3Scale(plane.front, cos(camAngle.x)*cos(-camAngle.y)), 
                -cameraDistance);
            camera.up = plane.up;
        } else if (cameraMode == CameraType::FIRST_PERSON) {
            camAngle.y = Clamp(camAngle.y, -1.45, 1);
            camAngle.x = Clamp(camAngle.x, -0.75*PI, 0.75*PI);
            camera.up = plane.up;
            camera.position = plane.position + Vector3Scale(plane.up, .5) + Vector3Scale(plane.front, .5);
            camera.target = camera.position + Vector3Scale(
                Vector3Scale(plane.right(), sin(camAngle.x)*cos(-camAngle.y)) + 
                Vector3Scale(plane.up, sin(-camAngle.y)) + 
                Vector3Scale(plane.front, cos(camAngle.x)*cos(-camAngle.y)), 
                10);
        }
        if (plane.elevatorDeflection != 0) {
            Vector3 offset = Vector3Scale({random(), random(), random()}, plane.elevatorDeflection * 0.1);
            if (cameraMode == CameraType::FIRST_PERSON) camera.position = camera.position + offset;
            else if (cameraMode == CameraType::THIRD_PERSON || cameraMode == CameraType::THIRD_PERSON_LOCKED) plane.position = plane.position + offset;
        }



        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);
                rlDisableDepthMask();
                DrawModel(skybox, plane.position, 2.0f, WHITE);
                rlEnableDepthMask();

                DrawModel(*plane.model, plane.position, 1.0f, WHITE);
                
                DrawModel(map, {0, 0, 0}, 1.0f, WHITE);
                
                
                rlDisableDepthMask();
                DrawSphere(plane.position + Vector3Scale(plane.front, 100), .4, RED);
                rlEnableDepthMask();
            EndMode3D();


        EndDrawing();
    }
    
    
    UnloadModel(model);
    UnloadModel(map);
    UnloadModel(skybox);

    CloseWindow();
    
    return 0;
}
