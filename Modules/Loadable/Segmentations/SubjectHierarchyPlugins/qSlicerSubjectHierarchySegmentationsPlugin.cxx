/*==============================================================================

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

// Segmentations includes
#include "qSlicerSubjectHierarchySegmentationsPlugin.h"

#include "qSlicerSubjectHierarchySegmentsPlugin.h"
#include "vtkMRMLSegmentationNode.h"
#include "vtkSegmentation.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"

// Qt includes
#include <QDebug>
#include <QAction>
#include <QMenu>
#include <QIcon>
#include <QMessageBox>

// MRML includes
#include <vtkMRMLScene.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy_Widgets
class qSlicerSubjectHierarchySegmentationsPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchySegmentationsPlugin);
protected:
  qSlicerSubjectHierarchySegmentationsPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchySegmentationsPluginPrivate(qSlicerSubjectHierarchySegmentationsPlugin& object);
  ~qSlicerSubjectHierarchySegmentationsPluginPrivate();
  void init();
public:
  QIcon SegmentationIcon;

  QAction* CreateRepresentationAction;
  QAction* CreateBinaryLabelmapAction;
  QAction* CreateClosedSurfaceAction;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchySegmentationsPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchySegmentationsPluginPrivate::qSlicerSubjectHierarchySegmentationsPluginPrivate(qSlicerSubjectHierarchySegmentationsPlugin& object)
: q_ptr(&object)
, SegmentationIcon(QIcon(":Icons/Segmentation.png"))
{
  this->CreateRepresentationAction = NULL;
  this->CreateBinaryLabelmapAction = NULL;
  this->CreateClosedSurfaceAction = NULL;
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentationsPluginPrivate::init()
{
  Q_Q(qSlicerSubjectHierarchySegmentationsPlugin);

  // Convert to representation action
  this->CreateRepresentationAction = new QAction("Create representation",q);
  QMenu* createRepresentationSubMenu = new QMenu();
  this->CreateRepresentationAction->setMenu(createRepresentationSubMenu);

  this->CreateBinaryLabelmapAction = new QAction("Binary labelmap",q);
  QObject::connect(this->CreateBinaryLabelmapAction, SIGNAL(triggered()), q, SLOT(createBinaryLabelmapRepresentation()));
  createRepresentationSubMenu->addAction(this->CreateBinaryLabelmapAction);

  this->CreateClosedSurfaceAction = new QAction("Closed surface",q);
  QObject::connect(this->CreateClosedSurfaceAction, SIGNAL(triggered()), q, SLOT(createClosedSurfaceRepresentation()));
  createRepresentationSubMenu->addAction(this->CreateClosedSurfaceAction);
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchySegmentationsPluginPrivate::~qSlicerSubjectHierarchySegmentationsPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchySegmentationsPlugin::qSlicerSubjectHierarchySegmentationsPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchySegmentationsPluginPrivate(*this) )
{
  this->m_Name = QString("Segmentations");

  Q_D(qSlicerSubjectHierarchySegmentationsPlugin);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchySegmentationsPlugin::~qSlicerSubjectHierarchySegmentationsPlugin()
{
}

//----------------------------------------------------------------------------
double qSlicerSubjectHierarchySegmentationsPlugin::canAddNodeToSubjectHierarchy(
  vtkMRMLNode* node, SubjectHierarchyItemID parentItemID/*=vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID*/)const
{
  Q_UNUSED(parentItemID);
  if (!node)
    {
    qCritical() << Q_FUNC_INFO << ": Input node is NULL!";
    return 0.0;
    }
  else if (node->IsA("vtkMRMLSegmentationNode"))
    {
    // Node is a segmentation
    return 0.9;
    }
  return 0.0;
}

