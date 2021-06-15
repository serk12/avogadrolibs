/******************************************************************************
  This source file is part of the Avogadro project.
  This source code is released under the 3-Clause BSD License, (see "LICENSE").
******************************************************************************/

#ifndef AVOGADRO_CORE_BIMAP_H
#define AVOGADRO_CORE_BIMAP_H

#include "avogadrocore.h"

#include <map>
#include <set>
#include <vector>

namespace Avogadro {
namespace Core {

class AVOGADROCORE_EXPORT BiMap
{
public:
  BiMap();
  BiMap(size_t n);
  ~BiMap();

  void addElement(size_t index);
  void addConection(size_t a, size_t b);

  void removeElement(size_t index);

  void removeConections();
  void removeConection(size_t index);
  void removeConection(size_t a, std::vector<size_t> a_neighbors, size_t b,
                       std::vector<size_t> b_neighbors);
  void clear();

  size_t getGroup(size_t element) const;
  std::vector<std::set<size_t>> getAllGroups() const;
  std::set<size_t> getElements(size_t group) const;

private:
  std::map<size_t, size_t> m_elementToGroup;
  std::vector<std::set<size_t>> m_groupToElement;

  void resetToSize(size_t n);
  void mergeGroups(size_t a, size_t b);
};

} // namespace Core
} // namespace Avogadro

#endif
