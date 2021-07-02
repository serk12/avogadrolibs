/******************************************************************************
  This source file is part of the Avogadro project.
  This source code is released under the 3-Clause BSD License, (see "LICENSE").
******************************************************************************/

// Base class for all undo commands used by this class.
// Used to expose molecule internals without needing to add explicit friendships
// between all undo commands and the container.
// Subclasses that want to support automatic merging should use the
// MergeUndoCommand interface below.
// Subclass implementations are inline in the code below. Specific undo
// operations are kept near the RWMolecule methods that use them.
class RWMolecule::UndoCommand : public QUndoCommand
{
public:
  UndoCommand(RWMolecule& m)
    : QUndoCommand(tr("Modify Molecule")), m_mol(m), m_molecule(m.m_molecule)
  {}

protected:
  Array<Index>& atomUniqueIds() { return m_molecule.atomUniqueIds(); }
  Array<Index>& bondUniqueIds() { return m_molecule.bondUniqueIds(); }
  Array<Vector3>& positions3d() { return m_molecule.atomPositions3d(); }
  Array<AtomHybridization>& hybridizations()
  {
    return m_molecule.hybridizations();
  }
  Array<signed char>& formalCharges() { return m_molecule.formalCharges(); }
  Array<Vector3ub>& colors() { return m_molecule.colors(); }

  Array<Vector3>& forceVectors() { return m_molecule.forceVectors(); }
  RWMolecule& m_mol;
  QtGui::Molecule& m_molecule;

  // ToDo delete this
  Array<unsigned char>& atomicNumbers() { return m_molecule.atomicNumbers(); }
  Array<std::pair<Index, Index>>& bondPairs() { return m_molecule.bondPairs(); }
  Array<unsigned char>& bondOrders() { return m_molecule.bondOrders(); }
};

namespace {
enum MergeIds
{
  SetPositions3dMergeId = 0,
  SetPosition3dMergeId,
  SetForceVectorMergeId,
  SetBondOrderMergeId
};

// Base class for undo commands that can be merged together, overriding the
// "after" state of an old command with that of the new command.
// Intended for use with RWMolecule's interactive mode.
// To add a new class, add a new entry to the MergeIds enum above and use that
// symbolic value as the template parameter. Implement the mergeWith() method
// to update the "after" state. See SetPositions3dCommand for an example.
template <int Id>
class MergeUndoCommand : public RWMolecule::UndoCommand
{
  bool m_canMerge;

public:
  MergeUndoCommand(RWMolecule& m) : UndoCommand(m), m_canMerge(false) {}
  void setCanMerge(bool merge) { m_canMerge = merge; }
  bool canMerge() const { return m_canMerge; }
  int id() const override { return m_canMerge ? Id : -1; }
};
} // namespace

namespace {
class AddAtomCommand : public RWMolecule::UndoCommand
{
  unsigned char m_atomicNumber;
  bool m_usingPositions;
  Index m_atomId;
  Index m_uniqueId;

public:
  AddAtomCommand(RWMolecule& m, unsigned char aN, bool usingPositions,
                 Index atomId, Index uid)
    : UndoCommand(m), m_atomicNumber(aN), m_usingPositions(usingPositions),
      m_atomId(atomId), m_uniqueId(uid)
  {}

  void redo() override
  {
    assert(m_molecule.atomCount() == m_atomId);
    m_molecule.addAtom(m_atomicNumber);
    if (m_usingPositions)
      positions3d().push_back(Vector3::Zero());
  }

  void undo() override
  {
    assert(m_molecule.atomCount() == m_atomId + 1);
    m_molecule.removeAtom(m_atomId);
  }
};
} // namespace

namespace {
class RemoveAtomCommand : public RWMolecule::UndoCommand
{
  Index m_atomId;
  Index m_atomUid;
  unsigned char m_atomicNumber;
  Vector3 m_position3d;

public:
  RemoveAtomCommand(RWMolecule& m, Index atomId, Index uid, unsigned char aN,
                    const Vector3& pos)
    : UndoCommand(m), m_atomId(atomId), m_atomUid(uid), m_atomicNumber(aN),
      m_position3d(pos)
  {}

