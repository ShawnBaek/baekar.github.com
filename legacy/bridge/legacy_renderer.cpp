// Moved from EngineMain.cpp: init(), DrawMultiMarkerDetections() and the
// drawing parts of mainLoop(). Drawing order and GL state are unchanged.

#include "legacy_renderer.h"

#include "wonjo.h"

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>

// Stanford bunny vertex/normal arrays used by the 2012 GL setup.
#include "3dobjects/bunny/inc/bunny.h"

#define EARTH_TEXTURE_FILENAME "3dobjects/earthTexture.jpg"

namespace legacy_render {
namespace {

GLuint gnEarthTexID = 0;

D3DXMATRIXA16 toD3DX(const Matrix& m) {
    D3DXMATRIXA16 out;
    for (int i = 0; i < 16; ++i) out[i] = m[i];
    return out;
}

}  // namespace

void Initialize(int* argc, char** argv) {
    // GLUT supplies glutSolidCone and bitmap fonts used by the overlays.
    glutInit(argc, argv);
    // Initialize D3D stub (allocates static gpDevice so GetDevice() is non-null)
    wonjo_dx::AAR3DInitD3D(nullptr);

    // turn on z-buffering, so we get proper occlusion
    glEnable( GL_DEPTH_TEST );
    // properly scale normal vectors
    glEnable( GL_NORMALIZE );
    // turn on default lighting
    glEnable( GL_LIGHTING );

    // light 0
    GLfloat light_position[] = { 100.0, 500, 200, 1.0 };
    GLfloat white_light[] = { 1.0, 1.0, 1.0, 0.8 };
    GLfloat lmodel_ambient[] = { 0.9, 0.9, 0.9, 0.5 };

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_SMOOTH);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
    glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glEnable(GL_LIGHT0);

    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_TEXTURE_2D);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    // Init Earth Texture
    glGenTextures( 1, &gnEarthTexID );
    glBindTexture( GL_TEXTURE_2D, gnEarthTexID );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, 0 );
    cv::Mat earth = cv::imread( EARTH_TEXTURE_FILENAME, cv::IMREAD_COLOR );
    if (!earth.empty()) {
        cv::cvtColor( earth, earth, cv::COLOR_BGR2RGB );
        cv::flip( earth, earth, 0 );
        glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, earth.cols, earth.rows, GL_RGB, GL_UNSIGNED_BYTE, earth.data );
    } else {
        fprintf(stderr, "Warning: could not load texture '%s'\n", EARTH_TEXTURE_FILENAME);
    }

    // Init bunny
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glVertexPointer(3,GL_FLOAT,0,bunny);
    glNormalPointer(GL_FLOAT,0,normals);
}

void BeginFrame() { wonjo_dx::BeginRender(); }

void DrawBackground(const cv::Mat& bgr) {
    // The 2012 loop flipped the HandyAR display image vertically and then
    // both ways before uploading it (net: horizontal); the preview quad
    // flips again in texture space.
    cv::Mat image = bgr.clone();
    cv::flip(image, image, 0);
    cv::flip(image, image, -1);
    wonjo_dx::AAR3DDrawCameraPreview(reinterpret_cast<char*>(image.data), 640, 480);
}

void DrawOutlines(const std::vector<Outline>& outlines) {
    static const float colors[][3] = {
        {0.10f, 0.95f, 1.00f},
        {1.00f, 0.25f, 0.75f},
        {1.00f, 0.90f, 0.15f},
        {0.25f, 1.00f, 0.35f},
        {1.00f, 0.50f, 0.10f},
        {0.55f, 0.45f, 1.00f}
    };

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 640, 480, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glLineWidth(4.0f);

    for (const Outline& outline : outlines) {
        const float* color = colors[outline.index % (sizeof(colors) / sizeof(colors[0]))];
        glColor3f(color[0], color[1], color[2]);
        glBegin(GL_LINE_LOOP);
        for (const cv::Point2f& corner : outline.corners)
            glVertex2f(corner.x, corner.y);
        glEnd();

        const std::string status = outline.tracking
            ? std::to_string(outline.trackedPoints) + " tracked"
            : std::to_string(outline.inliers) + " inliers";
        const std::string label = std::to_string(outline.index + 1) + "  " + outline.name + "  " + status;
        const float labelX = outline.corners[0].x;
        const float labelY = std::max(14.0f, outline.corners[0].y - 8.0f);
        glRasterPos2f(labelX, labelY);
        for (char character : label)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, character);
    }

    glPopAttrib();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void SetProjection(const Matrix& projection) {
    const D3DXMATRIXA16 matProj = toD3DX(projection);
    wonjo_dx::SetProjectionMatrix(&matProj);
}

void SetView(const Matrix& view) {
    const D3DXMATRIXA16 matView = toD3DX(view);
    wonjo_dx::SetModelViewMatrix(&matView);
}

