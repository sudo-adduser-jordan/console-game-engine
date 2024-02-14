#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
// #include <vector>
using namespace std;

struct Vec3d
{
    float x;
    float y;
    float z;
};

struct Triangle
{
    Vec3d points[3];
};

struct Mesh
{
    vector<Triangle> triangles;
};

struct Matrix // 4 x 4
{
    float matrix[4][4] = {0};
};

class ConsoleGameEngine : public olc::PixelGameEngine
{
public:
    ConsoleGameEngine()
    {
        sAppName = "Console Game Engine";
    }

private:
    Mesh meshCube;
    Matrix projectionMatrix;
    float fTheta;

    void MultiplyMatrixVector(Vec3d &vectOne, Vec3d &vectTwo, Matrix &matrix)
    {
        vectTwo.x = vectOne.x * matrix.matrix[0][0] + vectOne.y * matrix.matrix[1][0] + vectOne.z * matrix.matrix[2][0] + matrix.matrix[3][0];
        vectTwo.y = vectOne.x * matrix.matrix[0][1] + vectOne.y * matrix.matrix[1][1] + vectOne.z * matrix.matrix[2][1] + matrix.matrix[3][1];
        vectTwo.z = vectOne.x * matrix.matrix[0][2] + vectOne.y * matrix.matrix[1][2] + vectOne.z * matrix.matrix[2][2] + matrix.matrix[3][2];
        float w = vectOne.x * matrix.matrix[0][3] + vectOne.y * matrix.matrix[1][3] + vectOne.z * matrix.matrix[2][3] + matrix.matrix[3][3];
        if (w != 0.0f) // 4d to cartesion space
        {
            vectTwo.x /= w;
            vectTwo.y /= w;
            vectTwo.z /= w;
        }
    }

public:
    bool OnUserCreate() override
    {
        // Called once at the start, create things here
        meshCube.triangles = {
            // SOUTH
            {0.0f, 0.0f, 0.0f, /**/ 0.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, /**/ 1.0f, 1.0f, 0.0f, /**/ 1.0f, 0.0f, 0.0f},
            // EAST
            {1.0f, 0.0f, 0.0f, /**/ 1.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f},
            {1.0f, 0.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, /**/ 1.0f, 0.0f, 1.0f},
            // NORTH
            {1.0f, 0.0f, 1.0f, /**/ 1.0f, 1.0f, 1.0f, /**/ 0.0f, 1.0f, 1.0f},
            {1.0f, 0.0f, 1.0f, /**/ 0.0f, 1.0f, 1.0f, /**/ 0.0f, 0.0f, 1.0f},
            // WEST
            {0.0f, 0.0f, 1.0f, /**/ 0.0f, 1.0f, 1.0f, /**/ 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, /**/ 0.0f, 1.0f, 0.0f, /**/ 0.0f, 0.0f, 0.0f},
            // TOP
            {0.0f, 1.0f, 0.0f, /**/ 0.0f, 1.0f, 1.0f, /**/ 1.0f, 1.0f, 1.0f},
            {0.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, /**/ 1.0f, 1.0f, 0.0f},
            // BOTTOM
            {1.0f, 0.0f, 1.0f, /**/ 0.0f, 0.0f, 1.0f, /**/ 0.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 1.0f, /**/ 0.0f, 0.0f, 0.0f, /**/ 1.0f, 0.0f, 0.0f},

        };

        // PROJECTION MATRIX
        float fNear = 0.1f;
        float fFar = 1000.0f;
        float fFOV = 90.0f;
        float fAspectRatio = (float)ScreenHeight() / (float)ScreenWidth();
        float fFOVRad = 1.0f / tanf(fFOV * 0.5f / 180.0f * 3.14159f); // TANGENT CALCULATION CONVERTED TO RADIANS FROM DEGREES

        projectionMatrix.matrix[0][0] = fAspectRatio * fFOVRad;
        projectionMatrix.matrix[1][1] = fFOVRad;
        projectionMatrix.matrix[2][2] = fFar / (fFar - fNear);
        projectionMatrix.matrix[3][2] = (-fFar * fNear) / (fFar - fNear);
        projectionMatrix.matrix[2][3] = 1.0f;
        projectionMatrix.matrix[3][3] = 0.0f;

        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        // Clear Screen
        Clear(olc::BLACK);

        // SET UP ROTATION MATRICES
        Matrix matrixRotateZ;
        Matrix matrixRotateX;
        fTheta += 1.0f * fElapsedTime;

        // Rotation Z
        matrixRotateZ.matrix[0][0] = cosf(fTheta);
        matrixRotateZ.matrix[0][1] = sinf(fTheta);
        matrixRotateZ.matrix[1][0] = -sinf(fTheta);
        matrixRotateZ.matrix[1][1] = cosf(fTheta);
        matrixRotateZ.matrix[2][2] = 1;
        matrixRotateZ.matrix[3][3] = 1;

        // Rotation X
        matrixRotateX.matrix[0][0] = 1;
        matrixRotateX.matrix[1][1] = cosf(fTheta * 0.5f);
        matrixRotateX.matrix[1][2] = sinf(fTheta * 0.5f);
        matrixRotateX.matrix[2][1] = -sinf(fTheta * 0.5f);
        matrixRotateX.matrix[2][2] = cosf(fTheta * 0.5f);
        matrixRotateX.matrix[3][3] = 1;

        // DRAW TRIANGLES
        for (auto triangle : meshCube.triangles)
        {
            Triangle triangleProjected;
            Triangle triangleTranslated;
            Triangle triangleRotateZ;
            Triangle triangleRotateZX;

            // Rotate in Z-Axis
            MultiplyMatrixVector(triangle.points[0], triangleRotateZ.points[0], matrixRotateZ);
            MultiplyMatrixVector(triangle.points[1], triangleRotateZ.points[1], matrixRotateZ);
            MultiplyMatrixVector(triangle.points[2], triangleRotateZ.points[2], matrixRotateZ);

            // Rotate in X-Axis
            MultiplyMatrixVector(triangleRotateZ.points[0], triangleRotateZX.points[0], matrixRotateX);
            MultiplyMatrixVector(triangleRotateZ.points[1], triangleRotateZX.points[1], matrixRotateX);
            MultiplyMatrixVector(triangleRotateZ.points[2], triangleRotateZX.points[2], matrixRotateX);

            // Offset into the screen
            triangleTranslated = triangleRotateZX;
            triangleTranslated.points[0].z = triangleRotateZX.points[0].z + 3.0f;
            triangleTranslated.points[1].z = triangleRotateZX.points[1].z + 3.0f;
            triangleTranslated.points[2].z = triangleRotateZX.points[2].z + 3.0f;

            // Project triangles from 3D --> 2D
            MultiplyMatrixVector(triangleTranslated.points[0], triangleProjected.points[0], projectionMatrix);
            MultiplyMatrixVector(triangleTranslated.points[1], triangleProjected.points[1], projectionMatrix);
            MultiplyMatrixVector(triangleTranslated.points[2], triangleProjected.points[2], projectionMatrix);

            // Scale into view
            triangleProjected.points[0].x += 1.0f;
            triangleProjected.points[0].y += 1.0f;
            triangleProjected.points[1].x += 1.0f;
            triangleProjected.points[1].y += 1.0f;
            triangleProjected.points[2].x += 1.0f;
            triangleProjected.points[2].y += 1.0f;
            triangleProjected.points[0].x *= 0.5f * (float)ScreenWidth();
            triangleProjected.points[0].y *= 0.5f * (float)ScreenHeight();
            triangleProjected.points[1].x *= 0.5f * (float)ScreenWidth();
            triangleProjected.points[1].y *= 0.5f * (float)ScreenHeight();
            triangleProjected.points[2].x *= 0.5f * (float)ScreenWidth();
            triangleProjected.points[2].y *= 0.5f * (float)ScreenHeight();

            // Rasterize triangle
            DrawTriangle(triangleProjected.points[0].x, triangleProjected.points[0].y,
                         triangleProjected.points[1].x, triangleProjected.points[1].y,
                         triangleProjected.points[2].x, triangleProjected.points[2].y,
                         olc::WHITE);
        }
        return true;
    }
};

int main()
{
    ConsoleGameEngine demo;
    if (demo.Construct(256, 240, 4, 4))
        demo.Start();
    return 0;
}
