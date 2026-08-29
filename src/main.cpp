#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "rcamera.h"

#include "physics.h"

#include <random>
#include <iostream>

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif


//`CameraMode` is already defined in raylib but not quite what we want
enum CameraType {
    THIRD_PERSON,
    THIRD_PERSON_LOCKED,
    FIRST_PERSON,
};

float randomFloat() {
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

//--------------------------------------------------------------------------------------
// Module Functions Declaration
//--------------------------------------------------------------------------------------
// Load custom render texture with depth texture attached
static RenderTexture2D LoadRenderTextureDepthTex(int width, int height);

// Unload render texture from GPU memory (VRAM)
static void UnloadRenderTextureDepthTex(RenderTexture2D target);

int main() {
    const int screenWidth = 1600;
    const int screenHeight = 900;

    //SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(screenWidth, screenHeight, "Raylib Plane Sim");
    rlDisableBackfaceCulling();
    rlSetClipPlanes(0.1, 10000.0);


    Camera camera = { 0 };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    Vector2 camAngle {0, -.1};
    CameraType cameraMode = CameraType::THIRD_PERSON;
    
    float mouseWheelMomentum = 0;

    Model model = LoadModel("src/assets/plane.obj");
    Model map = LoadModel("src/assets/landscape.obj"); 
    Model skybox = LoadModel("src/assets/skybox.obj");
    Model cockpit = LoadModel("src/assets/cockpit.obj");
    
    // Load render texture with a depth texture attached
    RenderTexture2D target = LoadRenderTextureDepthTex(screenWidth, screenHeight);

    // Load depth shader and get depth texture shader location
    Shader depthShader = LoadShader(0, TextFormat("src/depth_render.fs", GLSL_VERSION));
    int depthLoc = GetShaderLocation(depthShader, "depthTexture");
    int colorLoc = GetShaderLocation(depthShader, "colorTexture");
    
    int flipTextureLoc = GetShaderLocation(depthShader, "flipY");
    std::cout << "<<<<<<<<<<<<<<" << depthLoc << std::endl<< std::endl<< std::endl<< std::endl<< std::endl;
    SetShaderValue(depthShader, flipTextureLoc, (int[]){ 1 }, SHADER_UNIFORM_INT); // Flip Y texture
    
    Plane plane(&model);
    
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        Vector2 mouseMovement = GetMouseDelta();
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            camAngle.x += mouseMovement.x*0.01;
            camAngle.y -= mouseMovement.y*0.01;
        }
        camAngle.y = Clamp(camAngle.y, -3.14/2, 3.14/2);

        mouseWheelMomentum += GetMouseWheelMove() * 0.02;
        camera.fovy = Clamp(camera.fovy * (1-mouseWheelMomentum), 10, 100);
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
            camera.position = Vector3Add(plane.position, Vector3Scale({sin(-camAngle.x)*cos(camAngle.y), sin(camAngle.y), cos(-camAngle.x)*cos(camAngle.y)}, -cameraDistance));
        } else if (cameraMode == CameraType::THIRD_PERSON_LOCKED) {
            camera.target = plane.position;
            camera.position = plane.position + Vector3Scale(
                Vector3Scale(plane.right(), sin(camAngle.x)*cos(camAngle.y)) + 
                Vector3Scale(plane.up, sin(camAngle.y)) + 
                Vector3Scale(plane.front, cos(camAngle.x)*cos(camAngle.y)), 
                -cameraDistance);
            camera.up = plane.up;
        } else if (cameraMode == CameraType::FIRST_PERSON) {
            camAngle.x = Clamp(camAngle.x, -0.75*PI, 0.75*PI);
            camAngle.y = Clamp(camAngle.y, -.7, 1.45);
            camera.up = plane.up;
            camera.position = plane.position + Vector3Scale(plane.up, .7) + Vector3Scale(plane.front, .65);
            camera.target = camera.position + Vector3Scale(
                Vector3Scale(plane.right(), sin(camAngle.x)*cos(camAngle.y)) + 
                Vector3Scale(plane.up, sin(camAngle.y)) + 
                Vector3Scale(plane.front, cos(camAngle.x)*cos(camAngle.y)), 
                10);
            cockpit.transform = plane.model->transform;
        }
        if (plane.elevatorDeflection != 0) {
            Vector3 offset = Vector3Scale({randomFloat()-0.5f, randomFloat()-0.5f, randomFloat()-0.5f}, plane.elevatorDeflection * 0.03);
            if (cameraMode == CameraType::FIRST_PERSON) camera.position = camera.position + offset;
            else if (cameraMode == CameraType::THIRD_PERSON || cameraMode == CameraType::THIRD_PERSON_LOCKED) plane.position = plane.position + offset;
        }



        /*BeginDrawing();

            


        EndDrawing();*/
        
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);
                if (cameraMode == CameraType::FIRST_PERSON) {
                    DrawModel(cockpit, plane.position, 1.0f, WHITE);
                } else DrawModel(*plane.model, plane.position, 1.0f, WHITE);
                
                DrawModel(map, {0, 0, 0}, 1.0f, WHITE);
                
                rlDisableDepthMask();
                DrawSphere(plane.position + Vector3Scale(plane.front, 100), .4, RED);
                rlEnableDepthMask();
            EndMode3D();
        EndTextureMode();

        // Draw into screen (main framebuffer)
        BeginDrawing();
            ClearBackground(RAYWHITE);
            
            BeginMode3D(camera);
                rlDisableDepthMask();
                DrawModel(skybox, plane.position, 1.0f, WHITE);
                rlEnableDepthMask();
            EndMode3D();

            BeginShaderMode(depthShader);
                SetShaderValueTexture(depthShader, colorLoc, target.texture);
                SetShaderValueTexture(depthShader, depthLoc, target.depth);
                
                DrawTexture(target.texture, 0, 0, WHITE);
            EndShaderMode();
            

        EndDrawing();
    }
    
    
    UnloadModel(model);
    UnloadModel(map);
    UnloadModel(skybox);
    UnloadModel(cockpit);
    
    UnloadRenderTextureDepthTex(target);
    UnloadShader(depthShader);      // Unload shader

    CloseWindow();
    
    return 0;
}


//--------------------------------------------------------------------------------------
// Module Functions Definition
//--------------------------------------------------------------------------------------
// Load custom render texture, create a writable depth texture buffer
static RenderTexture2D LoadRenderTextureDepthTex(int width, int height)
{
    RenderTexture2D target = { 0 };

    target.id = rlLoadFramebuffer(); // Load an empty framebuffer

    if (target.id > 0)
    {
        rlEnableFramebuffer(target.id);

        // Create color texture (default to RGBA)
        target.texture.id = rlLoadTexture(0, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        target.texture.width = width;
        target.texture.height = height;
        target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        target.texture.mipmaps = 1;

        // Create depth texture buffer (instead of raylib default renderbuffer)
        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19;       // DEPTH_COMPONENT_24BIT: Not defined in raylib
        target.depth.mipmaps = 1;

        // Attach color texture and depth texture to FBO
        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id)) TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    }
    else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

// Unload render texture from GPU memory (VRAM)
void UnloadRenderTextureDepthTex(RenderTexture2D target)
{
    if (target.id > 0)
    {
        // Color texture attached to FBO is deleted
        rlUnloadTexture(target.texture.id);
        rlUnloadTexture(target.depth.id);

        // NOTE: Depth texture is automatically
        // queried and deleted before deleting framebuffer
        rlUnloadFramebuffer(target.id);
    }
}