void DrawMarkerAnchor(const std::array<cv::Point2f, 4>& markerCorners, const Matrix& projection,
                      const Matrix& view) {
    const D3DXMATRIXA16 matProj = toD3DX(projection);
    const D3DXMATRIXA16 matView = toD3DX(view);
    wonjo_dx::SetModelViewMatrix(&matView);

    static int matLogTick = 0;
    if (++matLogTick % 30 == 0) {
        fprintf(stderr, "matProj:\n  %g %g %g %g\n  %g %g %g %g\n  %g %g %g %g\n  %g %g %g %g\n",
            matProj.m[0][0], matProj.m[0][1], matProj.m[0][2], matProj.m[0][3],
            matProj.m[1][0], matProj.m[1][1], matProj.m[1][2], matProj.m[1][3],
            matProj.m[2][0], matProj.m[2][1], matProj.m[2][2], matProj.m[2][3],
            matProj.m[3][0], matProj.m[3][1], matProj.m[3][2], matProj.m[3][3]);
        fprintf(stderr, "matView:\n  %g %g %g %g\n  %g %g %g %g\n  %g %g %g %g\n  %g %g %g %g\n",
            matView.m[0][0], matView.m[0][1], matView.m[0][2], matView.m[0][3],
            matView.m[1][0], matView.m[1][1], matView.m[1][2], matView.m[1][3],
            matView.m[2][0], matView.m[2][1], matView.m[2][2], matView.m[2][3],
            matView.m[3][0], matView.m[3][1], matView.m[3][2], matView.m[3][3]);
        fflush(stderr);
    }

    // AR overlay anchored to the matching corners (screen-space). The 2012
    // pose matrices are D3D left-handed; the world-space path below does not
    // land on the marker in OpenGL, so the mesh is also drawn at the
    // marker's centroid, sized by its diagonal in screen pixels.
    float cx = 0, cy = 0;
    for (int k = 0; k < 4; ++k) {
        cx += markerCorners[k].x;
        cy += markerCorners[k].y;
    }
    cx *= 0.25f;
    cy *= 0.25f;
    const float dx = markerCorners[0].x - markerCorners[2].x;
    const float dy = markerCorners[0].y - markerCorners[2].y;
    const float diag = sqrtf(dx*dx + dy*dy);
    const float meshScale = diag * 0.20f;

    if (diag > 20.0f && diag < 1500.0f) {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, 640, 480, 0, -1000, 1000);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // Yellow tracking rectangle in the same coordinates as the mesh.
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 0.0f);
        glLineWidth(4.0f);
        glBegin(GL_LINE_LOOP);
            for (int k = 0; k < 4; ++k)
                glVertex2f(markerCorners[k].x, markerCorners[k].y);
        glEnd();
        glLineWidth(1.0f);

        // Arrow3Axis.X (Assimp) at the marker centroid, spinning slowly
        // around screen-space Y so its depth is visible.
        glTranslatef(cx, cy, 0);
        static float spinDeg = 0;
        spinDeg += 1.2f;
        glRotatef(spinDeg, 0, 1, 0);
        glScalef(meshScale, -meshScale, meshScale);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        GLfloat lp[4] = { 0.5f, 0.5f, 1.0f, 0.0f };
        GLfloat ld[4] = { 1, 1, 1, 1 };
        GLfloat la[4] = { 0.3f, 0.3f, 0.3f, 1 };
        glLightfv(GL_LIGHT0, GL_POSITION, lp);
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  ld);
        glLightfv(GL_LIGHT0, GL_AMBIENT,  la);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

        D3DXMATRIXA16 ident;  // identity: draw at the current modelview
        wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("Arrow3Axis.X"), &ident);

        glDisable(GL_LIGHTING);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

    // The 2012 world-space plane and axis mesh in the marker pose.
    wonjo_dx::SetProjectionMatrix(&matProj);
    wonjo_dx::SetModelViewMatrix(&matView);
    wonjo_dx::DrawPlane(wonjo_dx::MakeScaleMatrix(35,35,35));
    wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("Arrow3Axis.X"),wonjo_dx::MakeScaleMatrix(10,10,10));
}

void DrawHand(const Matrix& projection, const Matrix& view) {
    const D3DXMATRIXA16 matProj = toD3DX(projection);
    const D3DXMATRIXA16 matView = toD3DX(view);
    wonjo_dx::SetProjectionMatrix(&matProj);
    wonjo_dx::SetModelViewMatrix(&matView);
    wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("Arrow3Axis.X"), wonjo_dx::MakeScaleMatrix(10.0f,10.0f,10.0f));
    // z축으로 조금 내림...
    D3DXMATRIXA16 matWorld = *wonjo_dx::MakeScaleMatrix(5.0f,5.0f,5.0f) * (*wonjo_dx::MakeTranslationMatrix(0,0,200));
    wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("hand001.X"), &matWorld);
}

void EndFrame() { wonjo_dx::Flip(); }

}  // namespace legacy_render
