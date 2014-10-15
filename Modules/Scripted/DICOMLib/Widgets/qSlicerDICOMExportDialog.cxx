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

// SubjectHierarchy_Widgets includes
#include "qSlicerDICOMExportDialog.h"
#include "ui_qSlicerDICOMExportDialog.h"

#include "qMRMLSubjectHierarchyTreeView.h"
#include "qMRMLSceneSubjectHierarchyModel.h"

// SubjectHierarchy includes
#include "vtkMRMLSubjectHierarchyNode.h"

// DICOMLib includes
#include "qSlicerDICOMExportable.h"

// Qt includes
#include <QDialog>
#include <QObject>
#include <QDebug>
#include <QItemSelection>
#include <QListWidgetItem>

// PythonQt includes
#include "PythonQt.h"

// MRML includes
#include <vtkMRMLScene.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy_Widgets
class qSlicerDICOMExportDialogPrivate : public Ui_qSlicerDICOMExportDialog, public QDialog
{
  Q_DECLARE_PUBLIC(qSlicerDICOMExportDialog);
protected:
  qSlicerDICOMExportDialog* const q_ptr;
public:
  qSlicerDICOMExportDialogPrivate(qSlicerDICOMExportDialog& object);
  virtual ~qSlicerDICOMExportDialogPrivate();
public:
  void init();
private:
  vtkMRMLScene* Scene;
  qSlicerDICOMExportable* SelectedExportable;
private:
  friend class qSlicerDICOMExportDialog;
};

//-----------------------------------------------------------------------------
qSlicerDICOMExportDialogPrivate::qSlicerDICOMExportDialogPrivate(qSlicerDICOMExportDialog& object)
  : q_ptr(&object)
  , Scene(NULL)
  , SelectedExportable(NULL)
{
}

//-----------------------------------------------------------------------------
qSlicerDICOMExportDialogPrivate::~qSlicerDICOMExportDialogPrivate()
{
}

//-----------------------------------------------------------------------------
void qSlicerDICOMExportDialogPrivate::init()
{
  Q_Q(qSlicerDICOMExportDialog);

  qMRMLSceneSubjectHierarchyModel* sceneModel = (qMRMLSceneSubjectHierarchyModel*)this->SubjectHierarchyTreeView->sceneModel();

  // Set up tree view
  this->SubjectHierarchyTreeView->setMRMLScene(this->Scene);
  this->SubjectHierarchyTreeView->expandToDepth(4);
  this->SubjectHierarchyTreeView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
  this->SubjectHierarchyTreeView->hideColumn(sceneModel->idColumn());
  this->SubjectHierarchyTreeView->hideColumn(sceneModel->visibilityColumn());
  this->SubjectHierarchyTreeView->hideColumn(sceneModel->transformColumn());
  //this->SubjectHierarchyTreeView->header()->resizeSection(sceneModel->transformColumn(), 60);

  // Make connections
  connect(this->SubjectHierarchyTreeView, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(onCurrentNodeChanged(vtkMRMLNode*)));
  connect(this->ExportablesListWidget, SIGNAL(currentRowChanged(int)), q, SLOT(onExportableSelectedAtRow(int)));
  connect(this->ExportButton, SIGNAL(clicked()), q, SLOT(onExport()));
}

//-----------------------------------------------------------------------------
// qSlicerDICOMExportDialog methods

//-----------------------------------------------------------------------------
qSlicerDICOMExportDialog::qSlicerDICOMExportDialog(QObject* parent)
  : QObject(parent)
  , d_ptr(new qSlicerDICOMExportDialogPrivate(*this))
{
  Q_D(qSlicerDICOMExportDialog);
  d->setupUi(d);
}

//-----------------------------------------------------------------------------
qSlicerDICOMExportDialog::~qSlicerDICOMExportDialog()
{
}

//-----------------------------------------------------------------------------
bool qSlicerDICOMExportDialog::exec(vtkMRMLSubjectHierarchyNode* nodeToSelect/*=NULL*/)
{
  Q_D(qSlicerDICOMExportDialog);

  // Initialize dialog
  d->init();

  // Make selection if requested
  if (nodeToSelect)
  {
    QApplication::processEvents();
    this->selectNode(nodeToSelect);
  }

  // Show dialog
  bool result = false;
  if (d->exec() != QDialog::Accepted)
  {
    return result;
  }

  // Perform actions after clean exit
  result = true;

  return result;
}

