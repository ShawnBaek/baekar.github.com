#pragma once

#ifdef _WIN32
#include <d3dx9.h>
#else
#include "compat/d3d_stub.h"
#endif

#include <vector>

#ifdef __APPLE__
#include <OpenGL/gl.h>
typedef GLuint TextureID;
#else
typedef unsigned int TextureID;
#endif

// Represents one piece of AR content drawn in marker-local space.
// Currently only Plane (textured quad) is wired up; Mesh is a placeholder.
struct ARContent
{
    enum Kind { Plane, Mesh };
    Kind            kind     = Plane;
    D3DXMATRIXA16   xform;                 // local transform (relative to marker pose)
    TextureID       texture  = 0;          // for Plane: BGRA texture (e.g. window stream)
    bool            selected = false;
};

// Minimal scene-object container with picking + interaction.
// Single global instance lives in EngineMain.cpp.
class Contents
{
public:
    Contents();
    ~Contents();

    // Add a textured plane that lives at marker-local origin (caller may
    // adjust .xform afterward). Returns the new index.
    int addWindowPlane(TextureID tex, float halfSize = 50.0f);

    // Render every item using the currently-bound projection/view matrices.
    void render() const;

    // Hit-test ray (already in world space) against each Plane's AABB.
    // Returns index of nearest hit or -1.
    int pick(const D3DXVECTOR3& rayOrigin, const D3DXVECTOR3& rayDir) const;

    // Selection + drag interaction.
    void select(int idx);
    void deselectAll();
    int  selectedIndex() const { return selected_; }
    // Translate the selected plane in marker-local XY (pixels of cursor delta).
    void dragSelected(double dxPixels, double dyPixels);

    // Direct access (read-only) for renderers that want per-item state.
    const std::vector<ARContent>& items() const { return items_; }

private:
    std::vector<ARContent> items_;
    int                    selected_ = -1;
};
