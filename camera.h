/*
    First person camera functions, removes headbob from default camera. Most other
    functionality is basically ripped from the raylib first person cam implementation
*/

#include "raylib.h"

#include <math.h>               // Required for: sqrt(), sinf(), cosf()

#ifndef PI
    #define PI 3.14159265358979323846
#endif
#ifndef DEG2RAD
    #define DEG2RAD (PI/180.0f)
#endif
#ifndef RAD2DEG
    #define RAD2DEG (180.0f/PI)
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// Camera mouse movement sensitivity
#define CAMERA_MOUSE_MOVE_SENSITIVITY                   0.003f
#define CAMERA_MOUSE_SCROLL_SENSITIVITY                 1.5f

// FREE_CAMERA
#define CAMERA_FREE_MOUSE_SENSITIVITY                   0.01f
#define CAMERA_FREE_DISTANCE_MIN_CLAMP                  0.3f
#define CAMERA_FREE_DISTANCE_MAX_CLAMP                  120.0f
#define CAMERA_FREE_MIN_CLAMP                           85.0f
#define CAMERA_FREE_MAX_CLAMP                          -85.0f
#define CAMERA_FREE_SMOOTH_ZOOM_SENSITIVITY             0.05f
#define CAMERA_FREE_PANNING_DIVIDER                     5.1f

// ORBITAL_CAMERA
#define CAMERA_ORBITAL_SPEED                            0.01f       // Radians per frame

// FIRST_PERSON
//#define CAMERA_FIRST_PERSON_MOUSE_SENSITIVITY           0.003f
#define CAMERA_FIRST_PERSON_FOCUS_DISTANCE              25.0f
#define CAMERA_FIRST_PERSON_MIN_CLAMP                   85.0f
#define CAMERA_FIRST_PERSON_MAX_CLAMP                  -85.0f

#define CAMERA_FIRST_PERSON_STEP_TRIGONOMETRIC_DIVIDER  5.0f
#define CAMERA_FIRST_PERSON_STEP_DIVIDER                30.0f
#define CAMERA_FIRST_PERSON_WAVING_DIVIDER              200.0f

// THIRD_PERSON
//#define CAMERA_THIRD_PERSON_MOUSE_SENSITIVITY           0.003f
#define CAMERA_THIRD_PERSON_DISTANCE_CLAMP              1.2f
#define CAMERA_THIRD_PERSON_MIN_CLAMP                   5.0f
#define CAMERA_THIRD_PERSON_MAX_CLAMP                  -85.0f
#define CAMERA_THIRD_PERSON_OFFSET                      (Vector3){ 0.4f, 0.0f, 0.0f }

// PLAYER (used by camera)
#define PLAYER_MOVEMENT_SENSITIVITY                     20.0f

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Camera move modes (first person and third person cameras)
typedef enum {
    MOVE_FRONT = 0,
    MOVE_BACK,
    MOVE_RIGHT,
    MOVE_LEFT,
    MOVE_UP,
    MOVE_DOWN
} CameraMove;

/*fwd decls, globals*/
void InitCustomCamera(Camera* camera);
void UpdateCustomCamera(Camera* camera);

static Vector2 cameraAngle = { 0.0f, 0.0f };          // Camera angle in plane XZ
static float cameraTargetDistance = 0.0f;             // Camera distance from position to target
static float playerEyesPosition = 1.85f;              // Default player eyes position from ground (in meters)

static int cameraMoveControl[6]  = { 'W', 'S', 'D', 'A', 'E', 'Q' };

/*functions*/
void InitCustomCamera(Camera* camera){
    camera->position = (Vector3){ 10.0f, 10.0f, 10.0f }; // Camera position
    camera->target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera->up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera->fovy = 45.0f;                                // Camera field-of-view Y
    camera->type = CAMERA_PERSPECTIVE;                   // Camera mode type

    SetCameraMode(*camera, CAMERA_CUSTOM); 
    
    DisableCursor();
}

