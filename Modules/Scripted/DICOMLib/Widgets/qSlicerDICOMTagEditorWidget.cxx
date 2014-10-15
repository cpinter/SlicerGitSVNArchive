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

// SlicerQt includes
#include "qSlicerApplication.h"

// SubjectHierarchy includes
#include "qSlicerDICOMTagEditorWidget.h"

// MRML includes
#include <vtkMRMLScene.h>

//------------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy
class qSlicerDICOMTagEditorWidgetPrivate
{
  Q_DECLARE_PUBLIC(qSlicerDICOMTagEditorWidget);
protected:
  qSlicerDICOMTagEditorWidget* const q_ptr;
public:
  qSlicerDICOMTagEditorWidgetPrivate(qSlicerDICOMTagEditorWidget& object);
  virtual void init();

  QList<qSlicerDICOMExportable*> Exportables;
};

//------------------------------------------------------------------------------
qSlicerDICOMTagEditorWidgetPrivate::qSlicerDICOMTagEditorWidgetPrivate(qSlicerDICOMTagEditorWidget& object)
  : q_ptr(&object)
{
}

//------------------------------------------------------------------------------
void qSlicerDICOMTagEditorWidgetPrivate::init()
{
  Q_Q(qSlicerDICOMTagEditorWidget);

  //TODO:
}

//------------------------------------------------------------------------------
qSlicerDICOMTagEditorWidget::qSlicerDICOMTagEditorWidget(QWidget *parent)
  : QWidget(parent)
{
  Q_D(qSlicerDICOMTagEditorWidget);
  d->init();
}

//------------------------------------------------------------------------------
qSlicerDICOMTagEditorWidget::~qSlicerDICOMTagEditorWidget()
{
}

//------------------------------------------------------------------------------
QString qSlicerDICOMTagEditorWidget::setExportables(QList<qSlicerDICOMExportable*> exportables)
{
  Q_D(qSlicerDICOMTagEditorWidget);

  // Check if the exportables are in the same study
  foreach (qSlicerDICOMExportable* exportable, exportables)
  {

  }

  return QString();
}

//------------------------------------------------------------------------------
QList<qSlicerDICOMExportable*> qSlicerDICOMTagEditorWidget::exportables()const
{
  Q_D(const qSlicerDICOMTagEditorWidget);
  return d->Exportables;
}

//------------------------------------------------------------------------------
void qSlicerDICOMTagEditorWidget::clear()
{
  Q_D(qSlicerDICOMTagEditorWidget);

  //TODO:
}

//------------------------------------------------------------------------------
void qSlicerDICOMTagEditorWidget::commitChangesToNodes()
{
  Q_D(qSlicerDICOMTagEditorWidget);

  //TODO:
}
