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

// Qt includes
#include <QDebug>
#include <QMimeData>
#include <QApplication>
#include <QMessageBox>
#include <QTimer>

// qMRML includes
#include "qMRMLSubjectHierarchyModel_p.h"

// MRML includes
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLScene.h>

// Subject Hierarchy includes
#include <vtkSlicerSubjectHierarchyModuleLogic.h>
#include <qSlicerSubjectHierarchyPluginHandler.h>
#include <qSlicerSubjectHierarchyAbstractPlugin.h>
#include <qSlicerSubjectHierarchyDefaultPlugin.h>

//------------------------------------------------------------------------------
qMRMLSubjectHierarchyModelPrivate::qMRMLSubjectHierarchyModelPrivate(qMRMLSubjectHierarchyModel& object)
  : q_ptr(&object)
  , NameColumn(-1)
  , IDColumn(-1)
  , VisibilityColumn(-1)
  , TransformColumn(-1)
  , SubjectHierarchyNode(NULL)
  , MRMLScene(NULL)
{
  this->CallBack = vtkSmartPointer<vtkCallbackCommand>::New();
  this->PendingItemModified = -1; // -1 means not updating

  this->HiddenIcon = QIcon(":Icons/VisibleOff.png");
  this->VisibleIcon = QIcon(":Icons/VisibleOn.png");
  this->PartiallyVisibleIcon = QIcon(":Icons/VisiblePartially.png");

  this->UnknownIcon = QIcon(":Icons/Unknown.png");
  this->WarningIcon = QIcon(":Icons/Warning.png");

  this->DelayedItemChangedInvoked = false;

  qRegisterMetaType<QStandardItem*>("QStandardItem*"); //TODO: Needed?
}

//------------------------------------------------------------------------------
qMRMLSubjectHierarchyModelPrivate::~qMRMLSubjectHierarchyModelPrivate()
{
  if (this->SubjectHierarchyNode)
    {
    this->SubjectHierarchyNode->RemoveObserver(this->CallBack);
    }
  if (this->MRMLScene)
    {
    this->MRMLScene->RemoveObserver(this->CallBack);
    }
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModelPrivate::init()
{
  Q_Q(qMRMLSubjectHierarchyModel);
  this->CallBack->SetClientData(q);
  this->CallBack->SetCallback(qMRMLSubjectHierarchyModel::onEvent);

  QObject::connect(q, SIGNAL(itemChanged(QStandardItem*)), q, SLOT(onItemChanged(QStandardItem*)));

  q->setNameColumn(0);
  q->setVisibilityColumn(1);
  q->setTransformColumn(2);
  q->setIDColumn(3);

  q->setHorizontalHeaderLabels(
    QStringList() << "Node" << "" << "" << "IDs");

  q->horizontalHeaderItem(q->nameColumn())->setToolTip(QObject::tr("Node name and type"));
  q->horizontalHeaderItem(q->visibilityColumn())->setToolTip(QObject::tr("Show/hide branch or node"));
  q->horizontalHeaderItem(q->transformColumn())->setToolTip(QObject::tr("Applied transform"));
  q->horizontalHeaderItem(q->idColumn())->setToolTip(QObject::tr("Node ID"));

  q->horizontalHeaderItem(q->visibilityColumn())->setIcon(QIcon(":/Icons/Small/SlicerVisibleInvisible.png"));
  q->horizontalHeaderItem(q->transformColumn())->setIcon(QIcon(":/Icons/Transform.png"));

  // Set visibility icons from model to the default plugin
  qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setDefaultVisibilityIcons(
    this->VisibleIcon, this->HiddenIcon, this->PartiallyVisibleIcon );
}

//------------------------------------------------------------------------------
QModelIndexList qMRMLSubjectHierarchyModelPrivate::indexes(const vtkIdType itemID)const
{
  Q_Q(const qMRMLSubjectHierarchyModel);
  QModelIndex scene = q->subjectHierarchySceneIndex();
  if (scene == QModelIndex())
    {
    return QModelIndexList();
    }
  // QAbstractItemModel::match doesn't browse through columns, we need to do it manually
  QModelIndexList shItemIndexes = q->match(
    scene, qMRMLSubjectHierarchyModel::SubjectHierarchyItemIDRole, QVariant(qlonglong(itemID)), 1, Qt::MatchExactly | Qt::MatchRecursive);
  if (shItemIndexes.size() != 1)
    {
    return QModelIndexList(); // If 0 it's empty, if >1 it's invalid (one item for each UID)
    }
  // Add the QModelIndexes from the other columns
  const int row = shItemIndexes[0].row();
  QModelIndex shItemParentIndex = shItemIndexes[0].parent();
  const int sceneColumnCount = q->columnCount(shItemParentIndex);
  for (int col=1; col<sceneColumnCount; ++col)
    {
    shItemIndexes << q->index(row, col, shItemParentIndex);
    }
  return shItemIndexes;
}

//------------------------------------------------------------------------------
QString qMRMLSubjectHierarchyModelPrivate::subjectHierarchyItemName(vtkIdType itemID)
{
  if (!this->SubjectHierarchyNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy";
    return "Error";
    }
  return QString(this->SubjectHierarchyNode->GetItemName(itemID).c_str());
}

//------------------------------------------------------------------------------
QStandardItem* qMRMLSubjectHierarchyModelPrivate::insertSubjectHierarchyItem(vtkIdType itemID, int index)
{
  Q_Q(qMRMLSubjectHierarchyModel);
  QStandardItem* item = q->itemFromSubjectHierarchyItem(itemID);
  if (item)
    {
    // It is possible that the item has been already added if it is the parent of a child item already inserted
    return item;
    }
  vtkIdType parentItemID = q->parentSubjectHierarchyItem(itemID);
  QStandardItem* parentItem = q->itemFromSubjectHierarchyItem(parentItemID);
  if (!parentItem)
    {
    if (parentItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
      {
      qCritical() << Q_FUNC_INFO << ": Unable to get parent for subject hierarchy item with ID " << itemID;
      return NULL;
      }
    parentItem = q->insertSubjectHierarchyItem(parentItemID);
    if (!parentItem)
      {
      qCritical() << Q_FUNC_INFO << ": Failed to insert parent subject hierarchy item with ID " << parentItemID;
      return NULL;
      }
    }
  item = q->insertSubjectHierarchyItem(itemID, parentItem, index);
  if (q->itemFromSubjectHierarchyItem(itemID) != item)
    {
    qCritical() << Q_FUNC_INFO << ": Item mismatch when inserting subject hierarchy item with ID " << itemID;
    return NULL;
    }
  return item;
}

//------------------------------------------------------------------------------
// qMRMLSubjectHierarchyModel
//------------------------------------------------------------------------------
qMRMLSubjectHierarchyModel::qMRMLSubjectHierarchyModel(QObject *_parent)
  :QStandardItemModel(_parent)
  , d_ptr(new qMRMLSubjectHierarchyModelPrivate(*this))
{
  Q_D(qMRMLSubjectHierarchyModel);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLSubjectHierarchyModel::qMRMLSubjectHierarchyModel(qMRMLSubjectHierarchyModelPrivate* pimpl, QObject* parent)
  : QStandardItemModel(parent)
  , d_ptr(pimpl)
{
  Q_D(qMRMLSubjectHierarchyModel);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLSubjectHierarchyModel::~qMRMLSubjectHierarchyModel()
{
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qMRMLSubjectHierarchyModel);
  if (scene == d->MRMLScene)
    {
    return;
    }

  if (d->MRMLScene)
    {
    d->MRMLScene->RemoveObserver(d->CallBack);
    }

  d->MRMLScene = scene;
  this->setSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(scene));

  if (scene)
    {
    scene->AddObserver(vtkMRMLScene::EndCloseEvent, d->CallBack);
    scene->AddObserver(vtkMRMLScene::EndImportEvent, d->CallBack);
    scene->AddObserver(vtkMRMLScene::StartBatchProcessEvent, d->CallBack);
    scene->AddObserver(vtkMRMLScene::EndBatchProcessEvent, d->CallBack);
    }
  else
    {
    qWarning() << Q_FUNC_INFO << ": Invalid MRML scene set";
    }
}

//------------------------------------------------------------------------------
vtkMRMLScene* qMRMLSubjectHierarchyModel::mrmlScene()const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  return d->MRMLScene;
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::setSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* shNode)
{
  Q_D(qMRMLSubjectHierarchyModel);
  if (shNode == d->SubjectHierarchyNode)
    {
    return;
    }

  if (d->SubjectHierarchyNode)
    {
    d->SubjectHierarchyNode->RemoveObserver(d->CallBack);
    }

  d->SubjectHierarchyNode = shNode;

  // Remove all items
  const int oldColumnCount = this->columnCount();
  this->removeRows(0, this->rowCount());
  this->setColumnCount(oldColumnCount);

  // Update whole subject hierarchy
  this->updateFromSubjectHierarchy();

  if (shNode)
    {
    // Using priority value of -10 in certain observations results in those callbacks being called after
    // those with neutral priorities. Useful to have the plugin handler deal with new items before allowing
    // them to be handled by the model.
    // Same idea for +10, in which case the callback is called first.
    shNode->AddObserver(vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemAddedEvent, d->CallBack, -10.0);
    shNode->AddObserver(vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemAboutToBeRemovedEvent, d->CallBack, +10.0);
    shNode->AddObserver(vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemRemovedEvent, d->CallBack, -10.0);
    shNode->AddObserver(vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemModifiedEvent, d->CallBack, -10.0);
    shNode->AddObserver(vtkCommand::ModifiedEvent, d->CallBack, -10.0);
    shNode->AddObserver(vtkCommand::DeleteEvent, d->CallBack, -10.0);
    }
}

//------------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode* qMRMLSubjectHierarchyModel::subjectHierarchyNode()const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  return d->SubjectHierarchyNode;
}