//----------------------------------------------------------------------------
bool qSlicerSubjectHierarchySegmentationsPlugin::addNodeToSubjectHierarchy(vtkMRMLNode* nodeToAdd,
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID parentItemID, std::string level/*=""*/)
{
  if (!qSlicerSubjectHierarchyAbstractPlugin::addNodeToSubjectHierarchy(nodeToAdd, parentItemID, level))
    {
    return false;
    }
  vtkMRMLSegmentationNode* addedSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(nodeToAdd);
  if (!addedSegmentationNode)
    {
    qWarning() << Q_FUNC_INFO << ": segmentation node was expected";
    return false;
    }
  this->updateAllSegmentsFromMRML(addedSegmentationNode);
  return true;
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchySegmentationsPlugin::canOwnSubjectHierarchyItem(
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID)const
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return 0.0;
    }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return 0.0;
    }

  // Segmentation
  vtkMRMLNode* associatedNode = shNode->GetItemDataNode(itemID);
  if (associatedNode && associatedNode->IsA("vtkMRMLSegmentationNode"))
    {
    // Make sure the segmentation subject hierarchy item indicates its virtual branch
    shNode->SetItemAttribute(itemID,
      vtkMRMLSubjectHierarchyConstants::GetVirtualBranchSubjectHierarchyNodeAttributeName().c_str(), "1");
    return 0.9;
    }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchySegmentationsPlugin::roleForPlugin()const
{
  return "Segmentation";
}

//-----------------------------------------------------------------------------
QString qSlicerSubjectHierarchySegmentationsPlugin::tooltip(
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID)const
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return QString("Invalid!");
    }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return QString("Invalid!");
    }

  // Get basic tooltip from abstract plugin
  QString tooltipString = Superclass::tooltip(itemID);

  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(shNode->GetItemDataNode(itemID));
  if (!segmentationNode)
    {
    qCritical() << Q_FUNC_INFO << ": Subject hierarchy item not associated to valid segmentation node!";
    return tooltipString;
    }

  // Representations
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  std::vector<std::string> containedRepresentationNames;
  segmentation->GetContainedRepresentationNames(containedRepresentationNames);
  tooltipString.append( QString(" (Representations: ") );
  if (containedRepresentationNames.empty())
    {
    tooltipString.append( QString("None!)") );
    }
  else
    {
    for (std::vector<std::string>::iterator reprIt = containedRepresentationNames.begin();
      reprIt != containedRepresentationNames.end(); ++reprIt)
      {
      tooltipString.append( reprIt->c_str() );
      tooltipString.append( ", " );
      }
    tooltipString = tooltipString.left(tooltipString.length()-2).append(")");
    }

  // Master representation
  tooltipString.append(QString(" (Master representation: %1)").arg(segmentation->GetMasterRepresentationName().c_str()));

  // Number of segments
  tooltipString.append(QString(" (Number of segments: %1)").arg(segmentation->GetNumberOfSegments()));

  return tooltipString;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchySegmentationsPlugin::helpText()const
{
  //TODO:
  //return QString("<p style=\" margin-top:4px; margin-bottom:1px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'sans-serif'; font-size:9pt; font-weight:600; color:#000000;\">Create new Contour set from scratch</span></p>"
  //  "<p style=\" margin-top:0px; margin-bottom:11px; margin-left:26px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'sans-serif'; font-size:9pt; color:#000000;\">Right-click on an existing Study node and select 'Create child contour set'. This menu item is only available for Study level nodes</span></p>");
  return QString();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchySegmentationsPlugin::icon(vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID)
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return QIcon();
    }

  Q_D(qSlicerSubjectHierarchySegmentationsPlugin);

  // Segmentation
  if (this->canOwnSubjectHierarchyItem(itemID))
    {
    return d->SegmentationIcon;
    }

  // Item unknown by plugin
  return QIcon();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchySegmentationsPlugin::visibilityIcon(int visible)
{
  // Have the default plugin (which is not registered) take care of this
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->visibilityIcon(visible);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentationsPlugin::setDisplayVisibility(
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID, int visible)
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return;
    }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
    }

  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(shNode->GetItemDataNode(itemID));
  if (!segmentationNode)
    {
    qCritical() << Q_FUNC_INFO << ": Subject hierarchy item not associated to valid segmentation node!";
    return;
    }

  segmentationNode->SetDisplayVisibility(visible);

  // Trigger updating subject hierarchy visibility icon by calling modified on the segmentation SH node and all its parents
  std::set<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID> parentItems;
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID parentItem = shNode->GetSubjectHierarchyItemByDataNode(segmentationNode);
  do
    {
    parentItems.insert(parentItem);
    }
  while ( (parentItem = shNode->GetItemParent(parentItem) ) != shNode->GetSceneItemID() ); // The double parentheses avoids a Linux build warning

  std::set<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID>::iterator parentsIt;
  for (parentsIt=parentItems.begin(); parentsIt!=parentItems.end(); ++parentsIt)
    {
//TODO: Remove section if works without it. Invoke item modified items if not
    //(*parentsIt)->Modified();
    }
}