  void redo() override
  {
    assert(m_atomUid < atomUniqueIds().size());
    atomUniqueIds()[m_atomUid] = MaxIndex;

    // Move the last atom to the removed atom's position:
    Index movedId = m_mol.atomCount() - 1;
    if (m_atomId != movedId) {
      atomicNumbers()[m_atomId] = atomicNumbers().back();
      if (positions3d().size() == atomicNumbers().size())
        positions3d()[m_atomId] = positions3d().back();

      // Update any bond pairs that have changed:
      Array<RWMolecule::BondType> atomBonds = m_mol.bonds(movedId);
      for (Array<RWMolecule::BondType>::const_iterator it = atomBonds.begin(),
                                                       itEnd = atomBonds.end();
           it != itEnd; ++it) {
        std::pair<Index, Index>& bondPair = bondPairs()[it->index()];
        if (bondPair.first == movedId)
          bondPair.first = m_atomId;
        else
          bondPair.second = m_atomId;
      }

      // Update the moved atom's uid
      Index movedUid = m_mol.atomUniqueId(movedId);
      assert(movedUid != MaxIndex);
      atomUniqueIds()[movedUid] = m_atomId;
    }

    // Resize the arrays:
    if (positions3d().size() == atomicNumbers().size())
      positions3d().resize(movedId, Vector3::Zero());
    atomicNumbers().resize(movedId, 0);
    m_molecule.setGraphDirty(true);
  }

  void undo() override
  {
    // Append removed atom's info to the end of the arrays:
    if (positions3d().size() == atomicNumbers().size())
      positions3d().push_back(m_position3d);
    atomicNumbers().push_back(m_atomicNumber);

    // Swap the moved and unremoved atom data if needed
    Index movedId = m_mol.atomCount() - 1;
    if (m_atomId != movedId) {
      using std::swap;
      if (positions3d().size() == atomicNumbers().size())
        swap(positions3d()[m_atomId], positions3d().back());
      swap(atomicNumbers()[m_atomId], atomicNumbers().back());

      // Update any bond pairs that have changed:
      Array<RWMolecule::BondType> atomBonds(m_mol.bonds(m_atomId));
      for (Array<RWMolecule::BondType>::iterator it = atomBonds.begin(),
                                                 itEnd = atomBonds.end();
           it != itEnd; ++it) {
        std::pair<Index, Index>& bondPair = bondPairs()[it->index()];
        if (bondPair.first == m_atomId)
          bondPair.first = movedId;
        else
          bondPair.second = movedId;
      }

      // Update the moved atom's UID
      Index movedUid = m_mol.atomUniqueId(m_atomId);
      assert(movedUid != MaxIndex);
      atomUniqueIds()[movedUid] = movedId;
    }

    // Update the removed atom's UID
    atomUniqueIds()[m_atomUid] = m_atomId;
    m_molecule.setGraphDirty(true);
  }
};
} // namespace

namespace {
class SetAtomicNumbersCommand : public RWMolecule::UndoCommand
{
  Core::Array<unsigned char> m_oldAtomicNumbers;
  Core::Array<unsigned char> m_newAtomicNumbers;

public:
  SetAtomicNumbersCommand(RWMolecule& m,
                          const Core::Array<unsigned char>& oldAtomicNumbers,
                          const Core::Array<unsigned char>& newAtomicNumbers)
    : UndoCommand(m), m_oldAtomicNumbers(oldAtomicNumbers),
      m_newAtomicNumbers(newAtomicNumbers)
  {}

  void redo() override { atomicNumbers() = m_newAtomicNumbers; }

  void undo() override { atomicNumbers() = m_oldAtomicNumbers; }
};
} // namespace

namespace {
class SetAtomicNumberCommand : public RWMolecule::UndoCommand
{
  Index m_atomId;
  unsigned char m_oldAtomicNumber;
  unsigned char m_newAtomicNumber;

public:
  SetAtomicNumberCommand(RWMolecule& m, Index atomId,
                         unsigned char oldAtomicNumber,
                         unsigned char newAtomicNumber)
    : UndoCommand(m), m_atomId(atomId), m_oldAtomicNumber(oldAtomicNumber),
      m_newAtomicNumber(newAtomicNumber)
  {}

  void redo() override { atomicNumbers()[m_atomId] = m_newAtomicNumber; }

  void undo() override { atomicNumbers()[m_atomId] = m_oldAtomicNumber; }
};
} // namespace

namespace {
class SetPositions3dCommand : public MergeUndoCommand<SetPositions3dMergeId>
{
  Core::Array<Vector3> m_oldPositions3d;
  Core::Array<Vector3> m_newPositions3d;

public:
  SetPositions3dCommand(RWMolecule& m,
                        const Core::Array<Vector3>& oldPositions3d,
                        const Core::Array<Vector3>& newPositions3d)
    : MergeUndoCommand<SetPositions3dMergeId>(m),
      m_oldPositions3d(oldPositions3d), m_newPositions3d(newPositions3d)
  {}

  void redo() override { positions3d() = m_newPositions3d; }

