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

// DICOMLib includes
#include "qSlicerDICOMTagEditorWidget.h"
#include "qSlicerDICOMExportable.h"

// Qt includes
#include <QDebug>
#include <QtGui/QVBoxLayout>
#include <QtGui/QTableWidget>

// SubjectHierarchy includes
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkMRMLSubjectHierarchyConstants.h"

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
  void setupUi(QWidget *qSlicerDICOMTagEditorWidget);

  QList<qSlicerDICOMExportable*> Exportables;
  vtkMRMLScene* Scene;

  QVBoxLayout* VerticalLayout;
  QTableWidget* PatientTable;
  QTableWidget* StudyTable;
  QList<QTableWidget*> SeriesTables;
};

//------------------------------------------------------------------------------
qSlicerDICOMTagEditorWidgetPrivate::qSlicerDICOMTagEditorWidgetPrivate(qSlicerDICOMTagEditorWidget& object)
  : q_ptr(&object)
{
  this->Scene = NULL;
  this->VerticalLayout = NULL;
  this->PatientTable = NULL;
  this->StudyTable = NULL;
}

//------------------------------------------------------------------------------
void qSlicerDICOMTagEditorWidgetPrivate::init()
{
  Q_Q(qSlicerDICOMTagEditorWidget);

  this->SeriesTables.clear();
}

//------------------------------------------------------------------------------
void qSlicerDICOMTagEditorWidgetPrivate::setupUi(QWidget *qSlicerDICOMTagEditorWidget)
{
  if (qSlicerDICOMTagEditorWidget->objectName().isEmpty())
  {
    qSlicerDICOMTagEditorWidget->setObjectName(QString::fromUtf8("qSlicerDICOMTagEditorWidget"));
  }

  // Create layout for the tables
  this->VerticalLayout = new QVBoxLayout(qSlicerDICOMTagEditorWidget);
  this->VerticalLayout->setSpacing(2);
  this->VerticalLayout->setContentsMargins(2, 2, 2, 2);

  // Create patient and study tables as they will be needed in any case
  this->PatientTable = new QTableWidget(qSlicerDICOMTagEditorWidget);
  this->VerticalLayout->addWidget(this->PatientTable);

  this->StudyTable = new QTableWidget(qSlicerDICOMTagEditorWidget);
  this->VerticalLayout->addWidget(this->StudyTable);
}

//------------------------------------------------------------------------------
qSlicerDICOMTagEditorWidget::qSlicerDICOMTagEditorWidget(QWidget *parent)
  : QWidget(parent)
  , d_ptr(new qSlicerDICOMTagEditorWidgetPrivate(*this))
{
  Q_D(qSlicerDICOMTagEditorWidget);
  d->init();
  d->setupUi(this);
}

//------------------------------------------------------------------------------
qSlicerDICOMTagEditorWidget::~qSlicerDICOMTagEditorWidget()
{
}

//------------------------------------------------------------------------------
void qSlicerDICOMTagEditorWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerDICOMTagEditorWidget);
  d->Scene = scene;
}

//------------------------------------------------------------------------------
QString qSlicerDICOMTagEditorWidget::setExportables(QList<qSlicerDICOMExportable*> exportables)
{
  Q_D(qSlicerDICOMTagEditorWidget);

  if (!d->Scene)
  {
    QString error("Invalid MRML scene!");
    qCritical() << "qSlicerDICOMTagEditorWidget::setExportables: " << error;
    return error;
  }

  // Clear tables
  foreach (QTableWidget* table, d->SeriesTables)
  {
    d->VerticalLayout->removeWidget(table);
    //table->deleteLater(); //TODO: needed?
  }
  d->SeriesTables.clear();

  // Check if the exportables are in the same study
  vtkMRMLSubjectHierarchyNode* parentStudyNode = NULL;
  foreach (qSlicerDICOMExportable* exportable, exportables)
  {
    vtkMRMLSubjectHierarchyNode* node = vtkMRMLSubjectHierarchyNode::SafeDownCast(
      d->Scene->GetNodeByID(exportable->nodeID().toLatin1().data()) );
    if ( !node || !node->IsLevel(vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES) )
    {
      qCritical() << "qSlicerDICOMTagEditorWidget::setExportables: Exportable '" << exportable->name() << "' points to invalid node '"
        << (node ? QString("%1 (level %2)").arg(node->GetNameWithoutPostfix().c_str()).arg(node->GetLevel()) : "NULL") << "'";
      continue;
    }

    vtkMRMLSubjectHierarchyNode* parentNode = vtkMRMLSubjectHierarchyNode::SafeDownCast( node->GetParentNode() );
    if (!parentStudyNode)
    {
      parentStudyNode = parentNode;
    }
    else if (parentStudyNode != parentNode)
    {
      QString error("Exportables are not in the same study!");
      qCritical() << "qSlicerDICOMTagEditorWidget::setExportables: " << error;
      return error;
    }
  }

  // Populate study section

  //TODO: copy tags from study node (and maybe merge it with a central list of study tags)

  // Populate patient section

  // Create sections for each exportable
  foreach (qSlicerDICOMExportable* exportable, exportables)
  {
    // Create series table for exportable
    QTableWidget* seriesTable = new QTableWidget(this);
    d->VerticalLayout->addWidget(seriesTable);
    d->SeriesTables.append(seriesTable);
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
