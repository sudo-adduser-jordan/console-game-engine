#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <vector> // some std imports from olcPixelGameEngine.h
#include <fstream>
#include <strstream>
#include <algorithm>
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
    olc::Pixel col;
};

struct Mesh
{
    vector<Triangle> triangles;

    bool LoadFromObjectFile(string sFilename)
    {
        ifstream file(sFilename);
        if (!file.is_open())
            return false;

        // Local cache of verts
        vector<Vec3d> vertices;

        while (!file.eof())
        {
            char line[128];
            file.getline(line, 128);

            strstream s;
            s << line;

            char junk;
            if (line[0] == 'v')
            {
                Vec3d v;
                s >> junk >> v.x >> v.y >> v.z;
                vertices.push_back(v);
            }

            if (line[0] == 'f')
            {
                int f[3];
                s >> junk >> f[0] >> f[1] >> f[2];
                triangles.push_back({vertices[f[0] - 1], vertices[f[1] - 1], vertices[f[2] - 1]});
            }
        }

        return true;
    }
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
    Vec3d vCamera; // 0, 0
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

    // Input parameter lum must be between 0 and 1 - i.e. [0, 1]
    olc::Pixel GetColour(float lum)
    {
        int nValue = (int)(std::max(lum, 0.20f) * 255.0f);
        return olc::Pixel(nValue, nValue, nValue);
    }

public:
    bool OnUserCreate() override
    {
        // // Called once at the start, create things here
        // meshCube.triangles = {
        //     // SOUTH
        //     {0.0f, 0.0f, 0.0f, /**/ 0.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 0.0f},
        //     {0.0f, 0.0f, 0.0f, /**/ 1.0f, 1.0f, 0.0f, /**/ 1.0f, 0.0f, 0.0f},
        //     // EAST
        //     {1.0f, 0.0f, 0.0f, /**/ 1.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f},
        //     {1.0f, 0.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, /**/ 1.0f, 0.0f, 1.0f},
        //     // NORTH
        //     {1.0f, 0.0f, 1.0f, /**/ 1.0f, 1.0f, 1.0f, /**/ 0.0f, 1.0f, 1.0f},
        //     {1.0f, 0.0f, 1.0f, /**/ 0.0f, 1.0f, 1.0f, /**/ 0.0f, 0.0f, 1.0f},
        //     // WEST
        //     {0.0f, 0.0f, 1.0f, /**/ 0.0f, 1.0f, 1.0f, /**/ 0.0f, 1.0f, 0.0f},
        //     {0.0f, 0.0f, 1.0f, /**/ 0.0f, 1.0f, 0.0f, /**/ 0.0f, 0.0f, 0.0f},
        //     // TOP
        //     {0.0f, 1.0f, 0.0f, /**/ 0.0f, 1.0f, 1.0f, /**/ 1.0f, 1.0f, 1.0f},
        //     {0.0f, 1.0f, 0.0f, /**/ 1.0f, 1.0f, 1.0f, /**/ 1.0f, 1.0f, 0.0f},
        //     // BOTTOM
        //     {1.0f, 0.0f, 1.0f, /**/ 0.0f, 0.0f, 1.0f, /**/ 0.0f, 0.0f, 0.0f},
        //     {1.0f, 0.0f, 1.0f, /**/ 0.0f, 0.0f, 0.0f, /**/ 1.0f, 0.0f, 0.0f},

        // };
        meshCube.LoadFromObjectFile("assets/cessna.obj");

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

            // USE CROSS-PRODUCT TO GET SURFACE NORMAL LINE
            Vec3d normal;
            Vec3d lineOne;
            Vec3d lineTwo;

            lineOne.x = triangleTranslated.points[1].x - triangleTranslated.points[0].x;
            lineOne.y = triangleTranslated.points[1].y - triangleTranslated.points[0].y;
            lineOne.z = triangleTranslated.points[1].z - triangleTranslated.points[0].z;

            lineTwo.x = triangleTranslated.points[2].x - triangleTranslated.points[0].x;
            lineTwo.y = triangleTranslated.points[2].y - triangleTranslated.points[0].y;
            lineTwo.z = triangleTranslated.points[2].z - triangleTranslated.points[0].z;

            normal.x = lineOne.y * lineTwo.z - lineOne.z * lineTwo.y;
            normal.y = lineOne.z * lineTwo.x - lineOne.x * lineTwo.z;
            normal.z = lineOne.x * lineTwo.y - lineOne.y * lineTwo.x;

            // NORMALIZE NORMAL
            float length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z); // PYHANGOREOUS THEOREM
            normal.x /= length;
            normal.y /= length;
            normal.z /= length;

            // DOT PRODUCT OF NORMAL LINE AND CAMERA
            float dotProduct = normal.x * (triangleTranslated.points[0].x - vCamera.x) +
                               normal.y * (triangleTranslated.points[0].y - vCamera.y) +
                               normal.z * (triangleTranslated.points[0].z - vCamera.z);

            vCamera.x = 0;
            vCamera.y = 0;
            vCamera.z = 0;

            if (dotProduct < 0.0f)
            {
                // ILLUMINATION
                Vec3d lightDirection = {0.0f, 0.0f, -1.0f};

                // NORMALIZE LIGHT VECTOR
                float length = sqrtf(lightDirection.x * lightDirection.x + lightDirection.y * lightDirection.y + lightDirection.z * lightDirection.z); // PYHANGOREOUS THEOREM
                lightDirection.x /= length;
                lightDirection.y /= length;
                lightDirection.z /= length;

                // How similar is normal to light direction
                float dp = normal.x * lightDirection.x + normal.y * lightDirection.y + normal.z * lightDirection.z;

                // Choose console colours as required
                triangleTranslated.col = GetColour(dp);

                // Project triangles from 3D --> 2D
                MultiplyMatrixVector(triangleTranslated.points[0], triangleProjected.points[0], projectionMatrix);
                MultiplyMatrixVector(triangleTranslated.points[1], triangleProjected.points[1], projectionMatrix);
                MultiplyMatrixVector(triangleTranslated.points[2], triangleProjected.points[2], projectionMatrix);
                triangleProjected.col = triangleTranslated.col;

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
                FillTriangle(triangleProjected.points[0].x, triangleProjected.points[0].y,
                             triangleProjected.points[1].x, triangleProjected.points[1].y,
                             triangleProjected.points[2].x, triangleProjected.points[2].y,
                             triangleProjected.col);

                DrawTriangle(triangleProjected.points[0].x, triangleProjected.points[0].y,
                             triangleProjected.points[1].x, triangleProjected.points[1].y,
                             triangleProjected.points[2].x, triangleProjected.points[2].y,
                             olc::BLACK);
            }
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