//------------------------------------------------------------------------------
QStandardItem* qMRMLSubjectHierarchyModel::subjectHierarchySceneItem()const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  if (!d->SubjectHierarchyNode || this->maxColumnId() == -1)
    {
    return NULL;
    }
  int count = this->invisibleRootItem()->rowCount();
  for (int row=0; row<count; ++row)
    {
    QStandardItem* child = this->invisibleRootItem()->child(row);
    if (!child)
      {
      continue;
      }
    QVariant uid = child->data(qMRMLSubjectHierarchyModel::SubjectHierarchyItemIDRole);
    if (uid.type() == QVariant::LongLong && uid == d->SubjectHierarchyNode->GetSceneItemID())
      {
      return child;
      }
    }
  return NULL;
}

//------------------------------------------------------------------------------
QModelIndex qMRMLSubjectHierarchyModel::subjectHierarchySceneIndex()const
{
  QStandardItem* shSceneItem = this->subjectHierarchySceneItem();
  if (shSceneItem == NULL)
    {
    return QModelIndex();
    }
  return shSceneItem ? shSceneItem->index() : QModelIndex();
}

// -----------------------------------------------------------------------------
vtkIdType qMRMLSubjectHierarchyModel::subjectHierarchyItemFromIndex(const QModelIndex &index)const
{
  return this->subjectHierarchyItemFromItem(this->itemFromIndex(index));
}

//------------------------------------------------------------------------------
vtkIdType qMRMLSubjectHierarchyModel::subjectHierarchyItemFromItem(QStandardItem* item)const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  if (!d->SubjectHierarchyNode || !item)
    {
    return vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
    }
  QVariant shItemID = item->data(qMRMLSubjectHierarchyModel::SubjectHierarchyItemIDRole);
  if (!shItemID.isValid())
    {
    return vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
    }
  return item->data(qMRMLSubjectHierarchyModel::SubjectHierarchyItemIDRole).toLongLong();
}
//------------------------------------------------------------------------------
QStandardItem* qMRMLSubjectHierarchyModel::itemFromSubjectHierarchyItem(vtkIdType itemID, int column/*=0*/)const
{
  QModelIndex index = this->indexFromSubjectHierarchyItem(itemID, column);
  QStandardItem* item = this->itemFromIndex(index);
  return item;
}

//------------------------------------------------------------------------------
QModelIndex qMRMLSubjectHierarchyModel::indexFromSubjectHierarchyItem(vtkIdType itemID, int column/*=0*/)const
{
  Q_D(const qMRMLSubjectHierarchyModel);

  QModelIndex itemIndex;
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    return itemIndex;
    }

  // Try to find the nodeIndex in the cache first
  QMap<vtkIdType,QPersistentModelIndex>::iterator rowCacheIt = d->RowCache.find(itemID);
  if (rowCacheIt==d->RowCache.end())
    {
    // Not found in cache, therefore it cannot be in the model
    return itemIndex;
    }
  if (rowCacheIt.value().isValid())
    {
    // An entry found in the cache. If the item at the cached index matches the requested item ID then we use it.
    QStandardItem* item = this->itemFromIndex(rowCacheIt.value());
    if (item && item->data(qMRMLSubjectHierarchyModel::SubjectHierarchyItemIDRole).toLongLong() == itemID)
      {
      // ID matched
      itemIndex = rowCacheIt.value();
      }
    }

  // The cache was not up-to-date. Do a slow linear search.
  if (!itemIndex.isValid())
    {
    // QAbstractItemModel::match doesn't browse through columns, we need to do it manually
    QModelIndexList itemIndexes = this->match(
      this->subjectHierarchySceneIndex(), SubjectHierarchyItemIDRole, itemID, 1, Qt::MatchExactly | Qt::MatchRecursive);
    if (itemIndexes.size() == 0)
      {
      d->RowCache.remove(itemID);
      return QModelIndex();
      }
    itemIndex = itemIndexes[0];
    d->RowCache[itemID] = itemIndex;
    }
  if (column == 0)
    {
    // QAbstractItemModel::match only search through the first column
    return itemIndex;
    }
  // Add the QModelIndexes from the other columns
  const int row = itemIndex.row();
  QModelIndex nodeParentIndex = itemIndex.parent();
  if (column >= this->columnCount(nodeParentIndex))
    {
    qCritical() << Q_FUNC_INFO << ": Invalid column " << column;
    return QModelIndex();
    }
  return nodeParentIndex.child(row, column);
}