  void undo() override { positions3d() = m_oldPositions3d; }

  bool mergeWith(const QUndoCommand* other) override
  {
    const SetPositions3dCommand* o =
      dynamic_cast<const SetPositions3dCommand*>(other);
    if (o) {
      m_newPositions3d = o->m_newPositions3d;
      return true;
    }
    return false;
  }
};
} // namespace

namespace {
class SetPosition3dCommand : public MergeUndoCommand<SetPosition3dMergeId>
{
  Array<Index> m_atomIds;
  Array<Vector3> m_oldPosition3ds;
  Array<Vector3> m_newPosition3ds;

public:
  SetPosition3dCommand(RWMolecule& m, Index atomId,
                       const Vector3& oldPosition3d,
                       const Vector3& newPosition3d)
    : MergeUndoCommand<SetPosition3dMergeId>(m), m_atomIds(1, atomId),
      m_oldPosition3ds(1, oldPosition3d), m_newPosition3ds(1, newPosition3d)
  {}

  void redo() override
  {
    for (size_t i = 0; i < m_atomIds.size(); ++i)
      positions3d()[m_atomIds[i]] = m_newPosition3ds[i];
  }

  void undo() override
  {
    for (size_t i = 0; i < m_atomIds.size(); ++i)
      positions3d()[m_atomIds[i]] = m_oldPosition3ds[i];
  }

  bool mergeWith(const QUndoCommand* o) override
  {
    const SetPosition3dCommand* other =
      dynamic_cast<const SetPosition3dCommand*>(o);
    if (!other)
      return false;

    size_t numAtoms = other->m_atomIds.size();
    if (numAtoms != other->m_oldPosition3ds.size() ||
        numAtoms != other->m_newPosition3ds.size()) {
      return false;
    }

    for (size_t i = 0; i < numAtoms; ++i) {
      const Index& atomId = other->m_atomIds[i];
      const Vector3& oldPos = other->m_oldPosition3ds[i];
      const Vector3& newPos = other->m_newPosition3ds[i];

      Array<Index>::const_iterator idsBegin = m_atomIds.begin();
      Array<Index>::const_iterator idsEnd = m_atomIds.end();
      Array<Index>::const_iterator match = std::find(idsBegin, idsEnd, atomId);

      if (match == idsEnd) {
        // Append a new atom:
        m_atomIds.push_back(atomId);
        m_oldPosition3ds.push_back(oldPos);
        m_newPosition3ds.push_back(newPos);
      } else {
        // Overwrite the existing movement:
        size_t offset = std::distance(idsBegin, match);
        assert(m_atomIds[offset] == atomId);
        m_newPosition3ds[offset] = newPos;
      }
    }

    return true;
  }
};
} // namespace

namespace {
class SetAtomHybridizationCommand : public RWMolecule::UndoCommand
{
  Index m_atomId;
  Core::AtomHybridization m_oldHybridization;
  Core::AtomHybridization m_newHybridization;

public:
  SetAtomHybridizationCommand(RWMolecule& m, Index atomId,
                              Core::AtomHybridization oldHybridization,
                              Core::AtomHybridization newHybridization)
    : UndoCommand(m), m_atomId(atomId), m_oldHybridization(oldHybridization),
      m_newHybridization(newHybridization)
  {}

  void redo() override { hybridizations()[m_atomId] = m_newHybridization; }

  void undo() override { hybridizations()[m_atomId] = m_oldHybridization; }
};
} // namespace

namespace {
class SetAtomFormalChargeCommand : public RWMolecule::UndoCommand
{
  Index m_atomId;
  signed char m_oldCharge;
  signed char m_newCharge;

public:
  SetAtomFormalChargeCommand(RWMolecule& m, Index atomId, signed char oldCharge,
                             signed char newCharge)
    : UndoCommand(m), m_atomId(atomId), m_oldCharge(oldCharge),
      m_newCharge(newCharge)
  {}

  void redo() override { formalCharges()[m_atomId] = m_newCharge; }

  void undo() override { formalCharges()[m_atomId] = m_oldCharge; }
};
} // namespace

namespace {
class SetAtomColorCommand : public RWMolecule::UndoCommand
{
  Index m_atomId;
  Vector3ub m_oldColor;
  Vector3ub m_newColor;

public:
  SetAtomColorCommand(RWMolecule& m, Index atomId, Vector3ub oldColor,
                      Vector3ub newColor)
    : UndoCommand(m), m_atomId(atomId), m_oldColor(oldColor),
      m_newColor(newColor)
  {}

  void redo() override { colors()[m_atomId] = m_newColor; }