void UpdateCustomCamera(Camera* camera){
    static Vector2 previousMousePosition = { 0.0f, 0.0f };

    // TODO: Compute cameraTargetDistance and cameraAngle here

    // Mouse movement detection
    Vector2 mousePositionDelta = { 0.0f, 0.0f };
    Vector2 mousePosition = GetMousePosition();
    int mouseWheelMove = GetMouseWheelMove();

    bool direction[6] = { IsKeyDown(cameraMoveControl[MOVE_FRONT]),
                          IsKeyDown(cameraMoveControl[MOVE_BACK]),
                          IsKeyDown(cameraMoveControl[MOVE_RIGHT]),
                          IsKeyDown(cameraMoveControl[MOVE_LEFT]),
                          IsKeyDown(cameraMoveControl[MOVE_UP]),
                          IsKeyDown(cameraMoveControl[MOVE_DOWN]) };

    // TODO: Consider touch inputs for camera

    mousePositionDelta.x = mousePosition.x - previousMousePosition.x;
    mousePositionDelta.y = mousePosition.y - previousMousePosition.y;

    previousMousePosition = mousePosition;
    camera->position.x +=  (sinf(cameraAngle.x)*direction[MOVE_BACK] -
                            sinf(cameraAngle.x)*direction[MOVE_FRONT] -
                            cosf(cameraAngle.x)*direction[MOVE_LEFT] +
                            cosf(cameraAngle.x)*direction[MOVE_RIGHT])/PLAYER_MOVEMENT_SENSITIVITY;

    camera->position.y +=  (sinf(cameraAngle.y)*direction[MOVE_FRONT] -
                            sinf(cameraAngle.y)*direction[MOVE_BACK] +
                            1.0f*direction[MOVE_UP] - 1.0f*direction[MOVE_DOWN])/PLAYER_MOVEMENT_SENSITIVITY;

    camera->position.z +=  (cosf(cameraAngle.x)*direction[MOVE_BACK] -
                            cosf(cameraAngle.x)*direction[MOVE_FRONT] +
                            sinf(cameraAngle.x)*direction[MOVE_LEFT] -
                            sinf(cameraAngle.x)*direction[MOVE_RIGHT])/PLAYER_MOVEMENT_SENSITIVITY;

    bool isMoving = false;  // Required for swinging

    for (int i = 0; i < 6; i++) if (direction[i]) { isMoving = true; break; }

    // Camera orientation calculation
    cameraAngle.x += (mousePositionDelta.x*-CAMERA_MOUSE_MOVE_SENSITIVITY);
    cameraAngle.y += (mousePositionDelta.y*-CAMERA_MOUSE_MOVE_SENSITIVITY);

    // Angle clamp
    if (cameraAngle.y > CAMERA_FIRST_PERSON_MIN_CLAMP*DEG2RAD) cameraAngle.y = CAMERA_FIRST_PERSON_MIN_CLAMP*DEG2RAD;
    else if (cameraAngle.y < CAMERA_FIRST_PERSON_MAX_CLAMP*DEG2RAD) cameraAngle.y = CAMERA_FIRST_PERSON_MAX_CLAMP*DEG2RAD;

    // Camera is always looking at player
    camera->target.x = camera->position.x - sinf(cameraAngle.x)*CAMERA_FIRST_PERSON_FOCUS_DISTANCE;
    camera->target.y = camera->position.y + sinf(cameraAngle.y)*CAMERA_FIRST_PERSON_FOCUS_DISTANCE;
    camera->target.z = camera->position.z - cosf(cameraAngle.x)*CAMERA_FIRST_PERSON_FOCUS_DISTANCE;

    // Camera position update
    // NOTE: On CAMERA_FIRST_PERSON player Y-movement is limited to player 'eyes position'
    camera->position.y = playerEyesPosition;
}