//------------------------------------------------------------------------------
QModelIndexList qMRMLSubjectHierarchyModel::indexes(vtkIdType itemID)const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  //TODO: Can we simply move the private method here?
  return d->indexes(itemID);
}

//------------------------------------------------------------------------------
vtkIdType qMRMLSubjectHierarchyModel::parentSubjectHierarchyItem(vtkIdType itemID)const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  if (!d->SubjectHierarchyNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy";
    return vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
    }
  return d->SubjectHierarchyNode->GetItemParent(itemID);
}

//------------------------------------------------------------------------------
int qMRMLSubjectHierarchyModel::subjectHierarchyItemIndex(vtkIdType itemID)const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  if (!d->SubjectHierarchyNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy";
    return -1;
    }
  return d->SubjectHierarchyNode->GetItemPositionUnderParent(itemID);
}

//------------------------------------------------------------------------------
bool qMRMLSubjectHierarchyModel::canBeAChild(vtkIdType itemID)const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  if (!d->SubjectHierarchyNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy";
    return false;
    }
  // Only the root and invalid item cannot be child
  return (itemID != d->SubjectHierarchyNode->GetSceneItemID() && itemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID);
}

//------------------------------------------------------------------------------
bool qMRMLSubjectHierarchyModel::canBeAParent(vtkIdType itemID)const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  if (!d->SubjectHierarchyNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy";
    return false;
    }
  // Only invalid item cannot be parent
  return (itemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID);
}

//------------------------------------------------------------------------------
bool qMRMLSubjectHierarchyModel::isAncestorItem(vtkIdType child, vtkIdType ancestor)const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  if (!d->SubjectHierarchyNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy";
    return false;
    }

  for (; child != d->SubjectHierarchyNode->GetSceneItemID(); child = d->SubjectHierarchyNode->GetItemParent(child))
    {
    if (child == ancestor)
      {
      return true;
      }
    }
  return false;
}

//------------------------------------------------------------------------------
bool qMRMLSubjectHierarchyModel::isAffiliatedItem(vtkIdType itemA, vtkIdType itemB)const
  {
  return this->isAncestorItem(itemA, itemB) || this->isAncestorItem(itemB, itemA);
  }

//------------------------------------------------------------------------------
bool qMRMLSubjectHierarchyModel::reparent(vtkIdType itemID, vtkIdType newParentID)
{
  if ( itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID || newParentID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID
    || newParentID == itemID )
    {
    qCritical() << Q_FUNC_INFO << ": Invalid input";
    return false;
    }

  vtkIdType oldParentID = this->parentSubjectHierarchyItem(itemID);
  if (oldParentID == newParentID)
    {
    return false;
    }

  Q_D(const qMRMLSubjectHierarchyModel);
  if (!d->SubjectHierarchyNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy";
    return false;
    }

  if (!this->canBeAParent(newParentID))
    {
    qCritical() << Q_FUNC_INFO << ": Target parent (" << d->SubjectHierarchyNode->GetItemName(newParentID).c_str() << ") is not a valid parent!";
    return false;
    }

  // If dropped from within the subject hierarchy tree
  QList<qSlicerSubjectHierarchyAbstractPlugin*> foundPlugins =
    qSlicerSubjectHierarchyPluginHandler::instance()->pluginsForReparentingItemInSubjectHierarchy(itemID, newParentID);
  qSlicerSubjectHierarchyAbstractPlugin* selectedPlugin = NULL;
  if (foundPlugins.size() > 1)
    {
    // Let the user choose a plugin if more than one returned the same non-zero confidence value
    vtkMRMLNode* dataNode = d->SubjectHierarchyNode->GetItemDataNode(itemID);
    QString textToDisplay = QString(
      "Equal confidence number found for more than one subject hierarchy plugin for reparenting.\n\n"
      "Select plugin to reparent item\n'%1'\n(type %2)\nParent item: %3").arg(
      d->SubjectHierarchyNode->GetItemName(itemID).c_str()).arg(
      dataNode?dataNode->GetNodeTagName():d->SubjectHierarchyNode->GetItemLevel(itemID).c_str()).arg(
      d->SubjectHierarchyNode->GetItemName(newParentID).c_str() );
    selectedPlugin = qSlicerSubjectHierarchyPluginHandler::instance()->selectPluginFromDialog(textToDisplay, foundPlugins);
    }
  else if (foundPlugins.size() == 1)
    {
    selectedPlugin = foundPlugins[0];
    }
  else
    {
    // Choose default plugin if all registered plugins returned confidence value 0
    selectedPlugin = qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin();
    }

  // If default plugin was chosen to reparent virtual item (an item in a virtual branch), or into a virtual branch,
  // then abort reparenting (it means that the actual owner plugin cannot reparent its own virtual item, so it then
  // cannot be reparented).
  if ( ( ( !d->SubjectHierarchyNode->GetItemAttribute(newParentID,
             vtkMRMLSubjectHierarchyConstants::GetVirtualBranchSubjectHierarchyNodeAttributeName().c_str()).empty() )
      || ( !d->SubjectHierarchyNode->GetItemAttribute(itemID,
             vtkMRMLSubjectHierarchyConstants::GetVirtualBranchSubjectHierarchyNodeAttributeName().c_str()).empty() ) )
    && selectedPlugin == qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin() )
  {
    qCritical() << Q_FUNC_INFO << ": Failed to reparent virtual item "
      << d->SubjectHierarchyNode->GetItemName(itemID).c_str() << " under parent " << d->SubjectHierarchyNode->GetItemName(newParentID).c_str();
    return false;
  }

  // Have the selected plugin reparent the node
  bool successfullyReparentedByPlugin = selectedPlugin->reparentItemInsideSubjectHierarchy(itemID, newParentID);
  if (!successfullyReparentedByPlugin)
    {
    //subjectHierarchyNode->SetParentNodeID( subjectHierarchyNode->GetParentNodeID() ); //TODO: Anything like this needed?

    qCritical() << Q_FUNC_INFO << ": Failed to reparent item "
      << d->SubjectHierarchyNode->GetItemName(itemID).c_str() << " through plugin '"
      << selectedPlugin->name().toLatin1().constData() << "'";
    return false;
    }

  return true;
}