  void undo() override { colors()[m_atomId] = m_oldColor; }
};
} // end namespace

namespace {
class AddBondCommand : public RWMolecule::UndoCommand
{
  unsigned char m_bondOrder;
  std::pair<Index, Index> m_bondPair;
  Index m_bondId;
  Index m_uniqueId;

public:
  AddBondCommand(RWMolecule& m, unsigned char order,
                 const std::pair<Index, Index>& bondPair, Index bondId,
                 Index uid)
    : UndoCommand(m), m_bondOrder(order), m_bondPair(bondPair),
      m_bondId(bondId), m_uniqueId(uid)
  {}

  void redo() override
  {
    assert(m_molecule.bondCount() == m_bondId);
    m_molecule.addBond(m_bondPair.first, m_bondPair.second, m_bondOrder);
  }

  void undo() override
  {
    assert(m_molecule.bondCount() == m_bondId + 1);
    m_molecule.removeBond(m_bondId);
  }
};
} // namespace

// Make an std::pair where the lower index is always first in the pair. This
// offers us the guarantee that any given pair of atoms will always result in
// a pair that is the same no matter what the order of the atoms given.
inline std::pair<Index, Index> makeBondPair(Index a, Index b)
{
  return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
}

namespace {
class RemoveBondCommand : public RWMolecule::UndoCommand
{
  Index m_bondId;
  Index m_bondUid;
  std::pair<Index, Index> m_bondPair;
  unsigned char m_bondOrder;

public:
  RemoveBondCommand(RWMolecule& m, Index bondId, Index bondUid,
                    const std::pair<Index, Index>& bondPair,
                    unsigned char bondOrder)
    : UndoCommand(m), m_bondId(bondId), m_bondUid(bondUid),
      m_bondPair(bondPair), m_bondOrder(bondOrder)
  {}

  void redo() override { m_molecule.removeBond(m_bondId); }

  void undo() override
  {
    m_molecule.addBond(m_bondPair.first, m_bondPair.second, m_bondOrder);
  }
};
} // namespace

namespace {
class SetBondOrdersCommand : public RWMolecule::UndoCommand
{
  Array<unsigned char> m_oldBondOrders;
  Array<unsigned char> m_newBondOrders;

public:
  SetBondOrdersCommand(RWMolecule& m, const Array<unsigned char>& oldBondOrders,
                       const Array<unsigned char>& newBondOrders)
    : UndoCommand(m), m_oldBondOrders(oldBondOrders),
      m_newBondOrders(newBondOrders)
  {}

  void redo() override { m_molecule.setBondOrders(m_newBondOrders); }

  void undo() override { m_molecule.setBondOrders(m_oldBondOrders); }
};
} // namespace

namespace {
class SetBondOrderCommand : public MergeUndoCommand<SetBondOrderMergeId>
{
  Index m_bondId;
  unsigned char m_oldBondOrder;
  unsigned char m_newBondOrder;

public:
  SetBondOrderCommand(RWMolecule& m, Index bondId, unsigned char oldBondOrder,
                      unsigned char newBondOrder)
    : MergeUndoCommand<SetBondOrderMergeId>(m), m_bondId(bondId),
      m_oldBondOrder(oldBondOrder), m_newBondOrder(newBondOrder)
  {}

  void redo() override { m_molecule.setBondOrder(m_bondId, m_newBondOrder); }

  void undo() override { m_molecule.setBondOrder(m_bondId, m_oldBondOrder); }

  bool mergeWith(const QUndoCommand* other) override
  {
    const SetBondOrderCommand* o =
      dynamic_cast<const SetBondOrderCommand*>(other);
    // Only merge when the bondIds match.
    if (o && o->m_bondId == this->m_bondId) {
      this->m_newBondOrder = o->m_newBondOrder;
      return true;
    }
    return false;
  }
};
} // namespace

namespace {
class SetBondPairsCommand : public RWMolecule::UndoCommand
{
  Array<std::pair<Index, Index>> m_oldBondPairs;
  Array<std::pair<Index, Index>> m_newBondPairs;

public:
  SetBondPairsCommand(RWMolecule& m,
                      const Array<std::pair<Index, Index>>& oldBondPairs,
                      const Array<std::pair<Index, Index>>& newBondPairs)
    : UndoCommand(m), m_oldBondPairs(oldBondPairs), m_newBondPairs(newBondPairs)
  {}

  void redo() override { m_molecule.setBondPairs(m_newBondPairs); }

  void undo() override { m_molecule.setBondPairs(m_oldBondPairs); }
};
} // namespace

