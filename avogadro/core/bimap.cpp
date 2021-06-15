/******************************************************************************
  This source file is part of the Avogadro project.
  This source code is released under the 3-Clause BSD License, (see "LICENSE").
******************************************************************************/

#include "bimap.h"

#include <algorithm>
#include <cassert>

namespace Avogadro {
namespace Core {

BiMap::BiMap() {}

BiMap::BiMap(size_t n) : m_groupToElement(n)
{
  resetToSize(n);
}

BiMap::~BiMap() {}

void BiMap::addElement(size_t index)
{
  m_elementToGroup[index] = m_groupToElement.size();
  std::set<size_t> group;
  group.insert(index);
  m_groupToElement.push_back(group);
}

void BiMap::addConection(size_t a, size_t b)
{
  assert(m_elementToGroup.find(a) != m_elementToGroup.end());
  assert(m_elementToGroup.find(b) != m_elementToGroup.end());

  size_t group_a = m_elementToGroup[a];
  size_t group_b = m_elementToGroup[b];
  if (group_a != group_b) {
    for (size_t element : m_groupToElement[group_b]) {
      m_elementToGroup[element] = group_a;
      m_groupToElement[group_a].insert(element);
    }
    m_groupToElement.erase(std::next(m_groupToElement.begin(), group_b));
  }
}

void BiMap::removeElement(size_t index)
{
  assert(m_elementToGroup.find(index) != m_elementToGroup.end());
  removeConection(index);
  size_t group = m_elementToGroup[index];
  m_elementToGroup.erase(group);
}

void BiMap::removeConections()
{
  size_t n = m_elementToGroup.size();
  clear();
  resetToSize(n);
}

void BiMap::removeConection(size_t index)
{
  assert(index < m_elementToGroup.size());
  size_t group = m_elementToGroup[index];
  if (m_groupToElement[group].size() > 1) {
    m_groupToElement[group].erase(index);
    addElement(index);
  }
}

void BiMap::removeConection(size_t a, std::vector<size_t> a_neighbors, size_t b,
                            std::vector<size_t> b_neighbors)
{
  assert(m_elementToGroup.find(a) != m_elementToGroup.end());
  assert(m_elementToGroup.find(b) != m_elementToGroup.end());

  std::sort(a_neighbors.begin(), a_neighbors.end());
  std::sort(b_neighbors.begin(), b_neighbors.end());
  std::vector<int> v_intersection;
  std::set_intersection(a_neighbors.begin(), a_neighbors.end(),
                        b_neighbors.begin(), b_neighbors.end(),
                        std::back_inserter(v_intersection));

  if (v_intersection.size() == 0) {
    removeConection(a);
    size_t group = m_elementToGroup[a];
    for (const auto& n : a_neighbors) {
      size_t oldGroup = m_elementToGroup[n];
      m_groupToElement[oldGroup].erase(n);
      m_groupToElement[group].insert(n);
      m_elementToGroup[n] = group;
    }
  }
}

void BiMap::clear()
{
  m_elementToGroup.clear();
  m_groupToElement.clear();
}

size_t BiMap::getGroup(size_t element) const
{
  assert(m_elementToGroup.find(element) != m_elementToGroup.end());
  return m_elementToGroup.at(element);
}

std::set<size_t> BiMap::getElements(size_t group) const
{
  assert(group < m_groupToElement.size());
  return m_groupToElement[group];
}

std::vector<std::set<size_t>> BiMap::getAllGroups() const { return m_groupToElement; }

void BiMap::resetToSize(size_t n)
{
  for (size_t i = 0; i < n; ++i) {
    m_elementToGroup[i] = i;
    m_groupToElement[i].insert(i);
  }
}

} // namespace Core
} // namespace Avogadro