//------------------------------------------------------------------------------
bool qMRMLSubjectHierarchyModel::moveToRow(vtkIdType itemID, int newRow)
{
  Q_D(const qMRMLSubjectHierarchyModel);
  if (!d->SubjectHierarchyNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy";
    return false;
    }

  if ( itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID
    || itemID == d->SubjectHierarchyNode->GetSceneItemID() )
    {
    qCritical() << Q_FUNC_INFO << ": Invalid input item ID";
    return false;
    }

  vtkIdType parentItemID = this->parentSubjectHierarchyItem(itemID);
  if (parentItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": No parent found for item " << itemID;
    return false;
    }

  // Get item currently next to desired position
  vtkIdType beforeItemID = d->SubjectHierarchyNode->GetItemByPositionUnderParent(parentItemID, newRow+1);

  // Move item to position
  return d->SubjectHierarchyNode->MoveItem(itemID, beforeItemID);
}

//------------------------------------------------------------------------------
QMimeData* qMRMLSubjectHierarchyModel::mimeData(const QModelIndexList& indexes)const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  if (!indexes.size())
    {
    return 0;
    }
  QModelIndex parent = indexes[0].parent();
  QModelIndexList allColumnsIndexes;
  foreach(const QModelIndex& index, indexes)
    {
    QModelIndex parent = index.parent();
    for (int column = 0; column < this->columnCount(parent); ++column)
      {
      allColumnsIndexes << this->index(index.row(), column, parent);
      }
    d->DraggedSubjectHierarchyItems << this->subjectHierarchyItemFromIndex(index);
    }
  // Remove duplicates
  allColumnsIndexes = allColumnsIndexes.toSet().toList();
  return this->QStandardItemModel::mimeData(allColumnsIndexes);
}