namespace {
class SetBondPairCommand : public RWMolecule::UndoCommand
{
  Index m_bondId;
  std::pair<Index, Index> m_oldBondPair;
  std::pair<Index, Index> m_newBondPair;

public:
  SetBondPairCommand(RWMolecule& m, Index bondId,
                     const std::pair<Index, Index>& oldBondPair,
                     const std::pair<Index, Index>& newBondPair)
    : UndoCommand(m), m_bondId(bondId), m_oldBondPair(oldBondPair),
      m_newBondPair(newBondPair)
  {}

  void redo() override { m_molecule.setBondPair(m_bondId, m_newBondPair); }

  void undo() override { m_molecule.setBondPair(m_bondId, m_oldBondPair); }
};
} // namespace

namespace {
class AddUnitCellCommand : public RWMolecule::UndoCommand
{
  UnitCell m_newUnitCell;

public:
  AddUnitCellCommand(RWMolecule& m, const UnitCell& newUnitCell)
    : UndoCommand(m), m_newUnitCell(newUnitCell)
  {}

  void redo() override
  {
    m_mol.molecule().setUnitCell(new UnitCell(m_newUnitCell));
  }

  void undo() override { m_mol.molecule().setUnitCell(nullptr); }
};
} // namespace

namespace {
class RemoveUnitCellCommand : public RWMolecule::UndoCommand
{
  UnitCell m_oldUnitCell;

public:
  RemoveUnitCellCommand(RWMolecule& m, const UnitCell& oldUnitCell)
    : UndoCommand(m), m_oldUnitCell(oldUnitCell)
  {}

  void redo() override { m_mol.molecule().setUnitCell(nullptr); }

  void undo() override
  {
    m_mol.molecule().setUnitCell(new UnitCell(m_oldUnitCell));
  }
};
} // namespace

namespace {
class ModifyMoleculeCommand : public RWMolecule::UndoCommand
{
  Molecule m_oldMolecule;
  Molecule m_newMolecule;

public:
  ModifyMoleculeCommand(RWMolecule& m, const Molecule& oldMolecule,
                        const Molecule& newMolecule)
    : UndoCommand(m), m_oldMolecule(oldMolecule), m_newMolecule(newMolecule)
  {}

  void redo() override { m_mol.molecule() = m_newMolecule; }

  void undo() override { m_mol.molecule() = m_oldMolecule; }
};
} // namespace

namespace {
class SetForceVectorCommand : public MergeUndoCommand<SetForceVectorMergeId>
{
  Array<Index> m_atomIds;
  Array<Vector3> m_oldForceVectors;
  Array<Vector3> m_newForceVectors;

public:
  SetForceVectorCommand(RWMolecule& m, Index atomId,
                        const Vector3& oldForceVector,
                        const Vector3& newForceVector)
    : MergeUndoCommand<SetForceVectorMergeId>(m), m_atomIds(1, atomId),
      m_oldForceVectors(1, oldForceVector), m_newForceVectors(1, newForceVector)
  {}

  void redo() override
  {
    for (size_t i = 0; i < m_atomIds.size(); ++i)
      forceVectors()[m_atomIds[i]] = m_newForceVectors[i];
  }

  void undo() override
  {
    for (size_t i = 0; i < m_atomIds.size(); ++i)
      forceVectors()[m_atomIds[i]] = m_oldForceVectors[i];
  }

  bool mergeWith(const QUndoCommand* o) override
  {
    const SetForceVectorCommand* other =
      dynamic_cast<const SetForceVectorCommand*>(o);
    if (!other)
      return false;

    size_t numAtoms = other->m_atomIds.size();
    if (numAtoms != other->m_oldForceVectors.size() ||
        numAtoms != other->m_newForceVectors.size()) {
      return false;
    }

    for (size_t i = 0; i < numAtoms; ++i) {
      const Index& atomId = other->m_atomIds[i];
      const Vector3& oldPos = other->m_oldForceVectors[i];
      const Vector3& newPos = other->m_newForceVectors[i];

      Array<Index>::const_iterator idsBegin = m_atomIds.begin();
      Array<Index>::const_iterator idsEnd = m_atomIds.end();
      Array<Index>::const_iterator match = std::find(idsBegin, idsEnd, atomId);

      if (match == idsEnd) {
        // Append a new atom:
        m_atomIds.push_back(atomId);
        m_oldForceVectors.push_back(oldPos);
        m_newForceVectors.push_back(newPos);
      } else {
        // Overwrite the existing movement:
        size_t offset = std::distance(idsBegin, match);
        assert(m_atomIds[offset] == atomId);
        m_newForceVectors[offset] = newPos;
      }
    }

    return true;
  }
};
} // namespace
