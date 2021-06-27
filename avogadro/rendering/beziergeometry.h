/******************************************************************************
  This source file is part of the Avogadro project.
  This source code is released under the 3-Clause BSD License, (see "LICENSE").
******************************************************************************/

#ifndef AVOGADRO_RENDERING_BEZIERGEOMETRY_H
#define AVOGADRO_RENDERING_BEZIERGEOMETRY_H

#include "curvegeometry.h"

namespace Avogadro {
namespace Rendering {

class AVOGADRORENDERING_EXPORT BezierGeometry : public CurveGeometry
{
public:
  BezierGeometry();

protected:
  void update(int index) override;
  Vector3f computeCurvePoint(float t, const std::list<Point*>& points) override;
  std::multimap<float, Identifier> hits(const Vector3f&, const Vector3f&,
                                        const Vector3f&) const override;
};

} // End namespace Rendering
} // End namespace Avogadro

#endif