//------------------------------------------------------------------------------
bool qMRMLSubjectHierarchyModel::dropMimeData( const QMimeData *data, Qt::DropAction action,
                                            int row, int column, const QModelIndex &parent )
{
  Q_D(qMRMLSubjectHierarchyModel);
  Q_UNUSED(column);
  // We want to do drag&drop only into the first item of a line (and not on a
  // random column.
  bool res = this->Superclass::dropMimeData(
    data, action, row, 0, parent.sibling(parent.row(), 0));
  d->DraggedSubjectHierarchyItems.clear();
  return res;
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::updateFromSubjectHierarchy()
{
  Q_D(qMRMLSubjectHierarchyModel);

  d->RowCache.clear();

  // Enabled so it can be interacted with
  this->invisibleRootItem()->setFlags(Qt::ItemIsEnabled);

  if (!d->SubjectHierarchyNode)
    {
    // Remove all items
    const int oldColumnCount = this->columnCount();
    this->removeRows(0, this->rowCount());
    this->setColumnCount(oldColumnCount);
    return;
    }
  else if (!this->subjectHierarchySceneItem())
    {
    // No subject hierarchy root item has been created yet, but the subject hierarchy
    // node is valid, so we need to create a scene item
    vtkIdType sceneItemID = d->SubjectHierarchyNode->GetSceneItemID();
    QList<QStandardItem*> sceneItems;
    QStandardItem* sceneItem = new QStandardItem();
    sceneItem->setFlags(Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
    sceneItem->setText("Scene");
    sceneItem->setData(sceneItemID, qMRMLSubjectHierarchyModel::SubjectHierarchyItemIDRole);
    sceneItems << sceneItem;
    for (int i = 1; i < this->columnCount(); ++i)
      {
      QStandardItem* sceneOtherColumn = new QStandardItem();
      sceneOtherColumn->setFlags(0);
      sceneItems << sceneOtherColumn;
      }
    sceneItem->setColumnCount(this->columnCount());

    d->RowCache[sceneItemID] = QModelIndex(); // Insert invalid item in cache to indicate that item is in the model but its index is unknown
    this->insertRow(0, sceneItems);
    d->RowCache[sceneItemID] = sceneItem->index();
    }
  else
    {
    // Update the scene item index in case subject hierarchy node has changed
    this->subjectHierarchySceneItem()->setData(
      QVariant::fromValue(d->SubjectHierarchyNode->GetSceneItemID()), qMRMLSubjectHierarchyModel::SubjectHierarchyItemIDRole );
    d->RowCache[d->SubjectHierarchyNode->GetSceneItemID()] = this->subjectHierarchySceneItem()->index();
    }

  if (!this->subjectHierarchySceneItem())
    {
    qCritical() << Q_FUNC_INFO << ": Failed to create subject hierarchy scene item";
    return;
    }

  // Remove rows before populating
  this->subjectHierarchySceneItem()->removeRows(0, this->subjectHierarchySceneItem()->rowCount());

  // Populate subject hierarchy with the items
  std::vector<vtkIdType> allItemIDs;
  d->SubjectHierarchyNode->GetItemChildren(d->SubjectHierarchyNode->GetSceneItemID(), allItemIDs, true);
  for (std::vector<vtkIdType>::iterator itemIt=allItemIDs.begin(); itemIt!=allItemIDs.end(); ++itemIt)
    {
    vtkIdType itemID = (*itemIt);
    int index = d->SubjectHierarchyNode->GetItemPositionUnderParent(itemID);
    d->insertSubjectHierarchyItem(itemID, index);
    }

  // Update expanded states (during inserting the update calls did not find valid indices, so
  // expand and collapse statuses were not set in the tree view)
  for (std::vector<vtkIdType>::iterator itemIt=allItemIDs.begin(); itemIt!=allItemIDs.end(); ++itemIt)
    {
    vtkIdType itemID = (*itemIt);
    // Expanded states are handled with the name column
    QStandardItem* item = this->itemFromSubjectHierarchyItem(itemID, this->nameColumn());
    this->updateItemDataFromSubjectHierarchyItem(item, itemID, this->nameColumn());
    }
}

//------------------------------------------------------------------------------
QStandardItem* qMRMLSubjectHierarchyModel::insertSubjectHierarchyItem(vtkIdType itemID)
{
  Q_D(qMRMLSubjectHierarchyModel);
  return d->insertSubjectHierarchyItem(itemID, this->subjectHierarchyItemIndex(itemID));
}

//------------------------------------------------------------------------------
QStandardItem* qMRMLSubjectHierarchyModel::insertSubjectHierarchyItem(vtkIdType itemID, QStandardItem* parent, int row/*=-1*/ )
{
  Q_D(qMRMLSubjectHierarchyModel);

  if (!parent)
    {
    // The scene is inserted individually, and the other items must always have a valid parent (if not other then the scene)
    qCritical() << Q_FUNC_INFO << ": Invalid parent to inserted subject hierarchy item with ID " << itemID;
    return NULL;
    }

  QList<QStandardItem*> items;
  for (int col=0; col<this->columnCount(); ++col)
    {
    QStandardItem* newItem = new QStandardItem();
    this->updateItemFromSubjectHierarchyItem(newItem, itemID, col);
    items.append(newItem);
    }

  // Insert an invalid item in the cache to indicate that the subject hierarchy item is in the
  // model but we don't know its index yet. This is needed because a custom widget may be notified
  // abot row insertion before insertRow() returns (and the RowCache entry is added).
  d->RowCache[itemID] = QModelIndex();
  parent->insertRow(row, items);
  d->RowCache[itemID] = items[0]->index();

  return items[0];
}

//------------------------------------------------------------------------------
QFlags<Qt::ItemFlag> qMRMLSubjectHierarchyModel::subjectHierarchyItemFlags(vtkIdType itemID, int column)const
{
  Q_D(const qMRMLSubjectHierarchyModel);

  QFlags<Qt::ItemFlag> flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  // Name and transform columns are editable
  if (column == this->nameColumn() || column == this->transformColumn())
    {
    flags |= Qt::ItemIsEditable;
    }

  if (this->canBeAChild(itemID))
    {
    flags |= Qt::ItemIsDragEnabled;
    }
  if (this->canBeAParent(itemID))
    {
    flags |= Qt::ItemIsDropEnabled;
    }

  // Drop is also enabled for virtual branches.
  // (a virtual branch is a branch where the children items do not correspond to actual MRML data nodes,
  // but to implicit items contained by the parent MRML node, e.g. in case of Markups or Segmentations)
  if ( d->SubjectHierarchyNode->HasItemAttribute( itemID,
    vtkMRMLSubjectHierarchyConstants::GetVirtualBranchSubjectHierarchyNodeAttributeName()) )
    {
    flags |= Qt::ItemIsDropEnabled;
    }
  // Along the same logic, drop is not enabled to children nodes in virtual branches
  vtkIdType parentItemID = d->SubjectHierarchyNode->GetItemParent(itemID);
  if ( parentItemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID
    && d->SubjectHierarchyNode->HasItemAttribute(
         parentItemID, vtkMRMLSubjectHierarchyConstants::GetVirtualBranchSubjectHierarchyNodeAttributeName()) )
    {
    flags &= ~Qt::ItemIsDropEnabled;
    }

  return flags;
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::updateItemFromSubjectHierarchyItem(QStandardItem* item, vtkIdType shItemID, int column)
{
  Q_D(qMRMLSubjectHierarchyModel);
  // We are going to make potentially multiple changes to the item. We want to refresh
  // the subject hierarchy item only once, so we "block" the updates in onItemChanged().
  d->PendingItemModified = 0;
  item->setFlags(this->subjectHierarchyItemFlags(shItemID, column));

  // Set ID
  bool blocked = this->blockSignals(true);
  item->setData(shItemID, qMRMLSubjectHierarchyModel::SubjectHierarchyItemIDRole);
  this->blockSignals(blocked);

  // Update item data for the current column
  this->updateItemDataFromSubjectHierarchyItem(item, shItemID, column);

  bool itemChanged = (d->PendingItemModified > 0);
  d->PendingItemModified = -1;

  if (this->canBeAChild(shItemID))
    {
    QStandardItem* parentItem = item->parent();
    QStandardItem* newParentItem = this->itemFromSubjectHierarchyItem(this->parentSubjectHierarchyItem(shItemID));
    if (!newParentItem)
      {
      newParentItem = this->subjectHierarchySceneItem();
      }
    // If the item has no parent, then it means it hasn't been put into the hierarchy yet and it will do it automatically
    if (parentItem && parentItem != newParentItem)
      {
      int newIndex = this->subjectHierarchyItemIndex(shItemID);
      if (parentItem != newParentItem || newIndex != item->row())
        {
        // Reparent items
        QList<QStandardItem*> children = parentItem->takeRow(item->row());
        newParentItem->insertRow(newIndex, children);
        }
      }
    }
  if (itemChanged)
    {
    this->onItemChanged(item);
    }
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::updateItemDataFromSubjectHierarchyItem(QStandardItem* item, vtkIdType shItemID, int column)
{
  Q_D(qMRMLSubjectHierarchyModel);
  if (!d->SubjectHierarchyNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy";
    return;
    }

  qSlicerSubjectHierarchyAbstractPlugin* ownerPlugin = NULL;
  if (!d->SubjectHierarchyNode->GetItemOwnerPluginName(shItemID).empty())
    {
    ownerPlugin = qSlicerSubjectHierarchyPluginHandler::instance()->getOwnerPluginForSubjectHierarchyItem(shItemID);
    if (!ownerPlugin)
      {
      if (column == this->nameColumn())
        {
        item->setText(d->subjectHierarchyItemName(shItemID));
        item->setToolTip(tr("No subject hierarchy role assigned! Please report error"));
        if (item->icon().cacheKey() != d->WarningIcon.cacheKey()) // Only set if it changed (https://bugreports.qt-project.org/browse/QTBUG-20248)
          {
          item->setIcon(d->WarningIcon);
          }
        }
        return;
      }
    }
  else
    {
    qDebug() << Q_FUNC_INFO << ": No owner plugin for subject hierarchy item '" << d->subjectHierarchyItemName(shItemID);

    // Owner plugin name is not set for subject hierarchy item. Show it as a regular node
    if (column == this->nameColumn())
      {
      item->setText(QString(d->SubjectHierarchyNode->GetItemName(shItemID).c_str()));
      if (item->icon().cacheKey() != d->UnknownIcon.cacheKey()) // Only set if it changed (https://bugreports.qt-project.org/browse/QTBUG-20248)
        {
        item->setIcon(d->UnknownIcon);
        }
      }
    if (column == this->idColumn())
      {
      vtkMRMLNode* dataNode = d->SubjectHierarchyNode->GetItemDataNode(shItemID);
      if (dataNode)
        {
        item->setText(QString(dataNode->GetID()));
        }
      }
    return;
    }

  // Owner plugin exists, show information normally

  // Name column
  if (column == this->nameColumn())
    {
    item->setText(ownerPlugin->displayedItemName(shItemID));
    item->setToolTip(ownerPlugin->tooltip(shItemID));

    // Have owner plugin set the icon
    QIcon icon = ownerPlugin->icon(shItemID);
    if (!icon.isNull())
      {
      if (item->icon().cacheKey() != icon.cacheKey()) // Only set if it changed (https://bugreports.qt-project.org/browse/QTBUG-20248)
        {
        item->setIcon(icon);
        }
      }
    else if (item->icon().cacheKey() != d->UnknownIcon.cacheKey()) // Only set if it changed (https://bugreports.qt-project.org/browse/QTBUG-20248)
      {
      item->setIcon(d->UnknownIcon);
      }

    // Set expanded state (in the name column so that it is only processed once for each item)
    if (d->SubjectHierarchyNode->GetItemExpanded(shItemID))
      {
      emit requestExpandItem(shItemID);
      }
    else
      {
      emit requestCollapseItem(shItemID);
      }
    }
  // ID column
  if (column == this->idColumn())
    {
    vtkMRMLNode* dataNode = d->SubjectHierarchyNode->GetItemDataNode(shItemID);
    if (dataNode)
      {
      item->setText(QString(dataNode->GetID()));
      }
    }
  // Visibility column
  if (column == this->visibilityColumn())
    {
    // Have owner plugin give the visibility state and icon
    int visible = ownerPlugin->getDisplayVisibility(shItemID);
    QIcon visibilityIcon = ownerPlugin->visibilityIcon(visible);

    // It should be fine to set the icon even if it is the same, but due
    // to a bug in Qt (http://bugreports.qt.nokia.com/browse/QTBUG-20248),
    // it would fire a superflous itemChanged() signal.
    if ( item->data(VisibilityRole).isNull()
      || item->data(VisibilityRole).toInt() != visible )
      {
      item->setData(visible, VisibilityRole);
      if (!visibilityIcon.isNull())
        {
        item->setIcon(visibilityIcon);
        }
      }
    }
  // Transform column
  if (column == this->transformColumn())
    {
    if (item->data(Qt::WhatsThisRole).toString().isEmpty())
      {
      item->setData( "Transform", Qt::WhatsThisRole );
      }

    vtkMRMLNode* dataNode = d->SubjectHierarchyNode->GetItemDataNode(shItemID);
    vtkMRMLTransformableNode* transformableNode = vtkMRMLTransformableNode::SafeDownCast(dataNode);
    if (transformableNode)
      {
      vtkMRMLTransformNode* parentTransformNode = ( transformableNode->GetParentTransformNode() ? transformableNode->GetParentTransformNode() : NULL );
      QString transformNodeId( parentTransformNode ? parentTransformNode->GetID() : "" );
      QString transformNodeName( parentTransformNode ? parentTransformNode->GetName() : "" );
      // Only change item if the transform itself changed
      if (item->text().compare(transformNodeName))
        {
        item->setData(transformNodeId, TransformIDRole);
        item->setText(transformNodeName);
        item->setToolTip( parentTransformNode ? tr("%1 (%2)").arg(parentTransformNode->GetName()).arg(parentTransformNode->GetID()) : "" );
        }
      }
    else
      {
      item->setToolTip(tr("No transform can be directly applied on non-transformable nodes,\n"
        "however a transform can be chosen to apply it on all the children") );
      }
    }
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::updateSubjectHierarchyItemFromItem(vtkIdType shItemID, QStandardItem* item)
{
  //int wasModifying = node->StartModify(); //TODO: Add feature to item if there are performance issues
  this->updateSubjectHierarchyItemFromItemData(shItemID, item);
  //node->EndModify(wasModifying);

  // the following only applies to tree hierarchies
  if (!this->canBeAChild(shItemID))
    {
    return;
    }

 Q_ASSERT(shItemID != this->subjectHierarchyItemFromItem(item->parent()));

  QStandardItem* parentItem = item->parent();
  int columnCount = parentItem ? parentItem->columnCount() : 0;
  // Don't do the following if the row is not complete (reparenting an incomplete row might lead to errors;
  // if there is no child yet for a given column, it will get there next time updateNodeFromItem is called).
  // updateNodeFromItem is called for every item drag&dropped (we ensure that all the indexes of the row are
  // reparented when entering the d&d function)
  for (int col=0; col<columnCount; ++col)
    {
    if (parentItem->child(item->row(), col) == 0)
      {
      return;
      }
    }

  vtkIdType parentItemID = this->subjectHierarchyItemFromItem(parentItem);
  if (this->parentSubjectHierarchyItem(shItemID) != parentItemID)
    {
    // Parent changed, need to reparent the subject hierarchy item in the node
    emit aboutToReparentByDragAndDrop(shItemID, parentItemID);
    if (this->reparent(shItemID, parentItemID))
      {
      emit reparentedByDragAndDrop(shItemID, parentItemID);
      emit requestExpandItem(parentItemID);
      }
    else
      {
      this->updateItemFromSubjectHierarchyItem(item, shItemID, item->column());
      }
    }
  else if (this->subjectHierarchyItemIndex(shItemID) != item->row())
    {
    // Moved within parent, need to re-order subject hierarchy item in the node
    if (!this->moveToRow(shItemID, item->row()))
      {
      this->updateItemFromSubjectHierarchyItem(item, shItemID, item->column());
      }
    }
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::updateSubjectHierarchyItemFromItemData(vtkIdType shItemID, QStandardItem* item)
{
  Q_D(qMRMLSubjectHierarchyModel);
  if (!d->SubjectHierarchyNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy";
    return;
    }

  qSlicerSubjectHierarchyAbstractPlugin* ownerPlugin =
    qSlicerSubjectHierarchyPluginHandler::instance()->getOwnerPluginForSubjectHierarchyItem(shItemID);

  // Name column
  if ( item->column() == this->nameColumn() )
    {
    // This call renames associated data node if any
    d->SubjectHierarchyNode->SetItemName(shItemID, item->text().toLatin1().constData());
    }
  // Visibility column
  if ( item->column() == this->visibilityColumn()
    && !item->data(VisibilityRole).isNull() )
    {
    int visible = item->data(VisibilityRole).toInt();
    if (visible > -1 && visible != ownerPlugin->getDisplayVisibility(shItemID))
      {
      // Have owner plugin set the display visibility
      ownerPlugin->setDisplayVisibility(shItemID, visible);
      }
    }
  // Transform column
  if (item->column() == this->transformColumn())
    {
    QVariant transformIdData = item->data(TransformIDRole);
    std::string newParentTransformNodeIdStr = transformIdData.toString().toLatin1().constData();
    vtkMRMLTransformNode* newParentTransformNode =
      vtkMRMLTransformNode::SafeDownCast( d->MRMLScene->GetNodeByID(newParentTransformNodeIdStr) );

    // No action if the chosen transform is the same as the applied one
    vtkMRMLTransformableNode* dataNode = vtkMRMLTransformableNode::SafeDownCast(
      d->SubjectHierarchyNode->GetItemDataNode(shItemID) );
    if (dataNode && dataNode->GetParentTransformNode() == newParentTransformNode)
      {
      return;
      }

    // No checks and questions when the transform is being removed
    if (!newParentTransformNode)
      {
      vtkSlicerSubjectHierarchyModuleLogic::TransformBranch(d->SubjectHierarchyNode, shItemID, NULL, false);
      return;
      }

    // Ask the user if any child node in the tree is transformed with a transform different from the chosen one
    bool hardenExistingTransforms = true;
    if (d->SubjectHierarchyNode->IsAnyNodeInBranchTransformed(shItemID))
      {
      QMessageBox::StandardButton answer =
        QMessageBox::question(NULL, tr("Some nodes in the branch are already transformed"),
        tr("Do you want to harden all already applied transforms before setting the new one?\n\n"
        "  Note: If you choose no, then the applied transform will simply be replaced."),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::Yes);
      if (answer == QMessageBox::No)
        {
        hardenExistingTransforms = false;
        }
      else if (answer == QMessageBox::Cancel)
        {
        //qDebug() << Q_FUNC_INFO << ": Transform branch cancelled";
        return;
        }
      }

    vtkSlicerSubjectHierarchyModuleLogic::TransformBranch(
      d->SubjectHierarchyNode, shItemID, newParentTransformNode, hardenExistingTransforms );
    }
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::updateModelItems(vtkIdType itemID)
{
  Q_D(qMRMLSubjectHierarchyModel);
  if (d->MRMLScene->IsClosing() || d->MRMLScene->IsBatchProcessing())
    {
    return;
    }

  QModelIndexList itemIndexes = d->indexes(itemID);
  if (!itemIndexes.count())
    {
    // Can happen while the item is added, the plugin handler sets the owner plugin, which triggers
    // item modified before it can be inserted to the model
    return;
    }

  for (int currentIndex=0; currentIndex<itemIndexes.size(); ++currentIndex)
    {
    // Note: If this loop is changed to foreach update after reparenting stops working.
    //   Apparently foreach makes a deep copy of itemIndexes, and as the indices change after the
    //   first reparenting in the updateItemFromSubjectHierarchyItem for column 0, the old indices
    //   are being used for the subsequent columns, which yield new items, and reparenting is
    //   performed on those too. With regular for and [] operator, the updated indices are got,
    //   so reparenting is only performed once, which is the desired behavior.
    QModelIndex index = itemIndexes[currentIndex];
    QStandardItem* item = this->itemFromIndex(index);
    int oldRow = item->row();
    QStandardItem* oldParent = item->parent();

    this->updateItemFromSubjectHierarchyItem(item, itemID, item->column());

    // If the item was reparented, then we need to rescan the indexes again as they may be wrong
    if (item->row() != oldRow || item->parent() != oldParent)
      {
      int oldSize = itemIndexes.size();
      itemIndexes = this->indexes(itemID);
      int newSize = itemIndexes.size();
      if (oldSize != newSize)
        {
        qCritical() << Q_FUNC_INFO << ": Index mismatch";
        return;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::onEvent(
  vtkObject* caller, unsigned long event, void* clientData, void* callData )
{
  vtkMRMLSubjectHierarchyNode* shNode = reinterpret_cast<vtkMRMLSubjectHierarchyNode*>(caller);
  vtkMRMLScene* scene = reinterpret_cast<vtkMRMLScene*>(caller);
  qMRMLSubjectHierarchyModel* sceneModel = reinterpret_cast<qMRMLSubjectHierarchyModel*>(clientData);
  if (!sceneModel || (!shNode && !scene))
    {
    qCritical() << Q_FUNC_INFO << ": Invalid event parameters";
    return;
    }

  // Get item ID
  vtkIdType itemID = vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
  if (callData)
    {
    vtkIdType* itemIdPtr = reinterpret_cast<vtkIdType*>(callData);
    if (itemIdPtr)
      {
      itemID = *itemIdPtr;
      }
    }

  switch (event)
    {
    case vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemAddedEvent:
      sceneModel->onSubjectHierarchyItemAdded(itemID);
      break;
    case vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemAboutToBeRemovedEvent:
      sceneModel->onSubjectHierarchyItemAboutToBeRemoved(itemID);
      break;
    case vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemRemovedEvent:
      sceneModel->onSubjectHierarchyItemRemoved(itemID);
      break;
    case vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemModifiedEvent:
      sceneModel->onSubjectHierarchyItemModified(itemID);
      break;
    case vtkMRMLScene::EndImportEvent:
      sceneModel->onMRMLSceneImported(scene);
      break;
    case vtkMRMLScene::EndCloseEvent:
      sceneModel->onMRMLSceneClosed(scene);
      break;
    case vtkMRMLScene::StartBatchProcessEvent:
      sceneModel->onMRMLSceneStartBatchProcess(scene);
      break;
    case vtkMRMLScene::EndBatchProcessEvent:
      sceneModel->onMRMLSceneEndBatchProcess(scene);
      break;
    case vtkCommand::ModifiedEvent:
      if (shNode)
        {
        sceneModel->onSubjectHierarchyNodeModified();
        }
      break;
    case vtkCommand::DeleteEvent:
      sceneModel->onSubjectHierarchyNodeRemoved();
      break;
    }
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::onSubjectHierarchyItemAdded(vtkIdType itemID)
{
  this->insertSubjectHierarchyItem(itemID);
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::onSubjectHierarchyItemAboutToBeRemoved(vtkIdType itemID)
{
  Q_D(qMRMLSubjectHierarchyModel);
  if (d->MRMLScene->IsClosing() || d->MRMLScene->IsBatchProcessing())
    {
    return;
    }

  QModelIndexList itemIndexes = this->match(
    this->subjectHierarchySceneIndex(), SubjectHierarchyItemIDRole, itemID, 1, Qt::MatchExactly | Qt::MatchRecursive );
  if (itemIndexes.count() > 0)
    {
    QStandardItem* item = this->itemFromIndex(itemIndexes[0].sibling(itemIndexes[0].row(),0));
    // The children may be lost if not reparented, we ensure they got reparented.
    while (item->rowCount())
      {
      // Need to remove the children from the removed item because they would be automatically deleted in QStandardItemModel::removeRow()
      d->Orphans.push_back(item->takeRow(0));
      }
    // Remove the item from any orphan list if it exist as we don't want to add it back later in onSubjectHierarchyItemRemoved
    foreach (QList<QStandardItem*> orphans, d->Orphans)
      {
      if (orphans.contains(item))
        {
        d->Orphans.removeAll(orphans);
        }
      }
    this->removeRow(itemIndexes[0].row(), itemIndexes[0].parent());
    }
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::onSubjectHierarchyItemRemoved(vtkIdType removedItemID)
{
  Q_D(qMRMLSubjectHierarchyModel);
  Q_UNUSED(removedItemID);
  if (d->MRMLScene->IsClosing() || d->MRMLScene->IsBatchProcessing())
    {
    return;
    }
  // The removed item may had children, if they haven't been updated, they are likely to be lost
  // (not reachable when browsing the model), we need to reparent them.
  foreach(QList<QStandardItem*> orphans, d->Orphans)
    {
    QStandardItem* orphan = orphans[0];
    // Make sure that the orphans have not already been reparented.
    if (orphan->parent())
      {
      continue;
      }
    vtkIdType itemID = this->subjectHierarchyItemFromItem(orphan);
    int newIndex = this->subjectHierarchyItemIndex(itemID);
    QStandardItem* newParentItem = this->itemFromSubjectHierarchyItem(
      this->parentSubjectHierarchyItem(itemID) );
    if (!newParentItem)
      {
      newParentItem = this->subjectHierarchySceneItem();
      }
    // Reparent orphans
    newParentItem->insertRow(newIndex, orphans);
    }
  d->Orphans.clear();
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::onSubjectHierarchyItemModified(vtkIdType itemID)
{
  this->updateModelItems(itemID);
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::onMRMLSceneImported(vtkMRMLScene* scene)
{
  Q_UNUSED(scene);
  this->updateFromSubjectHierarchy();
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::onMRMLSceneClosed(vtkMRMLScene* scene)
{
  // Make sure there is one subject hierarchy node in the scene, and it is used by the model
  vtkMRMLSubjectHierarchyNode* newSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(scene);
  if (!newSubjectHierarchyNode)
    {
    qCritical() << Q_FUNC_INFO << ": No subject hierarchy node could be retrieved from the scene";
    }

  this->setSubjectHierarchyNode(newSubjectHierarchyNode);
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::onMRMLSceneStartBatchProcess(vtkMRMLScene* scene)
{
  Q_UNUSED(scene);
  emit subjectHierarchyAboutToBeUpdated();
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::onMRMLSceneEndBatchProcess(vtkMRMLScene* scene)
{
  Q_UNUSED(scene);
  this->updateFromSubjectHierarchy();
  emit subjectHierarchyUpdated();
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::onSubjectHierarchyNodeModified()
{
  this->updateFromSubjectHierarchy();
  emit subjectHierarchyUpdated();
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::onSubjectHierarchyNodeRemoved()
{
  Q_D(qMRMLSubjectHierarchyModel);
  if (d->MRMLScene->IsClosing())
    {
    return;
    }

  // Make sure there is one subject hierarchy node in the scene, and it is used by the model
  vtkMRMLSubjectHierarchyNode* newSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(d->MRMLScene);
  if (!newSubjectHierarchyNode)
    {
    qCritical() << Q_FUNC_INFO << ": No subject hierarchy node could be retrieved from the scene";
    }

  this->setSubjectHierarchyNode(newSubjectHierarchyNode);
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::onItemChanged(QStandardItem* item)
{
  Q_D(qMRMLSubjectHierarchyModel);
  if (d->PendingItemModified >= 0)
    {
    ++d->PendingItemModified;
    return;
    }
  // When a drag&drop occurs, the order of the items called with onItemChanged is
  // random, it could be the item in column 1 then the item in column 0
  if (d->DraggedSubjectHierarchyItems.count())
    {
    if (item->column() == 0)
      {
      d->DraggedItems.insert(item);
      }
    if (d->DraggedItems.count() > 0 && !d->DelayedItemChangedInvoked)
      {
      // Item changed will be triggered multiple times in course of the drag&drop event. Setting this flag
      // makes sure the final onItemChanged with the collected DraggedItems is called only once.
      d->DelayedItemChangedInvoked = true;
      QTimer::singleShot(200, this, SLOT(delayedItemChanged()));
      }
    return;
    }

  this->updateSubjectHierarchyItemFromItem(this->subjectHierarchyItemFromItem(item), item);
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::delayedItemChanged()
{
  Q_D(qMRMLSubjectHierarchyModel);
  foreach(QStandardItem* item, d->DraggedItems)
    {
    this->onItemChanged(item);
    }
  d->DraggedItems.clear();
  d->DelayedItemChangedInvoked = false;
}

//------------------------------------------------------------------------------
Qt::DropActions qMRMLSubjectHierarchyModel::supportedDropActions()const
{
  return Qt::MoveAction;
}

//------------------------------------------------------------------------------
int qMRMLSubjectHierarchyModel::nameColumn()const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  return d->NameColumn;
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::setNameColumn(int column)
{
  Q_D(qMRMLSubjectHierarchyModel);
  d->NameColumn = column;
  this->updateColumnCount();
}

//------------------------------------------------------------------------------
int qMRMLSubjectHierarchyModel::idColumn()const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  return d->IDColumn;
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::setIDColumn(int column)
{
  Q_D(qMRMLSubjectHierarchyModel);
  d->IDColumn = column;
  this->updateColumnCount();
}

//------------------------------------------------------------------------------
int qMRMLSubjectHierarchyModel::visibilityColumn()const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  return d->VisibilityColumn;
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::setVisibilityColumn(int column)
{
  Q_D(qMRMLSubjectHierarchyModel);
  d->VisibilityColumn = column;
  this->updateColumnCount();
}

//------------------------------------------------------------------------------
int qMRMLSubjectHierarchyModel::transformColumn()const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  return d->TransformColumn;
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::setTransformColumn(int column)
{
  Q_D(qMRMLSubjectHierarchyModel);
  d->TransformColumn = column;
  this->updateColumnCount();
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::updateColumnCount()
{
  Q_D(const qMRMLSubjectHierarchyModel);

  int max = this->maxColumnId();
  int oldColumnCount = this->columnCount();
  this->setColumnCount(max + 1);
  if (oldColumnCount == 0)
    {
    this->updateFromSubjectHierarchy();
    }
  else
    {
    // Update all items
    if (!d->SubjectHierarchyNode)
      {
      return;
      }
    std::vector<vtkIdType> allItemIDs;
    d->SubjectHierarchyNode->GetItemChildren(d->SubjectHierarchyNode->GetSceneItemID(), allItemIDs, true);
    for (std::vector<vtkIdType>::iterator itemIt=allItemIDs.begin(); itemIt!=allItemIDs.end(); ++itemIt)
      {
      this->updateModelItems(*itemIt);
      }
    }
}

//------------------------------------------------------------------------------
int qMRMLSubjectHierarchyModel::maxColumnId()const
{
  Q_D(const qMRMLSubjectHierarchyModel);
  int maxId = 0;
  maxId = qMax(maxId, d->NameColumn);
  maxId = qMax(maxId, d->IDColumn);
  maxId = qMax(maxId, d->VisibilityColumn);
  maxId = qMax(maxId, d->TransformColumn);
  return maxId;
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::onHardenTransformOnBranchOfCurrentItem()
{
  Q_D(const qMRMLSubjectHierarchyModel);
  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  vtkIdType currentItemID = qSlicerSubjectHierarchyPluginHandler::instance()->currentItem();
  if (currentItemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    vtkSlicerSubjectHierarchyModuleLogic::HardenTransformOnBranch(d->SubjectHierarchyNode, currentItemID);
    }

  QApplication::restoreOverrideCursor();
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyModel::onRemoveTransformsFromBranchOfCurrentItem()
{
  Q_D(const qMRMLSubjectHierarchyModel);
  vtkIdType currentItemID = qSlicerSubjectHierarchyPluginHandler::instance()->currentItem();
  if (currentItemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    vtkSlicerSubjectHierarchyModuleLogic::TransformBranch(d->SubjectHierarchyNode, currentItemID, NULL, false);
    }
}

//------------------------------------------------------------------------------
void printStandardItem(QStandardItem* item, const QString& offset)
{
  if (!item)
    {
    return;
    }
  qDebug() << offset << item << item->index() << item->text()
           << item->data(qMRMLSubjectHierarchyModel::SubjectHierarchyItemIDRole).toString() << item->row()
           << item->column() << item->rowCount() << item->columnCount();
  for(int i = 0; i < item->rowCount(); ++i )
    {
    for (int j = 0; j < item->columnCount(); ++j)
      {
      printStandardItem(item->child(i,j), offset + "   ");
      }
    }
}
