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

// Qt includes
#include <QDialog>
#include <QObject>
#include <QDebug>

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
private:
  friend class qSlicerDICOMExportDialog;
};

//-----------------------------------------------------------------------------
qSlicerDICOMExportDialogPrivate::qSlicerDICOMExportDialogPrivate(qSlicerDICOMExportDialog& object)
  : q_ptr(&object)
  , Scene(NULL)
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
bool qSlicerDICOMExportDialog::exec()
{
  Q_D(qSlicerDICOMExportDialog);

  // Initialize dialog
  d->init();

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
void qSlicerDICOMExportDialog::onExport()
{
  Q_D(qSlicerDICOMExportDialog);

  // Get selected node
  QModelIndexList selectedIndices = d->SubjectHierarchyTreeView->selectionModel()->selectedRows();
  if (selectedIndices.size() < 1)
  {
    qCritical() << "qSlicerDICOMExportDialog::onExport: No subject hierarchy node selected!";
    return;
  }
  vtkMRMLSubjectHierarchyNode* selectedNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
    d->SubjectHierarchyTreeView->sortFilterProxyModel()->mrmlNodeFromIndex(selectedIndices.at(0)) ); // Single selection only
  if (!selectedNode)
  {
    qCritical() << "qSlicerDICOMExportDialog::onExport: Unable to get selected subject hierarchy node!";
    return;
  }

  // Get exportables from DICOM plugins
  PythonQt::init();
  PythonQtObjectPtr context = PythonQt::self()->getMainModule();
  context.evalScript( QString(
    "exportables = []\n"
    "loadables = []\n" //TODO
    "numOfLoadables = 0\n" //TODO
    "file1 = 'd:/devel/_Images/RT/20140923_ZahraDose/dose-2A/RTDose1.2.826.0.1.3680043.8.341.2.12048.4185466838053.14.dcm'\n" //TODO
    "file2 = 'd:/devel/_Images/RT/20140923_ZahraDose/dose-2B/RTDose1.2.826.0.1.3680043.8.341.2.12048.4185469684804.3.dcm'\n" //TODO
    "fileList = [file1,file2]\n" //TODO
    "selectedNode = slicer.mrmlScene.GetNodeByID('%1')\n"
    "print(selectedNode.GetName())\n" //TODO
    "for pluginClass in slicer.modules.dicomPlugins:\n"
    "  plugin = slicer.modules.dicomPlugins[pluginClass]()\n"
    "  if hasattr(plugin, 'examineForExport'):\n"
    "    exportables.append(plugin.examineForExport(selectedNode))\n"
    "  elif hasattr(plugin, 'examineFiles'):\n" //TODO
    "    loadables.append(plugin.examineFiles(fileList))\n" //TODO
    "    numOfLoadables += 1\n" //TODO
    "print numOfLoadables\n" ) //TODO
    .arg(selectedNode->GetID()) );
  //QVariant exportables = context.getVariable("exportables");
  QVariant numOfLoadablesVariant = context.getVariable("numOfLoadables"); //TODO
  unsigned int numOfLoadables = numOfLoadablesVariant.toUInt();
  QVariant fileListVariant = context.getVariable("fileList"); //TODO
  QStringList fileList = fileListVariant.toStringList();
  int size = fileList.size();
  QString file1 = fileList.at(0);
  QString file2 = fileList.at(1);
  QVariant node = context.getVariable("selectedNode"); //TODO

  int i=0; ++i;
}
