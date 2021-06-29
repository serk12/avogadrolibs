/******************************************************************************
  This source file is part of the Avogadro project.
  This source code is released under the 3-Clause BSD License, (see "LICENSE").
******************************************************************************/

#include "beziergeometry.h"

namespace Avogadro {
namespace Rendering {

BezierGeometry::BezierGeometry() : CurveGeometry() {}

Vector3f BezierGeometry::computeCurvePoint(float t,
                                           const std::list<Point*>& points)
{
  Vector3f h;
  h << 1.0f, 1.0f, 1.0f;
  float u = 1.0f - t;
  float n1 = points.size();
  float w = 1.0f / n1;
  float k = 0.0f;
  Vector3f Q;
  Q << w, w, w;
  for (const auto& p : points) {
    for (size_t i = 0; i < 3; ++i) {
      h[i] = h[i] * t * (n1 - k) * w;
      h[i] = h[i] / (k * u * w + h[i]);
      Q[i] = (1.0f - h[i]) * Q[i] + h[i] * p->pos[i];
    }
    k += 1.0f;
  }
  return Q;
}

void BezierGeometry::update(int index)
{
  // compute the intermidian bezier points
  Line* line = m_lines[index];
  unsigned int lineResolution = 12;
  size_t qttyPoints = line->points.size();
  size_t qttySegments = lineResolution * qttyPoints;
  std::vector<Vector3f> points(qttySegments);
  for (size_t i = 0; i < qttyPoints; ++i) {
    for (size_t j = 0; j < lineResolution; ++j) {
      points[i * lineResolution + j] = computeCurvePoint(
        (i * lineResolution + j) / float(qttySegments), line->points);
    }
  }

  // prepare VBO and EBO
  std::vector<unsigned int> indices;
  std::vector<ColorNormalVertex> vertices;
  unsigned int circleResolution = 12;
  const float resolutionRadians =
    2.0f * static_cast<float>(M_PI) / static_cast<float>(circleResolution);
  std::vector<Vector3f> radials(circleResolution);

  auto it = line->points.begin();
  float radius = line->radius;
  for (size_t i = 1; i < qttySegments; ++i) {
    if (i % lineResolution == 0) {
      ++it;
    }
    const Point* point = *it;
    const Vector3f& position1 = points[i - 1];
    const Vector3f& position2 = points[i];
    const Vector3f direction = (position2 - position1).normalized();

    Vector3f radial = direction.unitOrthogonal() * radius;
    Eigen::AngleAxisf transform(resolutionRadians, direction);
    for (unsigned int j = 0; j < circleResolution; ++j) {
      radials[j] = radial;
      radial = transform * radial;
    }

    ColorNormalVertex vert1(point->color, -direction, position1);
    ColorNormalVertex vert2(point->color, -direction, position1);
    for (const auto& normal : radials) {
      vert1.normal = normal;
      vert1.vertex = position1 + normal;
      vertices.push_back(vert1);

      vert2.normal = normal;
      vert2.vertex = position2 + normal;
      vertices.push_back(vert2);
    }

    // Now to stitch it together. we select the indices
    const unsigned int tubeStart = static_cast<unsigned int>(vertices.size());
    for (unsigned int j = 0; j < circleResolution; ++j) {
      unsigned int r1 = j + j;
      unsigned int r2 = (j != 0 ? r1 : circleResolution + circleResolution) - 2;
      indices.push_back(tubeStart + r1);
      indices.push_back(tubeStart + r1 + 1);
      indices.push_back(tubeStart + r2);

      indices.push_back(tubeStart + r2);
      indices.push_back(tubeStart + r1 + 1);
      indices.push_back(tubeStart + r2 + 1);
    }
  }

  line->vbo.upload(vertices, BufferObject::ArrayBuffer);
  line->ibo.upload(indices, BufferObject::ElementArrayBuffer);
  line->numberOfVertices = vertices.size();
  line->numberOfIndices = indices.size();

  line->dirty = false;
}

std::multimap<float, Identifier> BezierGeometry::hits(const Vector3f&,
                                                      const Vector3f&,
                                                      const Vector3f&) const
{
  return std::multimap<float, Identifier>();
}

} // namespace Rendering
} // namespace Avogadro