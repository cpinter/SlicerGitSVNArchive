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
#include <QScrollArea>
#include <QHeaderView>

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

  QWidget* ScrollWidget;
  QVBoxLayout* TablesLayout;
  QTableWidget* PatientTable;
  QTableWidget* StudyTable;
  QList<QTableWidget*> SeriesTables;
};

//------------------------------------------------------------------------------
qSlicerDICOMTagEditorWidgetPrivate::qSlicerDICOMTagEditorWidgetPrivate(qSlicerDICOMTagEditorWidget& object)
  : q_ptr(&object)
{
  this->Scene = NULL;
  this->ScrollWidget = NULL;
  this->TablesLayout = NULL;
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

  // Create scroll area
  QScrollArea* scrollArea = new QScrollArea(qSlicerDICOMTagEditorWidget);
  scrollArea->setWidgetResizable(true);
  scrollArea->setFrameShape(QFrame::NoFrame);
  QVBoxLayout* mainLayout = new QVBoxLayout(qSlicerDICOMTagEditorWidget);
  mainLayout->setSpacing(0);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addWidget(scrollArea);
  this->ScrollWidget = new QWidget(qSlicerDICOMTagEditorWidget);

  // Create layout for the tables
  this->TablesLayout = new QVBoxLayout(this->ScrollWidget);
  this->TablesLayout->setSpacing(0);
  this->TablesLayout->setContentsMargins(0, 0, 0, 0);

  // Create patient and study tables as they will be needed in any case
  this->PatientTable = new QTableWidget(this->ScrollWidget);
  this->PatientTable->setColumnCount(2);
  this->PatientTable->horizontalHeader()->setVisible(true);
  this->PatientTable->verticalHeader()->setVisible(false);
  this->PatientTable->horizontalHeader()->setResizeMode(0, QHeaderView::Fixed);
  this->PatientTable->horizontalHeader()->setStretchLastSection(true);
  this->PatientTable->setColumnWidth(0, 200);
  this->PatientTable->setSelectionMode(QAbstractItemView::NoSelection);
  this->PatientTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // One scrollbar for all the tables
  this->PatientTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  QStringList patientHeaderLabels;
  patientHeaderLabels << "Patient tag" << "Value";
  this->PatientTable->setHorizontalHeaderLabels(patientHeaderLabels);
  this->TablesLayout->addWidget(this->PatientTable);

  this->StudyTable = new QTableWidget(this->ScrollWidget);
  this->StudyTable->setColumnCount(2);
  this->StudyTable->horizontalHeader()->setVisible(true);
  this->StudyTable->verticalHeader()->setVisible(false);
  this->StudyTable->horizontalHeader()->setResizeMode(0, QHeaderView::Fixed);
  this->StudyTable->horizontalHeader()->setStretchLastSection(true);
  this->StudyTable->setColumnWidth(0, 200);
  this->StudyTable->setSelectionMode(QAbstractItemView::NoSelection);
  this->StudyTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // One scrollbar for all the tables
  this->StudyTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  QStringList studyHeaderLabels;
  studyHeaderLabels << "Study tag" << "Value";
  this->StudyTable->setHorizontalHeaderLabels(studyHeaderLabels);
  this->TablesLayout->addWidget(this->StudyTable);

  scrollArea->setWidget(this->ScrollWidget);
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
    d->TablesLayout->removeWidget(table);
    //table->deleteLater(); //TODO: needed?
  }
  d->SeriesTables.clear();

  // Check if the exportables are in the same study
  vtkMRMLSubjectHierarchyNode* studyNode = NULL;
  foreach (qSlicerDICOMExportable* exportable, exportables)
  {
    vtkMRMLSubjectHierarchyNode* node = vtkMRMLSubjectHierarchyNode::SafeDownCast(
      d->Scene->GetNodeByID(exportable->nodeID().toLatin1().data()) );
    if ( !node || !node->IsLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries()) )
    {
      qCritical() << "qSlicerDICOMTagEditorWidget::setExportables: Exportable '" << exportable->name() << "' points to invalid node '"
        << (node ? QString("%1 (level %2)").arg(node->GetNameWithoutPostfix().c_str()).arg(node->GetLevel()) : "NULL") << "'";
      continue;
    }

    vtkMRMLSubjectHierarchyNode* parentNode = vtkMRMLSubjectHierarchyNode::SafeDownCast( node->GetParentNode() );
    if (!studyNode)
    {
      studyNode = parentNode;
    }
    else if (studyNode != parentNode)
    {
      QString error("Exportables are not in the same study!");
      qCritical() << "qSlicerDICOMTagEditorWidget::setExportables: " << error;
      return error;
    }
  }

  // Populate patient section
  vtkMRMLSubjectHierarchyNode* patientNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
    studyNode->GetParentNode() );
  if (!patientNode)
  {
    QString error("No patient node found!");
    qCritical() << "qSlicerDICOMTagEditorWidget::setExportables: " << error;
    return error;
  }
  std::vector<std::string> patientAttributes = patientNode->GetAttributeNames();
  for (std::vector<std::string>::iterator it = patientAttributes.begin();
    it != patientAttributes.end(); ++it)
  {
    std::string attribute = (*it);
    std::string prefix = attribute.substr(0, vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix().size());
    // If DICOM tag attribute
    if (!prefix.compare(vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix()))
    {
      int rowCount = d->PatientTable->rowCount();
      d->PatientTable->setRowCount(rowCount+1);
      QString tagName(attribute.substr(vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix().size()).c_str());
      d->PatientTable->setItem(rowCount, 0, new QTableWidgetItem(tagName));
      QString tagValue(patientNode->GetAttribute(attribute.c_str()));
      d->PatientTable->setItem(rowCount, 1, new QTableWidgetItem(tagValue));
    }
  }
  d->PatientTable->setFixedHeight(d->PatientTable->rowCount() * 30 + 26);

  // Populate study section
  std::vector<std::string> studyAttributes = studyNode->GetAttributeNames();
  for (std::vector<std::string>::iterator it = studyAttributes.begin();
    it != studyAttributes.end(); ++it)
  {
    std::string attribute = (*it);
    std::string prefix = attribute.substr(0, vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix().size());
    // If DICOM tag attribute
    if (!prefix.compare(vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix()))
    {
      int rowCount = d->StudyTable->rowCount();
      d->StudyTable->setRowCount(rowCount+1);
      QString tagName(attribute.substr(vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix().size()).c_str());
      d->StudyTable->setItem(rowCount, 0, new QTableWidgetItem(tagName));
      QString tagValue(studyNode->GetAttribute(attribute.c_str()));
      d->StudyTable->setItem(rowCount, 1, new QTableWidgetItem(tagValue));
    }
  }
  d->StudyTable->setFixedHeight(d->StudyTable->rowCount() * 30 + 26);

  // Create sections for each exportable
  foreach (qSlicerDICOMExportable* exportable, exportables)
  {
    // Get exportable series node
    vtkMRMLSubjectHierarchyNode* seriesNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
      d->Scene->GetNodeByID(exportable->nodeID().toLatin1().constData()) );

    // Create series table for exportable
    QTableWidget* seriesTable = new QTableWidget(d->ScrollWidget);
    seriesTable->setColumnCount(2);
    seriesTable->horizontalHeader()->setVisible(true);
    seriesTable->verticalHeader()->setVisible(false);
    seriesTable->horizontalHeader()->setResizeMode(0, QHeaderView::Fixed);
    seriesTable->horizontalHeader()->setStretchLastSection(true);
    seriesTable->setColumnWidth(0, 200);
    seriesTable->setSelectionMode(QAbstractItemView::NoSelection);
    seriesTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // One scrollbar for all the tables
    seriesTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->TablesLayout->addWidget(seriesTable);
    QStringList seriesHeaderLabels;
    seriesHeaderLabels << QString("%1 tag").arg(seriesNode->GetNameWithoutPostfix().c_str()) << "Value";
    seriesTable->setHorizontalHeaderLabels(seriesHeaderLabels);

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
