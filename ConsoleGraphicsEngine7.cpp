// CAN BE OPTIMIZED
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <vector>
#include <fstream>
#include <strstream>
#include <algorithm>
using namespace std;

struct Vec2d
{
    float u = 0;
    float v = 0;
    float w = 1;
};

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
    Vec2d tpoints[3];
    olc::Pixel col;
};

struct Mesh
{
    vector<Triangle> triangles;

    bool LoadFromObjectFile(string sFilename, bool bHasTexture = false)
    {
        ifstream file(sFilename);
        if (!file.is_open())
            return false;

        // Local cache of verts
        vector<Vec3d> vertices;
        vector<Vec2d> textures;

        while (!file.eof())
        {
            char line[128];
            file.getline(line, 128);

            strstream s;
            s << line;

            char dataType;
            if (line[0] == 'v')
            {
                if (line[1] == 't')
                {
                    Vec2d v;
                    s >> dataType >> dataType >> v.u >> v.v;
                    // A little hack for the spyro texture
					// v.u = 1.0f - v.u;
					v.v = 1.0f - v.v;
                    textures.push_back(v);
                }
                else
                {
                    Vec3d v;
                    s >> dataType >> v.x >> v.y >> v.z;
                    vertices.push_back(v); // POOL OF VERTICES
                }
            }

            if (!bHasTexture)
            {
                if (line[0] == 'f')
                {
                    int f[3];
                    s >> dataType >> f[0] >> f[1] >> f[2]; // CREATE AND PUSH BACK TRIANGLE
                    triangles.push_back({vertices[f[0] - 1], vertices[f[1] - 1], vertices[f[2] - 1]});
                }
            }
            else
            {

                if (line[0] == 'f')
                {
                    s >> dataType;

                    string tokens[6];
                    int nTokenCount = -1;

                    while (!s.eof())
                    {
                        char c = s.get();
                        if (c == ' ' || c == '/')
                            nTokenCount++;
                        else
                            tokens[nTokenCount].append(1, c);
                    }

                    tokens[nTokenCount].pop_back();

                    triangles.push_back({vertices[stoi(tokens[0]) - 1], vertices[stoi(tokens[2]) - 1], vertices[stoi(tokens[4]) - 1],
                                         textures[stoi(tokens[1]) - 1], textures[stoi(tokens[3]) - 1], textures[stoi(tokens[5]) - 1]});
                }
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
    olc::Sprite *sprTex1;

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

    Vec3d Vector_IntersectPlane(Vec3d &plane_p, Vec3d &plane_n, Vec3d &lineStart, Vec3d &lineEnd, float &t)
    {
        plane_n = Vector_Normalise(plane_n);
        float plane_d = -Vector_DotProduct(plane_n, plane_p);
        float ad = Vector_DotProduct(lineStart, plane_n);
        float bd = Vector_DotProduct(lineEnd, plane_n);
        t = (-plane_d - ad) / (bd - ad);
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
        int nInsidePointCount = 0;
        Vec3d *outside_points[3];
        int nOutsidePointCount = 0;
        Vec2d *inside_tex[3];
        int nInsideTexCount = 0;
        Vec2d *outside_tex[3];
        int nOutsideTexCount = 0;

        // Get signed distance of each point in triangle to plane
        float d0 = dist(in_tri.points[0]);
        float d1 = dist(in_tri.points[1]);
        float d2 = dist(in_tri.points[2]);

        if (d0 >= 0)
        {
            inside_points[nInsidePointCount++] = &in_tri.points[0];
            inside_tex[nInsideTexCount++] = &in_tri.tpoints[0];
        }
        else
        {
            outside_points[nOutsidePointCount++] = &in_tri.points[0];
            outside_tex[nOutsideTexCount++] = &in_tri.tpoints[0];
        }
        if (d1 >= 0)
        {
            inside_points[nInsidePointCount++] = &in_tri.points[1];
            inside_tex[nInsideTexCount++] = &in_tri.tpoints[1];
        }
        else
        {
            outside_points[nOutsidePointCount++] = &in_tri.points[1];
            outside_tex[nOutsideTexCount++] = &in_tri.tpoints[1];
        }
        if (d2 >= 0)
        {
            inside_points[nInsidePointCount++] = &in_tri.points[2];
            inside_tex[nInsideTexCount++] = &in_tri.tpoints[2];
        }
        else
        {
            outside_points[nOutsidePointCount++] = &in_tri.points[2];
            outside_tex[nOutsideTexCount++] = &in_tri.tpoints[2];
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
            out_tri1.col = in_tri.col;

            // The inside point is valid, so keep that...
            out_tri1.points[0] = *inside_points[0];
            out_tri1.tpoints[0] = *inside_tex[0];

            // but the two new points are at the locations where the
            // original sides of the triangle (lines) intersect with the plane
            float t;
            out_tri1.points[1] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0], t);
            out_tri1.tpoints[1].u = t * (outside_tex[0]->u - inside_tex[0]->u) + inside_tex[0]->u;
            out_tri1.tpoints[1].v = t * (outside_tex[0]->v - inside_tex[0]->v) + inside_tex[0]->v;
            out_tri1.tpoints[1].w = t * (outside_tex[0]->w - inside_tex[0]->w) + inside_tex[0]->w;

            out_tri1.points[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[1], t);
            out_tri1.tpoints[2].u = t * (outside_tex[1]->u - inside_tex[0]->u) + inside_tex[0]->u;
            out_tri1.tpoints[2].v = t * (outside_tex[1]->v - inside_tex[0]->v) + inside_tex[0]->v;
            out_tri1.tpoints[2].w = t * (outside_tex[1]->w - inside_tex[0]->w) + inside_tex[0]->w;

            return 1; // Return the newly formed single triangle
        }

        if (nInsidePointCount == 2 && nOutsidePointCount == 1)
        {
            // Triangle should be clipped. As two points lie inside the plane,
            // the clipped triangle becomes a "quad". Fortunately, we can
            // represent a quad with two new triangles

            // Copy appearance info to new triangles
            out_tri1.col = in_tri.col;
            out_tri2.col = in_tri.col;

            // The first triangle consists of the two inside points and a new
            // point determined by the location where one side of the triangle
            // intersects with the plane
            out_tri1.points[0] = *inside_points[0];
            out_tri1.points[1] = *inside_points[1];
            out_tri1.tpoints[0] = *inside_tex[0];
            out_tri1.tpoints[1] = *inside_tex[1];

            float t;
            out_tri1.points[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0], t);
            out_tri1.tpoints[2].u = t * (outside_tex[0]->u - inside_tex[0]->u) + inside_tex[0]->u;
            out_tri1.tpoints[2].v = t * (outside_tex[0]->v - inside_tex[0]->v) + inside_tex[0]->v;
            out_tri1.tpoints[2].w = t * (outside_tex[0]->w - inside_tex[0]->w) + inside_tex[0]->w;

            // The second triangle is composed of one of he inside points, a
            // new point determined by the intersection of the other side of the
            // triangle and the plane, and the newly created point above
            out_tri2.points[0] = *inside_points[1];
            out_tri2.tpoints[0] = *inside_tex[1];
            out_tri2.points[1] = out_tri1.points[2];
            out_tri2.tpoints[1] = out_tri1.tpoints[2];
            out_tri2.points[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[1], *outside_points[0], t);
            out_tri2.tpoints[2].u = t * (outside_tex[0]->u - inside_tex[1]->u) + inside_tex[1]->u;
            out_tri2.tpoints[2].v = t * (outside_tex[0]->v - inside_tex[1]->v) + inside_tex[1]->v;
            out_tri2.tpoints[2].w = t * (outside_tex[0]->w - inside_tex[1]->w) + inside_tex[1]->w;
            return 2; // Return two newly formed triangles which form a quad
        }
    }

