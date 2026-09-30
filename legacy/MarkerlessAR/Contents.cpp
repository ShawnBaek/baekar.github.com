#include "Contents.hpp"
#include "wonjo.h"

#include <algorithm>
#include <cmath>
#include <cfloat>

Contents::Contents() {}
Contents::~Contents() {}

static D3DXMATRIXA16 makeIdentity()
{
    D3DXMATRIXA16 m;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            m.m[r][c] = (r == c) ? 1.0f : 0.0f;
    return m;
}

int Contents::addWindowPlane(TextureID tex, float halfSize)
{
    ARContent c;
    c.kind     = ARContent::Plane;
    c.xform    = makeIdentity();
    // Scale the unit quad (-1..1) to halfSize.
    c.xform.m[0][0] = halfSize;
    c.xform.m[1][1] = halfSize;
    c.texture  = tex;
    c.selected = false;
    items_.push_back(c);
    return (int)items_.size() - 1;
}

void Contents::render() const
{
    for (size_t i = 0; i < items_.size(); ++i) {
        const ARContent& it = items_[i];
        if (it.kind != ARContent::Plane) continue;
        D3DXMATRIXA16 x = it.xform;
        wonjo_dx::DrawTexturedPlane(&x, it.texture, it.selected);
    }
}

void Contents::select(int idx)
{
    selected_ = idx;
    for (size_t i = 0; i < items_.size(); ++i)
        items_[i].selected = ((int)i == idx);
}

void Contents::deselectAll()
{
    selected_ = -1;
    for (auto& it : items_) it.selected = false;
}

void Contents::dragSelected(double dxPixels, double dyPixels)
{
    if (selected_ < 0 || selected_ >= (int)items_.size()) return;
    ARContent& it = items_[selected_];
    // Marker units are roughly mm-sized in this engine; scale cursor pixels
    // to a reasonable per-frame translation.
    const float k = 0.5f;
    it.xform.m[3][0] += (float)dxPixels * k;
    it.xform.m[3][1] -= (float)dyPixels * k;
}

// Ray-AABB intersection in plane-local space.
// Plane is the unit quad in z=0 (-1..1); transform the ray into the item's
// local frame by inverting xform, then test against [-1,1]x[-1,1] at z=0.
int Contents::pick(const D3DXVECTOR3& rayOrigin, const D3DXVECTOR3& rayDir) const
{
    int best = -1;
    float bestT = FLT_MAX;
    for (size_t i = 0; i < items_.size(); ++i) {
        const ARContent& it = items_[i];
        if (it.kind != ARContent::Plane) continue;

        D3DXMATRIXA16 inv;
        if (!D3DXMatrixInverse(&inv, NULL, const_cast<D3DXMATRIXA16*>(&it.xform))) continue;

        D3DXVECTOR3 lo = rayOrigin, ld = rayDir;
        D3DXVec3TransformCoord(&lo, &lo, &inv);
        D3DXVec3TransformNormal(&ld, &ld, &inv);

        if (std::fabs(ld.z) < 1e-6f) continue;
        float t = -lo.z / ld.z;
        if (t < 0.0f) continue;
        float hx = lo.x + t * ld.x;
        float hy = lo.y + t * ld.y;
        if (hx < -1.0f || hx > 1.0f || hy < -1.0f || hy > 1.0f) continue;
        if (t < bestT) { bestT = t; best = (int)i; }
    }
    return best;
}
