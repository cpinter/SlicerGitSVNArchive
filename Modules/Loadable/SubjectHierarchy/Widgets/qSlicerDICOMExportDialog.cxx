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

// Qt includes
#include <QDialog>
#include <QObject>
#include <QDebug>

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
  this->SubjectHierarchyTreeView->hideColumn(sceneModel->visibilityColumn());
  //this->SubjectHierarchyTreeView->hideColumn(sceneModel->transformColumn());
  this->SubjectHierarchyTreeView->header()->resizeSection(sceneModel->transformColumn(), 60);
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
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerDICOMExportDialog::~qSlicerDICOMExportDialog()
{
}

//-----------------------------------------------------------------------------
bool qSlicerDICOMExportDialog::exec()
{
  Q_D(qSlicerDICOMExportDialog);

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
