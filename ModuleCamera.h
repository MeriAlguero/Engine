#pragma once
#include "Module.h"
#include "SimpleMath.h"

class ModuleCamera : public Module {
public:
    bool init() override;
    void update() override;

    void SetFOV(float fovRadians);
    void SetAspectRatio(float aspect);
    void SetPlaneDistances(float nearP, float farP);
    void LookAt(const DirectX::SimpleMath::Vector3& target);
    
    DirectX::SimpleMath::Matrix GetProjectionMatrix() const;
    DirectX::SimpleMath::Matrix GetViewMatrix() const { return view; }

    DirectX::SimpleMath::Matrix getViewMatrix() const { return GetViewMatrix(); }
    DirectX::SimpleMath::Matrix getProjectionMatrix() const { return GetProjectionMatrix(); }

private:
    DirectX::SimpleMath::Vector3 position = { 0.0f, 2.0f, 10.0f };
    DirectX::SimpleMath::Quaternion rotation = DirectX::SimpleMath::Quaternion::Identity;
    DirectX::SimpleMath::Matrix view;
    DirectX::SimpleMath::Vector3 GetForward() const {
        return DirectX::SimpleMath::Vector3::Transform(DirectX::SimpleMath::Vector3(0, 0, -1), rotation);
    }

    struct {
        float polar = 0.0f;
        float azimuthal = 0.0f;
        DirectX::SimpleMath::Vector3 translation = { 0.0f, 2.0f, 10.0f };
    } params;

    int dragPosX = 0;
    int dragPosY = 0;

    float fov = 0.785f; // 45º
    float aspectRatio = 1.77f;
    float nearPlane = 0.1f;
    float farPlane = 200.0f;
    float speed = 5.0f;
    float sensitivity = 0.25f;
};