//-----------------------------------------------------------------------------
int qSlicerSubjectHierarchySegmentationsPlugin::getDisplayVisibility(
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID)const
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return -1;
    }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return -1;
    }

  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(shNode->GetItemDataNode(itemID));
  if (!segmentationNode)
    {
    qCritical() << Q_FUNC_INFO << ": Subject hierarchy item not associated to valid segmentation node!";
    return -1;
    }

  return segmentationNode->GetDisplayVisibility();
}

//---------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchySegmentationsPlugin::itemContextMenuActions()const
{
  Q_D(const qSlicerSubjectHierarchySegmentationsPlugin);

  QList<QAction*> actions;
  actions << d->CreateRepresentationAction;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentationsPlugin::showContextMenuActionsForItem(
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID)
{
  Q_D(qSlicerSubjectHierarchySegmentationsPlugin);
  this->hideAllContextMenuActions();

  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    // There are no scene actions in this plugin
    return;
    }

  // Owned Segmentation or Segment (segments plugin shows all segmentations plugin functions in segment context menu)
  qSlicerSubjectHierarchySegmentsPlugin* segmentsPlugin = qobject_cast<qSlicerSubjectHierarchySegmentsPlugin*>(
    qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Segments") );
  if ( (this->canOwnSubjectHierarchyItem(itemID) && this->isThisPluginOwnerOfItem(itemID))
    || (segmentsPlugin->canOwnSubjectHierarchyItem(itemID) && segmentsPlugin->isThisPluginOwnerOfItem(itemID)) )
    {
    d->CreateRepresentationAction->setVisible(true);
    }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentationsPlugin::onSegmentAdded(vtkObject* caller, void* callData)
{
  // Get segmentation node
  vtkMRMLSegmentationNode* segmentationNode = reinterpret_cast<vtkMRMLSegmentationNode*>(caller);
  if (!segmentationNode)
    {
    return;
    }
  if (segmentationNode->GetScene()->IsImporting())
    {
    // During scene import SH may not exist yet (if the scene was created without automatic SH creation)
    return;
    }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
    }

  // Get associated subject hierarchy node
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID segmentationShItemID =
    shNode->GetSubjectHierarchyItemByDataNode(segmentationNode);
  if (segmentationShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy item for segmentation node " << segmentationNode->GetName();
    return;
    }

  // Get segment ID and segment
  char* segmentId = reinterpret_cast<char*>(callData);
  if (!segmentId)
    {
    // Calling InvokePendingModifiedEvent loses event parameters, so in this case segment IDs are empty
    this->updateAllSegmentsFromMRML(segmentationNode);
    return;
    }
  vtkSegment* segment = segmentationNode->GetSegmentation()->GetSegment(segmentId);
  if (!segment)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to get added segment with ID '" << segmentId << "'";
    return;
    }

  // Add the segment in subject hierarchy to allow individual handling (e.g. visibility)
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID segmentShItemID = shNode->CreateSubjectHierarchyItem(
    segmentationShItemID, NULL, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries(), segment->GetName() );
  shNode->SetItemAttribute(segmentShItemID, vtkMRMLSegmentationNode::GetSegmentIDAttributeName(), segmentId);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentationsPlugin::onSegmentRemoved(vtkObject* caller, void* callData)
{
  // Get segmentation node
  vtkMRMLSegmentationNode* segmentationNode = reinterpret_cast<vtkMRMLSegmentationNode*>(caller);
  if (!segmentationNode)
    {
    return;
    }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
    }

  // Get associated subject hierarchy item
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID segmentationShItemID =
    shNode->GetSubjectHierarchyItemByDataNode(segmentationNode);
  if (segmentationShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Subject hierarchy item cannot be found for segmentation node "
      << segmentationNode->GetName() << " so per-segment subject hierarchy node cannot be removed.";
    return;
    }

  // Get segment ID
  char* segmentId = reinterpret_cast<char*>(callData);
  if (!segmentId)
    {
    // Calling InvokePendingModifiedEvent loses event parameters, so in this case segment IDs are empty
    this->updateAllSegmentsFromMRML(segmentationNode);
    return;
    }

  // Find subject hierarchy item for segment
  std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID> segmentShItemIDs;
  shNode->GetItemChildren(segmentationShItemID, segmentShItemIDs);
  std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID>::iterator segmentIt;
  for (segmentIt = segmentShItemIDs.begin(); segmentIt != segmentShItemIDs.end(); ++segmentIt)
    {
    std::string currentSegmentId = shNode->GetItemAttribute(*segmentIt, vtkMRMLSegmentationNode::GetSegmentIDAttributeName());
    if (!currentSegmentId.compare(segmentId))
      {
      shNode->RemoveSubjectHierarchyItem(*segmentIt);
      return;
      }
    }

  // Log message if segment subject hierarchy item was not found
  qDebug() << Q_FUNC_INFO << ": Unable to find subject hierarchy item for segment" << segmentId << " in segmentation " << segmentationNode->GetName();
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentationsPlugin::onSegmentModified(vtkObject* caller, void* callData)
{
  // Get segmentation node
  vtkMRMLSegmentationNode* segmentationNode = reinterpret_cast<vtkMRMLSegmentationNode*>(caller);
  if (!segmentationNode)
    {
    return;
    }
  if (segmentationNode->GetScene()->IsImporting())
    {
    // during scene import SH may not exist yet (if the scene was created without automatic SH creation)
    return;
    }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
    }

  // Get associated subject hierarchy item
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID segmentationShItemID =
    shNode->GetSubjectHierarchyItemByDataNode(segmentationNode);
  if (segmentationShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Unable to find subject hierarchy item for segmentation node "
      << segmentationNode->GetName() << " so per-segment subject hierarchy node cannot be created";
    return;
    }

  // Get segment ID and segment
  char* segmentId = reinterpret_cast<char*>(callData);
  if (!segmentId)
    {
    // no segmentId is specified - it means that any and all may have been changed
    this->updateAllSegmentsFromMRML(segmentationNode);
    return;
    }

  vtkSegment* segment = segmentationNode->GetSegmentation()->GetSegment(segmentId);
  if (!segment)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to get added segment with ID '" << segmentId << "'";
    return;
    }

  // Find subject hierarchy item for segment
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID segmentShItemID = vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
  std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID> segmentShItemIDs;
  shNode->GetItemChildren(segmentationShItemID, segmentShItemIDs);
  std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID>::iterator segmentIt;
  for (segmentIt = segmentShItemIDs.begin(); segmentIt != segmentShItemIDs.end(); ++segmentIt)
    {
    std::string currentSegmentId = shNode->GetItemAttribute(*segmentIt, vtkMRMLSegmentationNode::GetSegmentIDAttributeName());
    if (!currentSegmentId.compare(segmentId))
      {
      segmentShItemID = (*segmentIt);
      break;
      }
    }

  // Rename segment subject hierarchy item if segment name is different (i.e. has just been renamed)
  if (shNode->GetItemName(segmentShItemID).compare(segment->GetName()))
    {
    shNode->SetItemName(segmentShItemID, segment->GetName());
    }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentationsPlugin::createBinaryLabelmapRepresentation()
{
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
    }
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID currentItemID =
    qSlicerSubjectHierarchyPluginHandler::instance()->currentItem();
  if (currentItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid current item!";
    return;
    }
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(shNode->GetItemDataNode(currentItemID));

  // Segmentations plugin provides the functionality for segments too, see if it is a segment
  if (!segmentationNode)
    {
    vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID parentItemID = shNode->GetItemParent(currentItemID);
    if (parentItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
      {
      return;
      }
    segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(shNode->GetItemDataNode(parentItemID));
    }

  // Create binary labelmap representation using default parameters
  bool success = segmentationNode->GetSegmentation()->CreateRepresentation(
    vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
  if (!success)
    {
    QString message = QString("Failed to create binary labelmap representation in segmentation %1 using default conversion parameters!\n\nPlease visit the Segmentation module and try the advanced create representation function.").
      arg(segmentationNode->GetName());
    QMessageBox::warning(NULL, tr("Failed to create binary labelmap"), message);
    }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentationsPlugin::createClosedSurfaceRepresentation()
{
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
    }
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID currentItemID =
    qSlicerSubjectHierarchyPluginHandler::instance()->currentItem();
  if (currentItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid current item!";
    return;
    }
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(shNode->GetItemDataNode(currentItemID));

  // Segmentations plugin provides the functionality for segments too, see if it is a segment
  if (!segmentationNode)
    {
    vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID parentItemID = shNode->GetItemParent(currentItemID);
    if (parentItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
      {
      return;
      }
    segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(shNode->GetItemDataNode(parentItemID));
    }

  // Create closed surface representation using default parameters
  bool success = segmentationNode->GetSegmentation()->CreateRepresentation(
    vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
  if (!success)
    {
    QString message = QString("Failed to create closed surface representation in segmentation %1 using default conversion parameters!\n\nPlease visit the Segmentation module and try the advanced create representation function.").
      arg(segmentationNode->GetName());
    QMessageBox::warning(NULL, tr("Failed to create closed surface"), message);
    }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentationsPlugin::updateAllSegmentsFromMRML(vtkMRMLSegmentationNode* segmentationNode)
{
  // Get segmentation node
  if (!segmentationNode)
    {
    qWarning() << Q_FUNC_INFO << ": invalid segmentation node";
    return;
    }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
    }

  // Get associated subject hierarchy item
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID segmentationShItemID =
    shNode->GetSubjectHierarchyItemByDataNode(segmentationNode);
  if (segmentationShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    qCritical() << Q_FUNC_INFO << ": Unable to find subject hierarchy item for segmentation node "
      << segmentationNode->GetName() << " so per-segment subject hierarchy node cannot be created";
    return;
    }
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  if (!segmentation)
    {
    qWarning() << Q_FUNC_INFO << ": invalid segmentation";
    return;
    }

  // List of segment IDs that have to be added to the segment list
  std::vector<std::string> segmentIDsToBeAddedToSh;
  segmentationNode->GetSegmentation()->GetSegmentIDs(segmentIDsToBeAddedToSh);

  // Segment modify/remove
  std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID> segmentShItemIDs;
  shNode->GetItemChildren(segmentationShItemID, segmentShItemIDs);
  std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID>::iterator segmentIt;
  for (segmentIt = segmentShItemIDs.begin(); segmentIt != segmentShItemIDs.end(); ++segmentIt)
    {
    std::string segmentId = shNode->GetItemAttribute(*segmentIt, vtkMRMLSegmentationNode::GetSegmentIDAttributeName());
    vtkSegment* segment = segmentation->GetSegment(segmentId);
    if (!segment)
      {
      // Segment has been removed
      this->onSegmentRemoved(segmentationNode, (void*)(segmentId.c_str()));
      continue;
      }
    this->onSegmentModified(segmentationNode, (void*)(segmentId.c_str()));

    // Remove segment ID from the list of segments to be added (it's already added)
    segmentIDsToBeAddedToSh.erase(std::remove(segmentIDsToBeAddedToSh.begin(), segmentIDsToBeAddedToSh.end(), segmentId), segmentIDsToBeAddedToSh.end());
    }

  // Segment add
  for (std::vector<std::string>::iterator segmentIdIt = segmentIDsToBeAddedToSh.begin(); segmentIdIt != segmentIDsToBeAddedToSh.end(); ++segmentIdIt)
    {
    this->onSegmentAdded(segmentationNode, (void*)(segmentIdIt->c_str()));
    }
}
