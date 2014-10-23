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

// SubjectHierarchy MRML includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDICOMPlugin.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"

// DICOMLib includes
#include "qSlicerDICOMExportDialog.h"

// Qt includes
#include <QDebug>
#include <QAction>
#include <QStandardItem>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLStorableNode.h>
#include <vtkMRMLStorageNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy_Widgets
class qSlicerSubjectHierarchyDICOMPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyDICOMPlugin);
protected:
  qSlicerSubjectHierarchyDICOMPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyDICOMPluginPrivate(qSlicerSubjectHierarchyDICOMPlugin& object);
  ~qSlicerSubjectHierarchyDICOMPluginPrivate();
  void init();
public:
  QIcon PatientIcon;

  QAction* CreateGenericSeriesAction;
  QAction* CreateGenericSubseriesAction;
  QAction* OpenDICOMExportDialogAction;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyDICOMPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDICOMPluginPrivate::qSlicerSubjectHierarchyDICOMPluginPrivate(qSlicerSubjectHierarchyDICOMPlugin& object)
: q_ptr(&object)
{
  this->PatientIcon = QIcon(":Icons/Patient.png");

  this->CreateGenericSeriesAction = NULL;
  this->CreateGenericSubseriesAction = NULL;
  this->OpenDICOMExportDialogAction = NULL;
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchyDICOMPluginPrivate::init()
{
  Q_Q(qSlicerSubjectHierarchyDICOMPlugin);

  this->CreateGenericSeriesAction = new QAction("Create child generic series",q);
  QObject::connect(this->CreateGenericSeriesAction, SIGNAL(triggered()), q, SLOT(createChildForCurrentNode()));

  this->CreateGenericSubseriesAction = new QAction("Create child generic subseries",q);
  QObject::connect(this->CreateGenericSubseriesAction, SIGNAL(triggered()), q, SLOT(createChildForCurrentNode()));

  this->OpenDICOMExportDialogAction = new QAction("Export to DICOM...",q);
  QObject::connect(this->OpenDICOMExportDialogAction, SIGNAL(triggered()), q, SLOT(openDICOMExportDialog()));
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDICOMPluginPrivate::~qSlicerSubjectHierarchyDICOMPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDICOMPlugin::qSlicerSubjectHierarchyDICOMPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyDICOMPluginPrivate(*this) )
{
  this->m_Name = QString("DICOM");

  // Scene (empty level) -> Subject
  qSlicerSubjectHierarchyAbstractPlugin::m_ChildLevelMap.insert( QString(),
    vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelSubject() );
  // Subject -> Study
  qSlicerSubjectHierarchyAbstractPlugin::m_ChildLevelMap.insert( vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelSubject(),
    vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelStudy() );
  // Study -> Series
  qSlicerSubjectHierarchyAbstractPlugin::m_ChildLevelMap.insert( vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelStudy(),
    vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries() );
  // Series -> Subseries
  qSlicerSubjectHierarchyAbstractPlugin::m_ChildLevelMap.insert( vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries(),
    vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries() );
  // Subseries -> Subseries
  qSlicerSubjectHierarchyAbstractPlugin::m_ChildLevelMap.insert( vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries(),
    vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries() );

  Q_D(qSlicerSubjectHierarchyDICOMPlugin);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDICOMPlugin::~qSlicerSubjectHierarchyDICOMPlugin()
{
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyDICOMPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
    {
    qCritical() << "qSlicerSubjectHierarchyDICOMPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
    }

  // Subject level
  if (node->IsLevel(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelSubject()))
    {
    return 0.7;
    }
  // Study level (so that creation of a generic series is possible)
  if (node->IsLevel(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelStudy()))
    {
    return 0.3;
    }
  // Series level
  if (node->IsLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries()))
    {
    return 0.3;
    }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyDICOMPlugin::roleForPlugin()const
{
  // Get current node to determine tole
  //TODO: This is a workaround, needs to be fixed (each plugin should provide only one role!)
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  if (!currentNode)
    {
    qCritical() << "qSlicerSubjectHierarchyDICOMPlugin::roleForPlugin: Invalid current node!";
    return "Error!";
    }

  // Subject level
  if (currentNode->IsLevel(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelSubject()))
    {
    return "Patient";
    }
  // Study level (so that creation of a generic series is possible)
  if (currentNode->IsLevel(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelStudy()))
    {
    return "Study";
    }
  // Series level
  if (currentNode->IsLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries()))
    {
    return "Generic series";
    }

  return QString("Error!");
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyDICOMPlugin::helpText()const
{
  return QString(
    "<p style=\" margin-top:4px; margin-bottom:1px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">"
    "<span style=\" font-family:'sans-serif'; font-size:9pt; font-weight:600; color:#000000;\">"
    "Create new generic Subject hierarchy node from scratch"
    "</span>"
    "</p>"
    "<p style=\" margin-top:0px; margin-bottom:11px; margin-left:26px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">"
    "<span style=\" font-family:'sans-serif'; font-size:9pt; color:#000000;\">"
    "Right-click on an existing node and select 'Create generic child node'. "
    "The level of the child node will be one under the parent node if available (e.g. 'Subject' -&gt; 'Study', 'Subseries' -&gt; 'Subseries')."
    "</span>"
    "</p>");
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyDICOMPlugin::icon(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
    {
    qCritical() << "qSlicerSubjectHierarchyDICOMPlugin::icon: NULL node given!";
    return QIcon();
    }

  Q_D(qSlicerSubjectHierarchyDICOMPlugin);

  // Patient icon
  if (node->IsLevel(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelSubject()))
    {
    return d->PatientIcon;
    }
  // Study icon
  if (node->IsLevel(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelStudy()))
    {
    return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->icon(node);
    }

  // Node unknown by plugin
  return QIcon();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyDICOMPlugin::visibilityIcon(int visible)
{
  // Have the default plugin (which is not registered) take care of this
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->visibilityIcon(visible);
}

//---------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchyDICOMPlugin::nodeContextMenuActions()const
{
  Q_D(const qSlicerSubjectHierarchyDICOMPlugin);

  QList<QAction*> actions;
  actions << d->CreateGenericSeriesAction << d->CreateGenericSubseriesAction << d->OpenDICOMExportDialogAction;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDICOMPlugin::showContextMenuActionsForNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qSlicerSubjectHierarchyDICOMPlugin);
  this->hideAllContextMenuActions();

  if (!node)
    {
    // There are no scene actions in this plugin
    return;
    }

  // Study
  if (node->IsLevel(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelStudy()))
    {
    d->CreateGenericSeriesAction->setVisible(true);
    //if (this->canBeExported(node)) //TODO:
      {
      d->OpenDICOMExportDialogAction->setVisible(true);
      }
    }
  // Series or Subseries
  else if (node->IsLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries()))
    {
    d->CreateGenericSubseriesAction->setVisible(true);
    //if (this->canBeExported(node)) //TODO:
      {
      d->OpenDICOMExportDialogAction->setVisible(true);
      }
    }
  else if (node->IsLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries()))
    {
    d->CreateGenericSubseriesAction->setVisible(true);
    }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDICOMPlugin::editProperties(vtkMRMLSubjectHierarchyNode* node)
{
  Q_UNUSED(node);
  //TODO: Show DICOM tag editor? Only for studies and subjects (only owns these kinds). Series? Probably owned by something else
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDICOMPlugin::openDICOMExportDialog()
{
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  if (!currentNode)
  {
    qCritical() << "qSlicerSubjectHierarchyDICOMPlugin::openDICOMExportDialog: Invalid current node!";
    return;
  }

  qSlicerDICOMExportDialog* exportDialog = new qSlicerDICOMExportDialog(NULL);
  exportDialog->setMRMLScene(qSlicerSubjectHierarchyPluginHandler::instance()->scene());
  exportDialog->exec(currentNode);

  delete exportDialog;
}
