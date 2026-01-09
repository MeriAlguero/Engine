#include "Globals.h"
#include "ModuleCamera.h"
#include "Application.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "ModuleD3D12.h"

bool ModuleCamera::init() {
    position = Vector3(0.0f, 2.0f, 10.0f);
    rotation = Quaternion::Identity;

    // Inicializar parámetros como hace tu profesor
    params.polar = 0.0f;
    params.azimuthal = 0.0f;
    params.translation = position;

    // Inicializar proyección
    projection = Matrix::CreatePerspectiveFieldOfView(fov, aspectRatio, nearPlane, farPlane);

    // Inicializar vista como hace tu profesor
    Quaternion invRot;
    rotation.Inverse(invRot);
    view = Matrix::CreateFromQuaternion(invRot);
    view.Translation(-position);

    return true;
}

void ModuleCamera::update() {
    auto kb = DirectX::Keyboard::Get().GetState();
    auto ms = DirectX::Mouse::Get().GetState();
    float dt = app->getDeltaTime();

    float currentSpeed = speed * (kb.LeftShift ? 3.0f : 1.0f);

    // Movimiento como hace tu profesor - usa Vector3::Transform
    Vector3 translate = Vector3::Zero;
    if (kb.W) translate.z -= 1.0f;
    if (kb.S) translate.z += 1.0f;
    if (kb.D) translate.x += 1.0f;
    if (kb.A) translate.x -= 1.0f;
    if (kb.Q) translate.y += 1.0f;  // Subir
    if (kb.E) translate.y -= 1.0f;  // Bajar

    if (translate != Vector3::Zero) {
        translate.Normalize();
        // Transforma la dirección local a espacio mundial
        Vector3 localDir = Vector3::Transform(translate, rotation);
        position += localDir * currentSpeed * dt;
        params.translation = position;  // Actualizar params como hace tu profesor
    }

    // ZOOM con rueda del ratón (añadido extra)
    if (ms.scrollWheelValue != lastScrollValue) {
        int delta = ms.scrollWheelValue - lastScrollValue;
        float zoomSpeed = 0.1f;
        Vector3 forward = GetForward();
        position += forward * (delta * zoomSpeed);
        params.translation = position;
        lastScrollValue = ms.scrollWheelValue;
    }

    // Rotación como hace tu profesor
    if (ms.leftButton) {
        float dx = (ms.x - dragPosX) * sensitivity * dt;
        float dy = (ms.y - dragPosY) * sensitivity * dt;

        params.polar -= dx;      // Rotación horizontal (yaw)
        params.azimuthal -= dy;  // Rotación vertical (pitch)

        // Limitar pitch como hace tu profesor
        params.azimuthal = std::max(-1.5f, std::min(1.5f, params.azimuthal));
    }

    dragPosX = ms.x;
    dragPosY = ms.y;

    // Actualizar rotación como hace tu profesor
    Quaternion rotation_polar = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), params.polar);
    Quaternion rotation_azimuthal = Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), params.azimuthal);
    rotation = rotation_azimuthal * rotation_polar;  // ¡Importante: este orden!

    position = params.translation;  // Sincronizar posición

    // Actualizar matriz de vista como hace tu profesor
    Quaternion invRot;
    rotation.Inverse(invRot);
    view = Matrix::CreateFromQuaternion(invRot);
    view.Translation(Vector3::Transform(-position, invRot));
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
    // Implementación más simple siguiendo el estilo de tu profesor
    Vector3 forward = target - position;
    forward.Normalize();

    // Crear una vista LookAt
    view = Matrix::CreateLookAt(position, target, Vector3(0, 1, 0));

    // Extraer rotación de la matriz de vista
    Matrix rotationMatrix = view;
    rotationMatrix.Invert();  // La vista es la inversa de la matriz de la cámara

    rotation = Quaternion::CreateFromRotationMatrix(rotationMatrix);
    rotation.Normalize();

    // Actualizar params
    params.translation = position;
}