/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#include "qMRMLSortFilterSubjectHierarchyProxyModel.h"

// Subject Hierarchy MRML includes
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

// MRML Widgets includes
#include "qMRMLSubjectHierarchyModel.h"

// -----------------------------------------------------------------------------
// qMRMLSortFilterSubjectHierarchyProxyModelPrivate

// -----------------------------------------------------------------------------
/// \ingroup Slicer_MRMLWidgets
class qMRMLSortFilterSubjectHierarchyProxyModelPrivate
{
public:
  qMRMLSortFilterSubjectHierarchyProxyModelPrivate();
  QString NameFilter;
};

// -----------------------------------------------------------------------------
qMRMLSortFilterSubjectHierarchyProxyModelPrivate::qMRMLSortFilterSubjectHierarchyProxyModelPrivate()
{
}

// -----------------------------------------------------------------------------
// qMRMLSortFilterSubjectHierarchyProxyModel

//------------------------------------------------------------------------------
qMRMLSortFilterSubjectHierarchyProxyModel::qMRMLSortFilterSubjectHierarchyProxyModel(QObject *vparent)
 : QSortFilterProxyModel(vparent)
 , d_ptr(new qMRMLSortFilterSubjectHierarchyProxyModelPrivate)
{
}

//------------------------------------------------------------------------------
qMRMLSortFilterSubjectHierarchyProxyModel::~qMRMLSortFilterSubjectHierarchyProxyModel()
{
}

//-----------------------------------------------------------------------------
vtkMRMLScene* qMRMLSortFilterSubjectHierarchyProxyModel::mrmlScene()const
{
  qMRMLSubjectHierarchyModel* model = qobject_cast<qMRMLSubjectHierarchyModel*>(this->sourceModel());
  if (!model)
    {
    return NULL;
    }
  return model->mrmlScene();
}

//-----------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode* qMRMLSortFilterSubjectHierarchyProxyModel::subjectHierarchyNode()const
{
  qMRMLSubjectHierarchyModel* model = qobject_cast<qMRMLSubjectHierarchyModel*>(this->sourceModel());
  if (!model)
    {
    return NULL;
    }
  return model->subjectHierarchyNode();
}

//-----------------------------------------------------------------------------
void qMRMLSortFilterSubjectHierarchyProxyModel::setNameFilterString(QString nameFilter)
{
  Q_D(qMRMLSortFilterSubjectHierarchyProxyModel);
  d->NameFilter = nameFilter;
  this->invalidateFilter();
}

//-----------------------------------------------------------------------------
QStandardItem* qMRMLSortFilterSubjectHierarchyProxyModel::sourceItem(const QModelIndex& sourceIndex)const
{
  qMRMLSubjectHierarchyModel* model = qobject_cast<qMRMLSubjectHierarchyModel*>(this->sourceModel());
  if (!model)
    {
    return NULL;
    }
  return sourceIndex.isValid() ? model->itemFromIndex(sourceIndex) : model->invisibleRootItem();
}

//------------------------------------------------------------------------------
bool qMRMLSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent)const
{
  QStandardItem* parentItem = this->sourceItem(sourceParent);
  if (!parentItem)
    {
    return false;
    }
  QStandardItem* item = NULL;

  // Sometimes the row is not complete (DnD), search for a non null item
  for (int childIndex=0; childIndex < parentItem->columnCount(); ++childIndex)
    {
    item = parentItem->child(sourceRow, childIndex);
    if (item)
      {
      break;
      }
    }
  if (item == NULL)
    {
    return false;
    }
  qMRMLSubjectHierarchyModel* model = qobject_cast<qMRMLSubjectHierarchyModel*>(this->sourceModel());
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID = model->subjectHierarchyItemFromItem(item);
  return this->filterAcceptsItem(item);
}

//------------------------------------------------------------------------------
qMRMLSortFilterProxyModel::AcceptType qMRMLSortFilterSubjectHierarchyProxyModel::filterAcceptsItem(
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID )const
{
  Q_D(qMRMLSortFilterSubjectHierarchyProxyModel);

  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    return true;
    }
  vtkMRMLSubjectHierarchyNode* shNode = this->subjectHierarchyNode();
  if (!shNode)
    {
    return true;
    }

  // Filtering by data node properties
  vtkMRMLNode* dataNode = shNode->GetItemDataNode(itemID);
  if (dataNode)
    {
    // Filter by hide from editor property
    if (dataNode->GetHideFromEditors())
      {
      return false;
      }

    // Filter by exclude attribute
    if (dataNode->GetAttribute(itemID, vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyExcludeFromTreeAttributeName().c_str()))
      {
      return false;
      }
    }

  // Filter by name
  QString itemName(shNode->GetItemName(itemID).c_str());
  return itemName.contains(d->NameFilter, Qt::CaseInsensitive);
}
