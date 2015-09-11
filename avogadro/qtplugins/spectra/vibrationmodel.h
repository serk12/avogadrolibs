/******************************************************************************

  This source file is part of the Avogadro project.

  Copyright 2015 Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/

#ifndef AVOGADRO_QTPLUGINS_VIBRATIONMODEL_H
#define AVOGADRO_QTPLUGINS_VIBRATIONMODEL_H

#include <QtCore/QAbstractItemModel>

namespace Avogadro {
namespace QtGui {
class Molecule;
}
namespace QtPlugins {

class VibrationModel : public QAbstractItemModel
{
public:
  explicit VibrationModel(QObject *p = 0);

  QModelIndex parent(const QModelIndex &child) const;
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;

  Qt::ItemFlags flags(const QModelIndex &index) const;

  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

  bool setData(const QModelIndex &index, const QVariant &value, int role);
  QVariant data(const QModelIndex &index, int role) const;

  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const;

  void clear();

  void setMolecule(QtGui::Molecule *mol) { m_molecule = mol; }

signals:

public slots:

private:
  QtGui::Molecule * m_molecule;
};

}
}

#endif // AVOGADRO_QTPLUGINS_VIBRATIONMODEL_H