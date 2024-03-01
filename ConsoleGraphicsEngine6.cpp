// CAN BE OPTIMIZED
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <vector>
#include <fstream>
#include <strstream>
#include <algorithm>
using namespace std;

struct Vec3d
{
    float x = 0;
    float y = 0;
    float z = 0;
    float w = 1; // Need a 4th term to perform sensible matrix vector multiplication
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

            char dataType;
            if (line[0] == 'v')
            {
                Vec3d v;
                s >> dataType >> v.x >> v.y >> v.z;
                vertices.push_back(v); // POOL OF VERTICES
            }

            if (line[0] == 'f')
            {
                int f[3];
                s >> dataType >> f[0] >> f[1] >> f[2]; // CREATE AND PUSH BACK TRIANGLE
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
    Mesh meshCube;
    Matrix matrixProjection; // Matrix that converts from view space to screen space
    Vec3d vCamera;           // Location of camera in world space
    Vec3d vLookDirection;    // Direction vector along the direction camera points
    float fYaw;              // FPS Camera rotation in XZ plane
    float fTheta;            // Spins World transform

    Vec3d Matrix_MultiplyVector(Matrix &m, Vec3d &i)
    {
        Vec3d v;
        v.x = i.x * m.matrix[0][0] + i.y * m.matrix[1][0] + i.z * m.matrix[2][0] + i.w * m.matrix[3][0];
        v.y = i.x * m.matrix[0][1] + i.y * m.matrix[1][1] + i.z * m.matrix[2][1] + i.w * m.matrix[3][1];
        v.z = i.x * m.matrix[0][2] + i.y * m.matrix[1][2] + i.z * m.matrix[2][2] + i.w * m.matrix[3][2];
        v.w = i.x * m.matrix[0][3] + i.y * m.matrix[1][3] + i.z * m.matrix[2][3] + i.w * m.matrix[3][3];
        return v;
    }

    Matrix Matrix_MakeIdentity()
    {
        Matrix matrix;
        matrix.matrix[0][0] = 1.0f;
        matrix.matrix[1][1] = 1.0f;
        matrix.matrix[2][2] = 1.0f;
        matrix.matrix[3][3] = 1.0f;
        return matrix;
    }

    Matrix Matrix_MakeRotationX(float fAngleRad)
    {
        Matrix matrix;
        matrix.matrix[0][0] = 1.0f;
        matrix.matrix[1][1] = cosf(fAngleRad);
        matrix.matrix[1][2] = sinf(fAngleRad);
        matrix.matrix[2][1] = -sinf(fAngleRad);
        matrix.matrix[2][2] = cosf(fAngleRad);
        matrix.matrix[3][3] = 1.0f;
        return matrix;
    }

    Matrix Matrix_MakeRotationY(float fAngleRad)
    {
        Matrix matrix;
        matrix.matrix[0][0] = cosf(fAngleRad);
        matrix.matrix[0][2] = sinf(fAngleRad);
        matrix.matrix[2][0] = -sinf(fAngleRad);
        matrix.matrix[1][1] = 1.0f;
        matrix.matrix[2][2] = cosf(fAngleRad);
        matrix.matrix[3][3] = 1.0f;
        return matrix;
    }

    Matrix Matrix_MakeRotationZ(float fAngleRad)
    {
        Matrix matrix;
        matrix.matrix[0][0] = cosf(fAngleRad);
        matrix.matrix[0][1] = sinf(fAngleRad);
        matrix.matrix[1][0] = -sinf(fAngleRad);
        matrix.matrix[1][1] = cosf(fAngleRad);
        matrix.matrix[2][2] = 1.0f;
        matrix.matrix[3][3] = 1.0f;
        return matrix;
    }

    Matrix Matrix_MakeTranslation(float x, float y, float z)
    {
        Matrix matrix;
        matrix.matrix[0][0] = 1.0f;
        matrix.matrix[1][1] = 1.0f;
        matrix.matrix[2][2] = 1.0f;
        matrix.matrix[3][3] = 1.0f;
        matrix.matrix[3][0] = x;
        matrix.matrix[3][1] = y;
        matrix.matrix[3][2] = z;
        return matrix;
    }

    Matrix Matrix_MakeProjection(float fFovDegrees, float fAspectRatio, float fNear, float fFar)
    {
        float fFovRad = 1.0f / tanf(fFovDegrees * 0.5f / 180.0f * 3.14159f);
        Matrix matrix;
        matrix.matrix[0][0] = fAspectRatio * fFovRad;
        matrix.matrix[1][1] = fFovRad;
        matrix.matrix[2][2] = fFar / (fFar - fNear);
        matrix.matrix[3][2] = (-fFar * fNear) / (fFar - fNear);
        matrix.matrix[2][3] = 1.0f;
        matrix.matrix[3][3] = 0.0f;
        return matrix;
    }

    Matrix Matrix_MultiplyMatrix(Matrix &m1, Matrix &m2)
    {
        Matrix matrix;
        for (int c = 0; c < 4; c++)
            for (int r = 0; r < 4; r++)
                matrix.matrix[r][c] = m1.matrix[r][0] * m2.matrix[0][c] + m1.matrix[r][1] * m2.matrix[1][c] + m1.matrix[r][2] * m2.matrix[2][c] + m1.matrix[r][3] * m2.matrix[3][c];
        return matrix;
    }

    Matrix Matrix_PointAt(Vec3d &pos, Vec3d &target, Vec3d &up)
    {
        // Calculate new forward direction
        Vec3d newForward = Vector_Sub(target, pos);
        newForward = Vector_Normalise(newForward);

        // Calculate new Up direction
        Vec3d a = Vector_Mul(newForward, Vector_DotProduct(up, newForward));
        Vec3d newUp = Vector_Sub(up, a);
        newUp = Vector_Normalise(newUp);

        // New Right direction is easy, its just cross product
        Vec3d newRight = Vector_CrossProduct(newUp, newForward);

        // Construct Dimensioning and Translation Matrix
        Matrix matrix;
        matrix.matrix[0][0] = newRight.x;
        matrix.matrix[0][1] = newRight.y;
        matrix.matrix[0][2] = newRight.z;
        matrix.matrix[0][3] = 0.0f;
        matrix.matrix[1][0] = newUp.x;
        matrix.matrix[1][1] = newUp.y;
        matrix.matrix[1][2] = newUp.z;
        matrix.matrix[1][3] = 0.0f;
        matrix.matrix[2][0] = newForward.x;
        matrix.matrix[2][1] = newForward.y;
        matrix.matrix[2][2] = newForward.z;
        matrix.matrix[2][3] = 0.0f;
        matrix.matrix[3][0] = pos.x;
        matrix.matrix[3][1] = pos.y;
        matrix.matrix[3][2] = pos.z;
        matrix.matrix[3][3] = 1.0f;
        return matrix;
    }

    Matrix Matrix_QuickInverse(Matrix &m) // Only for Rotation/Translation Matrices
    {
        Matrix matrix;
        matrix.matrix[0][0] = m.matrix[0][0];
        matrix.matrix[0][1] = m.matrix[1][0];
        matrix.matrix[0][2] = m.matrix[2][0];
        matrix.matrix[0][3] = 0.0f;
        matrix.matrix[1][0] = m.matrix[0][1];
        matrix.matrix[1][1] = m.matrix[1][1];
        matrix.matrix[1][2] = m.matrix[2][1];
        matrix.matrix[1][3] = 0.0f;
        matrix.matrix[2][0] = m.matrix[0][2];
        matrix.matrix[2][1] = m.matrix[1][2];
        matrix.matrix[2][2] = m.matrix[2][2];
        matrix.matrix[2][3] = 0.0f;
        matrix.matrix[3][0] = -(m.matrix[3][0] * matrix.matrix[0][0] + m.matrix[3][1] * matrix.matrix[1][0] + m.matrix[3][2] * matrix.matrix[2][0]);
        matrix.matrix[3][1] = -(m.matrix[3][0] * matrix.matrix[0][1] + m.matrix[3][1] * matrix.matrix[1][1] + m.matrix[3][2] * matrix.matrix[2][1]);
        matrix.matrix[3][2] = -(m.matrix[3][0] * matrix.matrix[0][2] + m.matrix[3][1] * matrix.matrix[1][2] + m.matrix[3][2] * matrix.matrix[2][2]);
        matrix.matrix[3][3] = 1.0f;
        return matrix;
    }

    Vec3d Vector_Add(Vec3d &v1, Vec3d &v2)
    {
        return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
    }

    Vec3d Vector_Sub(Vec3d &v1, Vec3d &v2)
    {
        return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
    }

    Vec3d Vector_Mul(Vec3d &v1, float k)
    {
        return {v1.x * k, v1.y * k, v1.z * k};
    }

    Vec3d Vector_Div(Vec3d &v1, float k)
    {
        return {v1.x / k, v1.y / k, v1.z / k};
    }

    float Vector_DotProduct(Vec3d &v1, Vec3d &v2)
    {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    float Vector_Length(Vec3d &v)
    {
        return sqrtf(Vector_DotProduct(v, v));
    }

    Vec3d Vector_Normalise(Vec3d &v)
    {
        float l = Vector_Length(v);
        return {v.x / l, v.y / l, v.z / l};
    }

    Vec3d Vector_CrossProduct(Vec3d &v1, Vec3d &v2)
    {
        Vec3d v;
        v.x = v1.y * v2.z - v1.z * v2.y;
        v.y = v1.z * v2.x - v1.x * v2.z;
        v.z = v1.x * v2.y - v1.y * v2.x;
        return v;
    }

    Vec3d Vector_IntersectPlane(Vec3d &plane_p, Vec3d &plane_n, Vec3d &lineStart, Vec3d &lineEnd)
    {
        plane_n = Vector_Normalise(plane_n);
        float plane_d = -Vector_DotProduct(plane_n, plane_p);
        float ad = Vector_DotProduct(lineStart, plane_n);
        float bd = Vector_DotProduct(lineEnd, plane_n);
        float t = (-plane_d - ad) / (bd - ad);
        Vec3d lineStartToEnd = Vector_Sub(lineEnd, lineStart);
        Vec3d lineToIntersect = Vector_Mul(lineStartToEnd, t);
        return Vector_Add(lineStart, lineToIntersect);
    }

    int Triangle_ClipAgainstPlane(Vec3d plane_p, Vec3d plane_n, Triangle &in_tri, Triangle &out_tri1, Triangle &out_tri2)
    {
        // Make sure plane normal is indeed normal
        plane_n = Vector_Normalise(plane_n);

        // Return signed shortest distance from point to plane, plane normal must be normalised
        auto dist = [&](Vec3d &p)
        {
            Vec3d n = Vector_Normalise(p);
            return (plane_n.x * p.x + plane_n.y * p.y + plane_n.z * p.z - Vector_DotProduct(plane_n, plane_p));
        };

        // Create two temporary storage arrays to classify points either side of plane
        // If distance sign is positive, point lies on "inside" of plane
        Vec3d *inside_points[3];
        Vec3d *outside_points[3];
        int nInsidePointCount = 0;
        int nOutsidePointCount = 0;

        // Get signed distance of each point in triangle to plane
        float d0 = dist(in_tri.points[0]);
        float d1 = dist(in_tri.points[1]);
        float d2 = dist(in_tri.points[2]);

        if (d0 >= 0)
        {
            inside_points[nInsidePointCount++] = &in_tri.points[0];
        }
        else
        {
            outside_points[nOutsidePointCount++] = &in_tri.points[0];
        }
        if (d1 >= 0)
        {
            inside_points[nInsidePointCount++] = &in_tri.points[1];
        }
        else
        {
            outside_points[nOutsidePointCount++] = &in_tri.points[1];
        }
        if (d2 >= 0)
        {
            inside_points[nInsidePointCount++] = &in_tri.points[2];
        }
        else
        {
            outside_points[nOutsidePointCount++] = &in_tri.points[2];
        }

        // Now classify triangle points, and break the input triangle into
        // smaller output triangles if required. There are four possible
        // outcomes...

        if (nInsidePointCount == 0)
        {
            // All points lie on the outside of plane, so clip whole triangle
            // It ceases to exist
            return 0; // No returned triangles are valid
        }

        if (nInsidePointCount == 3)
        {
            // All points lie on the inside of plane, so do nothing
            // and allow the triangle to simply pass through
            out_tri1 = in_tri;
            return 1; // Just the one returned original triangle is valid
        }

        if (nInsidePointCount == 1 && nOutsidePointCount == 2)
        {
            // Triangle should be clipped. As two points lie outside
            // the plane, the triangle simply becomes a smaller triangle

            // Copy appearance info to new triangle
            // out_tri1.col = in_tri.col;
            out_tri1.col = olc::Pixel(0, 0, 255);

            // The inside point is valid, so keep that...
            out_tri1.points[0] = *inside_points[0];

            // but the two new points are at the locations where the
            // original sides of the triangle (lines) intersect with the plane
            out_tri1.points[1] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);
            out_tri1.points[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[1]);

            return 1; // Return the newly formed single triangle
        }

        if (nInsidePointCount == 2 && nOutsidePointCount == 1)
        {
            // Triangle should be clipped. As two points lie inside the plane,
            // the clipped triangle becomes a "quad". Fortunately, we can
            // represent a quad with two new triangles

            // Copy appearance info to new triangles
            // out_tri1.col = in_tri.col;
            // out_tri2.col = in_tri.col;
            out_tri1.col = olc::Pixel(255, 0, 0);
            out_tri2.col = olc::Pixel(0, 255, 0);

            // The first triangle consists of the two inside points and a new
            // point determined by the location where one side of the triangle
            // intersects with the plane
            out_tri1.points[0] = *inside_points[0];
            out_tri1.points[1] = *inside_points[1];
            out_tri1.points[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);

            // The second triangle is composed of one of he inside points, a
            // new point determined by the intersection of the other side of the
            // triangle and the plane, and the newly created point above
            out_tri2.points[0] = *inside_points[1];
            out_tri2.points[1] = out_tri1.points[2];
            out_tri2.points[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[1], *outside_points[0]);

            return 2; // Return two newly formed triangles which form a quad
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
        // Called once at the start, create things here
        // meshCube.LoadFromObjectFile("assets/spaceship.obj");
        // meshCube.LoadFromObjectFile("assets/utahteapot.obj");
        // meshCube.LoadFromObjectFile("assets/axis.obj");
        meshCube.LoadFromObjectFile("assets/mountains.obj");

        // PROJECTION MATRIX
        float fNear = 0.1f;
        float fFar = 1000.0f;
        float fFOV = 90.0f;
        float fAspectRatio = (float)ScreenHeight() / (float)ScreenWidth();
        matrixProjection = Matrix_MakeProjection(fFOV, fAspectRatio, fNear, fFar);

        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        if (GetKey(olc::Key::UP).bHeld)
            vCamera.y += 8.0f * fElapsedTime; // Travel Upwards

        if (GetKey(olc::Key::DOWN).bHeld)
            vCamera.y -= 8.0f * fElapsedTime; // Travel Downwards

        // Dont use these two in FPS mode, it is confusing
        // if (GetKey(olc::Key::RIGHT).bHeld)
        //     vCamera.x -= 8.0f * fElapsedTime; // Travel Along X-Axis

        // if (GetKey(olc::Key::LEFT).bHeld)
        //     vCamera.x += 8.0f * fElapsedTime; // Travel Along X-Axis
        //                                       ///////

        Vec3d vForward = Vector_Mul(vLookDirection, 8.0f * fElapsedTime);
        // Standard FPS Control scheme, but turn instead of strafe
        if (GetKey(olc::Key::E).bHeld)
            vCamera = Vector_Add(vCamera, vForward);

        if (GetKey(olc::Key::D).bHeld)
            vCamera = Vector_Sub(vCamera, vForward);

        if (GetKey(olc::Key::S).bHeld)
            fYaw -= 2.0f * fElapsedTime;

        if (GetKey(olc::Key::F).bHeld)
            fYaw += 2.0f * fElapsedTime;

        // SET UP ROTATION MATRICES
        // fTheta += 1.0f * fElapsedTime;
        Matrix matrixRotateZ = Matrix_MakeRotationZ(fTheta * 0.5f);
        Matrix matrixRotateX = Matrix_MakeRotationX(fTheta);

        // TRANSLATE MATRIX
        Matrix matrixTranslated = Matrix_MakeTranslation(0.0f, 0.0f, 5.0f);

        // WORLD MATRIX
        Matrix matrixWorld;
        matrixWorld = Matrix_MakeIdentity();
        matrixWorld = Matrix_MultiplyMatrix(matrixRotateZ, matrixRotateX);  // ROTATE AROUND ORGIN
        matrixWorld = Matrix_MultiplyMatrix(matrixWorld, matrixTranslated); // TRANSLATE OBJECT TO A DIFFERENT LOCATION

        Vec3d vUp = {0, 1, 0};
        Vec3d vTarget = {0, 0, 1};
        Matrix matrixCameraRotation = Matrix_MakeRotationY(fYaw);
        vLookDirection = Matrix_MultiplyVector(matrixCameraRotation, vTarget);
        vTarget = Vector_Add(vCamera, vLookDirection);

        Matrix matrixCamera = Matrix_PointAt(vCamera, vTarget, vUp);

        // MAKE VEW MATRIX FROM CAMERA
        Matrix matrixView = Matrix_QuickInverse(matrixCamera);

        // STORE TRIANGLES FOR SORTING
        vector<Triangle>
            trianglesToRaster;

        // DRAW TRIANGLES
        for (auto triangle : meshCube.triangles)
        {
            Triangle triangleProjected;
            Triangle triangleTransformed;
            Triangle triangleViewed;

            // WORLD MATRIX TRANSFORM
            triangleTransformed.points[0] = Matrix_MultiplyVector(matrixWorld, triangle.points[0]);
            triangleTransformed.points[1] = Matrix_MultiplyVector(matrixWorld, triangle.points[1]);
            triangleTransformed.points[2] = Matrix_MultiplyVector(matrixWorld, triangle.points[2]);

            // CALCULATE TRIANGLE NORMAL
            // GET LINES EITHER SIDE OF TRIANGLE
            Vec3d lineOne = Vector_Sub(triangleTransformed.points[1], triangleTransformed.points[0]);
            Vec3d lineTwo = Vector_Sub(triangleTransformed.points[2], triangleTransformed.points[0]);
            // TAKE CROSS PRODUCT OF LINES TO GET NORMAL TO TRIANGLE SURFACE
            Vec3d normal = Vector_CrossProduct(lineOne, lineTwo);
            // NORMALIZE NORMAL
            normal = Vector_Normalise(normal);

            // Get Ray from triangle to camera
            Vec3d vCameraRay = Vector_Sub(triangleTransformed.points[0], vCamera);
            // If ray is aligned with normal, then triangle is visible
            if (Vector_DotProduct(normal, vCameraRay) < 0.0f)
            {
                // ILLUMINATION
                Vec3d lightDirection = {0.0f, 1.0f, -1.0f};
                // Vec3d lightDirection = {0.0f, 0.0f, -1.0f};
                lightDirection = Vector_Normalise(lightDirection);

                // How "aligned" are light direction and triangle surface normal?
                float dp = max(0.1f, Vector_DotProduct(lightDirection, normal));

                // Choose console colours as required
                triangleTransformed.col = GetColour(dp);

                // Convert World Space --> View Space
                triangleViewed.points[0] = Matrix_MultiplyVector(matrixView, triangleTransformed.points[0]);
                triangleViewed.points[1] = Matrix_MultiplyVector(matrixView, triangleTransformed.points[1]);
                triangleViewed.points[2] = Matrix_MultiplyVector(matrixView, triangleTransformed.points[2]);
                triangleViewed.col = triangleTransformed.col;

                // Clip Viewed Triangle against near plane, this could form two additional
                // additional triangles.
                int nClippedTriangles = 0;
                Triangle clipped[2];
                nClippedTriangles = Triangle_ClipAgainstPlane({0.0f, 0.0f, 0.1f}, {0.0f, 0.0f, 1.0f}, triangleViewed, clipped[0], clipped[1]);

                for (int index = 0; index < nClippedTriangles; index++)
                {

                    // PROJECT TRIANGLES FROM 3D ---> 2D
                    triangleProjected.points[0] = Matrix_MultiplyVector(matrixProjection, clipped[index].points[0]);
                    triangleProjected.points[1] = Matrix_MultiplyVector(matrixProjection, clipped[index].points[1]);
                    triangleProjected.points[2] = Matrix_MultiplyVector(matrixProjection, clipped[index].points[2]);
                    triangleProjected.col = clipped[index].col;

                    // Scale into view
                    triangleProjected.points[0] = Vector_Div(triangleProjected.points[0], triangleProjected.points[0].w);
                    triangleProjected.points[1] = Vector_Div(triangleProjected.points[1], triangleProjected.points[1].w);
                    triangleProjected.points[2] = Vector_Div(triangleProjected.points[2], triangleProjected.points[2].w);

                    // X/Y are inverted so put them back
                    triangleProjected.points[0].x *= -1.0f;
                    triangleProjected.points[1].x *= -1.0f;
                    triangleProjected.points[2].x *= -1.0f;
                    triangleProjected.points[0].y *= -1.0f;
                    triangleProjected.points[1].y *= -1.0f;
                    triangleProjected.points[2].y *= -1.0f;

                    // Offset verts into visible normalised space
                    Vec3d vOffsetView = {1, 1, 0};
                    triangleProjected.points[0] = Vector_Add(triangleProjected.points[0], vOffsetView);
                    triangleProjected.points[1] = Vector_Add(triangleProjected.points[1], vOffsetView);
                    triangleProjected.points[2] = Vector_Add(triangleProjected.points[2], vOffsetView);

                    triangleProjected.points[0].x *= 0.5f * (float)ScreenWidth();
                    triangleProjected.points[0].y *= 0.5f * (float)ScreenHeight();
                    triangleProjected.points[1].x *= 0.5f * (float)ScreenWidth();
                    triangleProjected.points[1].y *= 0.5f * (float)ScreenHeight();
                    triangleProjected.points[2].x *= 0.5f * (float)ScreenWidth();
                    triangleProjected.points[2].y *= 0.5f * (float)ScreenHeight();

                    // STORE TRIANGLES FOR SORTING
                    trianglesToRaster.push_back(triangleProjected);
                }
            }
        }

        // Clear(olc::BLACK);
        // SORT TRIANGLES FOR PAINTERS ALGO
        // CLIPPING DEBUG

        // sort(trianglesToRaster.begin(), trianglesToRaster.end(), [](Triangle &t1, Triangle &t2)
        //      {
        //     float z1 = (t1.points[0].z + t1.points[1].z + t1.points[2].z) / 3.0f;
        // 	float z2 = (t2.points[0].z + t2.points[1].z + t2.points[2].z) / 3.0f;
        // 	return z1 > z2; });

        // Loop through all transformed, viewed, projected, and sorted triangles
        for (auto &triangleToRaster : trianglesToRaster)
        {
            // Clip triangles against all four screen edges, this could yield
            // a bunch of triangles, so create a queue that we traverse to
            //  ensure we only test new triangles generated against planes
            Triangle clipped[2];
            std::list<Triangle> listTriangles;

            // Add initial triangle
            listTriangles.push_back(triangleToRaster);
            int nNewTriangles = 1;

            for (int p = 0; p < 4; p++)
            {
                int nTrisToAdd = 0;
                while (nNewTriangles > 0)
                {
                    // Take triangle from front of queue
                    Triangle test = listTriangles.front();
                    listTriangles.pop_front();
                    nNewTriangles--;

                    // Clip it against a plane. We only need to test each
                    // subsequent plane, against subsequent new triangles
                    // as all triangles after a plane clip are guaranteed
                    // to lie on the inside of the plane. I like how this
                    // comment is almost completely and utterly justified
                    switch (p)
                    {
                    case 0:
                        nTrisToAdd = Triangle_ClipAgainstPlane({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, test, clipped[0], clipped[1]);
                        break;
                    case 1:
                        nTrisToAdd = Triangle_ClipAgainstPlane({0.0f, (float)ScreenHeight() - 1, 0.0f}, {0.0f, -1.0f, 0.0f}, test, clipped[0], clipped[1]);
                        break;
                    case 2:
                        nTrisToAdd = Triangle_ClipAgainstPlane({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, test, clipped[0], clipped[1]);
                        break;
                    case 3:
                        nTrisToAdd = Triangle_ClipAgainstPlane({(float)ScreenWidth() - 1, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, test, clipped[0], clipped[1]);
                        break;
                    }

                    // Clipping may yield a variable number of triangles, so
                    // add these new ones to the back of the queue for subsequent
                    // clipping against next planes
                    for (int w = 0; w < nTrisToAdd; w++)
                        listTriangles.push_back(clipped[w]);
                }
                nNewTriangles = listTriangles.size();
            }

            // Draw the transformed, viewed, clipped, projected, sorted, clipped triangles
            for (auto &t : listTriangles)
            {
                FillTriangle(t.points[0].x, t.points[0].y, t.points[1].x, t.points[1].y, t.points[2].x, t.points[2].y, t.col);
                DrawTriangle(t.points[0].x, t.points[0].y, t.points[1].x, t.points[1].y, t.points[2].x, t.points[2].y, olc::BLACK);
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
