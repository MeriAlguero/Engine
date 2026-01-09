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
    void LookAt(const Vector3& target);

    const Matrix& GetProjectionMatrix() const;
    const Matrix& GetViewMatrix() const { return view; }

    const Matrix& getViewMatrix() const { return GetViewMatrix(); }
    const Matrix& getProjectionMatrix() const { return GetProjectionMatrix(); }

private:
    Vector3 position = { 0.0f, 2.0f, 10.0f };
    Quaternion rotation = Quaternion::Identity;
    Matrix view;
    Matrix projection;

    Vector3 GetForward() const {
        return Vector3::Transform(Vector3(0, 0, -1), rotation);
    }

    struct {
        float polar = 0.0f;
        float azimuthal = 0.0f;
        Vector3 translation = { 0.0f, 2.0f, 10.0f };
    } params;

    int dragPosX = 0;
    int dragPosY = 0;

    float fov = 0.785f; // 45°
    float aspectRatio = 1.77f;
    float nearPlane = 0.1f;
    float farPlane = 200.0f;
    float speed = 5.0f;
    float sensitivity = 0.25f;
};