//-----------------------------------------------------------------------------
void qSlicerDICOMExportDialog::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerDICOMExportDialog);
  d->Scene = scene;
}

//-----------------------------------------------------------------------------
void qSlicerDICOMExportDialog::selectNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qSlicerDICOMExportDialog);
  QModelIndex selectionIndex = d->SubjectHierarchyTreeView->sceneModel()->indexFromNode(node);
  d->SubjectHierarchyTreeView->selectionModel()->select(selectionIndex, QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
  d->SubjectHierarchyTreeView->setCurrentNode(node);
}

//-----------------------------------------------------------------------------
void qSlicerDICOMExportDialog::onCurrentNodeChanged(vtkMRMLNode* node)
{
  this->examineSelectedNode();
}

//-----------------------------------------------------------------------------
void qSlicerDICOMExportDialog::examineSelectedNode()
{
  Q_D(qSlicerDICOMExportDialog);

  vtkMRMLSubjectHierarchyNode* selectedNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
    d->SubjectHierarchyTreeView->currentNode() );
  if (!selectedNode)
  {
    qCritical() << "qSlicerDICOMExportDialog::examineSelectedNode: Unable to get selected subject hierarchy node!";
    return;
  }

  // Get exportables from DICOM plugins
  PythonQt::init();
  PythonQtObjectPtr context = PythonQt::self()->getMainModule();
  context.evalScript( QString(
    "exportables = []\n"
    "selectedNode = slicer.mrmlScene.GetNodeByID('%1')\n"
    "for pluginClass in slicer.modules.dicomPlugins:\n"
    "  plugin = slicer.modules.dicomPlugins[pluginClass]()\n"
    "  exportables.extend(plugin.examineForExport(selectedNode))\n" )
    .arg(selectedNode->GetID()) );

  // Extract resulting exportables from python and order them by confidence values
  d->ExportablesListWidget->clear();
  QList<QVariant> exportablesVariantList = context.getVariable("exportables").toList();
  QMap<double,qSlicerDICOMExportable*> exportablesMap;
  foreach(QVariant exportableVariant, exportablesVariantList)
  {
    // Get exportable object (to compose item text)
    qSlicerDICOMExportable* exportable = qobject_cast<qSlicerDICOMExportable*>(
      exportableVariant.value<QObject*>() );
    if (!exportable)
    {
      qCritical() << "qSlicerDICOMExportDialog::examineSelectedNode: Invalid exportable returned by DICOM plugin for node " << selectedNode->GetNameWithoutPostfix().c_str();
      continue;
    }

    // Add exportable to map with confidence as key. Confidence value is subtracted
    // from 1 so that iterating through the map automatically orders the exportables.
    exportablesMap[1.0 - exportable->confidence()] = exportable;
  }

  // Populate the exportables list widget
  QMapIterator<double,qSlicerDICOMExportable*> exportableIterator(exportablesMap);
  while (exportableIterator.hasNext())
  {
    exportableIterator.next();
    qSlicerDICOMExportable* exportable = exportableIterator.value();
    QString itemText = QString("%1 (%2%)").arg(exportable->name()).arg(exportable->confidence()*100.0, 0, 'f', 0);
    QListWidgetItem* exportableItem = new QListWidgetItem(itemText, d->ExportablesListWidget);
    exportableItem->setToolTip(exportable->tooltip());
    exportableItem->setData(Qt::UserRole, QVariant::fromValue<QObject*>(exportable));
  }

  // Select exportable with highest confidence (top one)
  d->ExportablesListWidget->setCurrentRow(0);
}

//-----------------------------------------------------------------------------
void qSlicerDICOMExportDialog::onExportableSelectedAtRow(int row)
{
  Q_D(qSlicerDICOMExportDialog);

  // Get exportable object from list item
  QListWidgetItem* exportableItem = d->ExportablesListWidget->item(row);
  qSlicerDICOMExportable* exportable = qobject_cast<qSlicerDICOMExportable*>(
    exportableItem->data(Qt::UserRole).value<QObject*>() );
  if (!exportable)
  {
    qCritical() << "qSlicerDICOMExportDialog::onExportableSelectedAtRow: Unable to extract exportable";
    return;
  }

  // Populate DICOM tag editor from exportable
  //TODO:
}

//-----------------------------------------------------------------------------
void qSlicerDICOMExportDialog::onExport()
{
  //TODO:
}
