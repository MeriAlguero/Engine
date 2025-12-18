#include "Globals.h"
#include "ModuleCamera.h"
#include "Application.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "ModuleD3D12.h"

using namespace DirectX::SimpleMath;
bool ModuleCamera::init() {
    params.translation = Vector3(0.0f, 2.0f, 10.0f);
    return true;
}

void ModuleCamera::update() {
    auto kb = DirectX::Keyboard::Get().GetState();
    auto ms = DirectX::Mouse::Get().GetState();
    float dt = app->getDeltaTime();

    float currentSpeed = speed * (kb.LeftShift ? 3.0f : 1.0f);

    DirectX::SimpleMath::Vector3 move = DirectX::SimpleMath::Vector3::Zero;
    if (kb.W) move.z -= 1.0f;
    if (kb.S) move.z += 1.0f;
    if (kb.D) move.x += 1.0f;
    if (kb.A) move.x -= 1.0f;
    if (kb.Q) move.y += 1.0f;
    if (kb.E) move.y -= 1.0f;

    if (move != DirectX::SimpleMath::Vector3::Zero) {
        move.Normalize();
        position += DirectX::SimpleMath::Vector3::Transform(move * currentSpeed * dt, rotation);
    }

    if (ms.leftButton) {
        params.polar -= (ms.x - dragPosX) * sensitivity * dt;
        params.azimuthal -= (ms.y - dragPosY) * sensitivity * dt;
        params.azimuthal = std::max(-1.5f, std::min(1.5f, params.azimuthal));
    }

    dragPosX = ms.x;
    dragPosY = ms.y;

    rotation = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle({ 0, 1, 0 }, params.polar) *
        DirectX::SimpleMath::Quaternion::CreateFromAxisAngle({ 1, 0, 0 }, params.azimuthal);

    view = DirectX::SimpleMath::Matrix::CreateLookAt(position, position + GetForward(), { 0, 1, 0 });
}

Matrix ModuleCamera::GetProjectionMatrix() const {
    return Matrix::CreatePerspectiveFieldOfView(fov, aspectRatio, nearPlane, farPlane);
}

void ModuleCamera::SetAspectRatio(float aspect) {
    this->aspectRatio = aspect;
}

void ModuleCamera::SetFOV(float fovRadians) {
    this->fov = fovRadians;
}