    // Input parameter lum must be between 0 and 1 - i.e. [0, 1]
    olc::Pixel GetColour(float lum)
    {
        int nValue = (int)(std::max(lum, 0.20f) * 255.0f);
        return olc::Pixel(nValue, nValue, nValue);
    }

    float *pDepthBuffer = nullptr;

public:
    bool OnUserCreate() override
    {
        pDepthBuffer = new float[ScreenWidth() * ScreenHeight()];
        // meshCube.LoadFromObjectFile("assets/spaceship.obj");
        // meshCube.LoadFromObjectFile("assets/utahteapot.obj");
        // meshCube.LoadFromObjectFile("assets/axis.obj");
        // meshCube.LoadFromObjectFile("assets/mountains.obj");

        // meshCube.triangles = {
        //     // SOUTH
        //     {
        //         0.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //     },
        //     {
        //         0.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //     },
        //     // EAST
        //     {
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //     },
        //     {
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //     },
        //     // NORTH
        //     {
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //     },
        //     {
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //     },
        //     // WEST
        //     {
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //     },
        //     {
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //     },
        //     // TOP
        //     {
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //     },
        //     {
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //     },
        //     // BOTTOM
        //     {
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //     },
        //     {
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         0.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         0.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //         1.0f,
        //     },
        // };
   
        // sprTex1 = new olc::Sprite("assets/Jario.png");
        // sprTex1 = new olc::Sprite("assets/avatar.png");

        // meshCube.LoadFromObjectFile("assets/test/untitled.obj", true);
        // sprTex1 = new olc::Sprite("assets/test/High.png");
        meshCube.LoadFromObjectFile("assets/test2/untitled.obj", true);
        sprTex1 = new olc::Sprite("assets/test2/textures3.png");

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
        float moveSpeed = 300.0f;

        if (GetKey(olc::Key::UP).bHeld)
            vCamera.y += moveSpeed * fElapsedTime; // Travel Upwards

        if (GetKey(olc::Key::DOWN).bHeld)
            vCamera.y -= moveSpeed * fElapsedTime; // Travel Downwards

        // Dont use these two in FPS mode, it is confusing
        if (GetKey(olc::Key::RIGHT).bHeld)
            vCamera.x -= moveSpeed * fElapsedTime; // Travel Along X-Axis

        if (GetKey(olc::Key::LEFT).bHeld)
            vCamera.x += moveSpeed * fElapsedTime; // Travel Along X-Axis
        //                                       ///////

        Vec3d vForward = Vector_Mul(vLookDirection, moveSpeed * fElapsedTime);
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
            triangleTransformed.tpoints[0] = triangle.tpoints[0];
            triangleTransformed.tpoints[1] = triangle.tpoints[1];
            triangleTransformed.tpoints[2] = triangle.tpoints[2];

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
                triangleViewed.tpoints[0] = triangleTransformed.tpoints[0];
                triangleViewed.tpoints[1] = triangleTransformed.tpoints[1];
                triangleViewed.tpoints[2] = triangleTransformed.tpoints[2];

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
                    triangleProjected.tpoints[0] = clipped[index].tpoints[0];
                    triangleProjected.tpoints[1] = clipped[index].tpoints[1];
                    triangleProjected.tpoints[2] = clipped[index].tpoints[2];

                    triangleProjected.tpoints[0].u = triangleProjected.tpoints[0].u / triangleProjected.points[0].w;
                    triangleProjected.tpoints[1].u = triangleProjected.tpoints[1].u / triangleProjected.points[1].w;
                    triangleProjected.tpoints[2].u = triangleProjected.tpoints[2].u / triangleProjected.points[2].w;

                    triangleProjected.tpoints[0].v = triangleProjected.tpoints[0].v / triangleProjected.points[0].w;
                    triangleProjected.tpoints[1].v = triangleProjected.tpoints[1].v / triangleProjected.points[1].w;
                    triangleProjected.tpoints[2].v = triangleProjected.tpoints[2].v / triangleProjected.points[2].w;

                    triangleProjected.tpoints[0].w = 1.0f / triangleProjected.points[0].w;
                    triangleProjected.tpoints[1].w = 1.0f / triangleProjected.points[1].w;
                    triangleProjected.tpoints[2].w = 1.0f / triangleProjected.points[2].w;

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

        // // SORT TRIANGLES PAINTERS ALGO
        sort(trianglesToRaster.begin(), trianglesToRaster.end(), [](Triangle &t1, Triangle &t2)
             {
            float z1 = (t1.points[0].z + t1.points[1].z + t1.points[2].z) / 3.0f;
        	float z2 = (t2.points[0].z + t2.points[1].z + t2.points[2].z) / 3.0f;
        	return z1 > z2; });

        // Clear Screen  
        // Clear(olc::WHITE);
        FillRect(0, 0, ScreenWidth(), ScreenHeight(), olc::CYAN);

        // Clear Depth Buffer
        for (int i = 0; i < ScreenWidth() * ScreenHeight(); i++)
            pDepthBuffer[i] = 0.0f;

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

                TexturedTriangle(t.points[0].x, t.points[0].y, t.tpoints[0].u, t.tpoints[0].v, t.tpoints[0].w,
                                 t.points[1].x, t.points[1].y, t.tpoints[1].u, t.tpoints[1].v, t.tpoints[1].w,
                                 t.points[2].x, t.points[2].y, t.tpoints[2].u, t.tpoints[2].v, t.tpoints[2].w,
                                 sprTex1);

                // FillTriangle(t.points[0].x, t.points[0].y, t.points[1].x, t.points[1].y, t.points[2].x, t.points[2].y, t.col);
                // DrawTriangle(t.points[0].x, t.points[0].y, t.points[1].x, t.points[1].y, t.points[2].x, t.points[2].y, olc::BLACK);
                // DrawTriangle(t.points[0].x, t.points[0].y, t.points[1].x, t.points[1].y, t.points[2].x, t.points[2].y, olc::WHITE);
            }
        }

        return true;
    }

    void TexturedTriangle(int x1, int y1, float u1, float v1, float w1,
                          int x2, int y2, float u2, float v2, float w2,
                          int x3, int y3, float u3, float v3, float w3,
                          olc::Sprite *tex)
    {
        if (y2 < y1)
        {
            std::swap(y1, y2);
            std::swap(x1, x2);
            std::swap(u1, u2);
            std::swap(v1, v2);
            std::swap(w1, w2);
        }

        if (y3 < y1)
        {
            std::swap(y1, y3);
            std::swap(x1, x3);
            std::swap(u1, u3);
            std::swap(v1, v3);
            std::swap(w1, w3);
        }

        if (y3 < y2)
        {
            std::swap(y2, y3);
            std::swap(x2, x3);
            std::swap(u2, u3);
            std::swap(v2, v3);
            std::swap(w2, w3);
        }

        int dy1 = y2 - y1;
        int dx1 = x2 - x1;
        float dv1 = v2 - v1;
        float du1 = u2 - u1;
        float dw1 = w2 - w1;

        int dy2 = y3 - y1;
        int dx2 = x3 - x1;
        float dv2 = v3 - v1;
        float du2 = u3 - u1;
        float dw2 = w3 - w1;

        float tex_u, tex_v, tex_w;

        float dax_step = 0, dbx_step = 0,
              du1_step = 0, dv1_step = 0,
              du2_step = 0, dv2_step = 0,
              dw1_step = 0, dw2_step = 0;

        if (dy1)
            dax_step = dx1 / (float)abs(dy1);
        if (dy2)
            dbx_step = dx2 / (float)abs(dy2);

        if (dy1)
            du1_step = du1 / (float)abs(dy1);
        if (dy1)
            dv1_step = dv1 / (float)abs(dy1);
        if (dy1)
            dw1_step = dw1 / (float)abs(dy1);

        if (dy2)
            du2_step = du2 / (float)abs(dy2);
        if (dy2)
            dv2_step = dv2 / (float)abs(dy2);
        if (dy2)
            dw2_step = dw2 / (float)abs(dy2);

        if (dy1)
        {
            for (int i = y1; i <= y2; i++)
            {
                int ax = x1 + (float)(i - y1) * dax_step;
                int bx = x1 + (float)(i - y1) * dbx_step;

                float tex_su = u1 + (float)(i - y1) * du1_step;
                float tex_sv = v1 + (float)(i - y1) * dv1_step;
                float tex_sw = w1 + (float)(i - y1) * dw1_step;

                float tex_eu = u1 + (float)(i - y1) * du2_step;
                float tex_ev = v1 + (float)(i - y1) * dv2_step;
                float tex_ew = w1 + (float)(i - y1) * dw2_step;

                if (ax > bx)
                {
                    std::swap(ax, bx);
                    std::swap(tex_su, tex_eu);
                    std::swap(tex_sv, tex_ev);
                    std::swap(tex_sw, tex_ew);
                }

                tex_u = tex_su;
                tex_v = tex_sv;
                tex_w = tex_sw;

                float tstep = 1.0f / ((float)(bx - ax));
                float t = 0.0f;

                for (int j = ax; j < bx; j++)
                {
                    tex_u = (1.0f - t) * tex_su + t * tex_eu;
                    tex_v = (1.0f - t) * tex_sv + t * tex_ev;
                    tex_w = (1.0f - t) * tex_sw + t * tex_ew;
                    if (tex_w > pDepthBuffer[i * ScreenWidth() + j])
                    {
                        Draw(j, i, tex->Sample(tex_u / tex_w, tex_v / tex_w));
                        pDepthBuffer[i * ScreenWidth() + j] = tex_w;
                    }
                    t += tstep;
                }
            }
        }

        dy1 = y3 - y2;
        dx1 = x3 - x2;
        dv1 = v3 - v2;
        du1 = u3 - u2;
        dw1 = w3 - w2;

        if (dy1)
            dax_step = dx1 / (float)abs(dy1);
        if (dy2)
            dbx_step = dx2 / (float)abs(dy2);

        du1_step = 0, dv1_step = 0;
        if (dy1)
            du1_step = du1 / (float)abs(dy1);
        if (dy1)
            dv1_step = dv1 / (float)abs(dy1);
        if (dy1)
            dw1_step = dw1 / (float)abs(dy1);

        if (dy1)
        {
            for (int i = y2; i <= y3; i++)
            {
                int ax = x2 + (float)(i - y2) * dax_step;
                int bx = x1 + (float)(i - y1) * dbx_step;

                float tex_su = u2 + (float)(i - y2) * du1_step;
                float tex_sv = v2 + (float)(i - y2) * dv1_step;
                float tex_sw = w2 + (float)(i - y2) * dw1_step;

                float tex_eu = u1 + (float)(i - y1) * du2_step;
                float tex_ev = v1 + (float)(i - y1) * dv2_step;
                float tex_ew = w1 + (float)(i - y1) * dw2_step;

                if (ax > bx)
                {
                    std::swap(ax, bx);
                    std::swap(tex_su, tex_eu);
                    std::swap(tex_sv, tex_ev);
                    std::swap(tex_sw, tex_ew);
                }

                tex_u = tex_su;
                tex_v = tex_sv;
                tex_w = tex_sw;

                float tstep = 1.0f / ((float)(bx - ax));
                float t = 0.0f;

                for (int j = ax; j < bx; j++)
                {
                    tex_u = (1.0f - t) * tex_su + t * tex_eu;
                    tex_v = (1.0f - t) * tex_sv + t * tex_ev;
                    tex_w = (1.0f - t) * tex_sw + t * tex_ew;

                    if (tex_w > pDepthBuffer[i * ScreenWidth() + j])
                    {
                        Draw(j, i, tex->Sample(tex_u / tex_w, tex_v / tex_w));
                        pDepthBuffer[i * ScreenWidth() + j] = tex_w;
                    }
                    t += tstep;
                }
            }
        }
    }
};

int main()
{
    ConsoleGameEngine demo;
    if (demo.Construct(256, 240, 4, 4))
        demo.Start();
    return 0;
}
