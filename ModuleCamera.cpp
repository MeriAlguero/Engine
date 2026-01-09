#include "Globals.h"
#include "ModuleCamera.h"
#include "Application.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "ModuleD3D12.h"

bool ModuleCamera::init() {
    params.translation = Vector3(0.0f, 2.0f, 10.0f);

    // Initialize projection matrix
    projection = Matrix::CreatePerspectiveFieldOfView(fov, aspectRatio, nearPlane, farPlane);

    // Initialize view matrix from initial position/rotation
    Vector3 forward = GetForward();
    view = Matrix::CreateLookAt(position, position + forward, Vector3(0, 1, 0));

    return true;
}

void ModuleCamera::update() {
    auto kb = DirectX::Keyboard::Get().GetState();
    auto ms = DirectX::Mouse::Get().GetState();
    float dt = app->getDeltaTime();

    float currentSpeed = speed * (kb.LeftShift ? 3.0f : 1.0f);

    Vector3 move = Vector3::Zero;
    if (kb.W) move.z -= 1.0f;
    if (kb.S) move.z += 1.0f;
    if (kb.D) move.x += 1.0f;
    if (kb.A) move.x -= 1.0f;
    if (kb.Q) move.y += 1.0f;
    if (kb.E) move.y -= 1.0f;

    if (move != Vector3::Zero) {
        move.Normalize();
        position += Vector3::Transform(move * currentSpeed * dt, rotation);
    }

    if (ms.leftButton) {
        params.polar -= (ms.x - dragPosX) * sensitivity * dt;
        params.azimuthal -= (ms.y - dragPosY) * sensitivity * dt;
        params.azimuthal = std::max(-1.5f, std::min(1.5f, params.azimuthal));

        // Update rotation from polar/azimuthal
        rotation = Quaternion::CreateFromAxisAngle(Vector3(0, 1, 0), params.polar) *
            Quaternion::CreateFromAxisAngle(Vector3(1, 0, 0), params.azimuthal);
    }

    dragPosX = ms.x;
    dragPosY = ms.y;

    // Create view matrix directly from position and rotation
    Vector3 forward = GetForward();
    Vector3 up = Vector3::Transform(Vector3(0, 1, 0), rotation);
    view = Matrix::CreateLookAt(position, position + forward, up);
}

const Matrix& ModuleCamera::GetProjectionMatrix() const {
    return projection;
}

void ModuleCamera::SetAspectRatio(float aspect) {
    this->aspectRatio = aspect;
    projection = Matrix::CreatePerspectiveFieldOfView(fov, aspectRatio, nearPlane, farPlane);
}

void ModuleCamera::SetFOV(float fovRadians) {
    this->fov = fovRadians;
    projection = Matrix::CreatePerspectiveFieldOfView(fov, aspectRatio, nearPlane, farPlane);
}

void ModuleCamera::SetPlaneDistances(float nearP, float farP) {
    this->nearPlane = nearP;
    this->farPlane = farP;
    projection = Matrix::CreatePerspectiveFieldOfView(fov, aspectRatio, nearPlane, farPlane);
}

void ModuleCamera::LookAt(const Vector3& target) {
    // Create a view matrix looking at target
    view = Matrix::CreateLookAt(position, target, Vector3(0, 1, 0));

    // Extract rotation from view matrix
    // We need to invert the view matrix to get the camera's world matrix
    Matrix inverseView;
    view.Invert(inverseView);

    // Extract rotation from the inverse view matrix
    // The upper 3x3 of the inverse view matrix contains the rotation
    rotation = Quaternion::CreateFromRotationMatrix(inverseView);
    rotation.Normalize();
}