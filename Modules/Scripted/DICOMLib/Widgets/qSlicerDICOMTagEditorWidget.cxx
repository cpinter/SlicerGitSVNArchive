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

// STD includes
#include <algorithm>

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
  QList<QString> PatientTags;
  QList<QString> StudyTags;
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

  q->clear();

  // Static list of patient and study tags that need to be filled
  this->PatientTags.append(QString(vtkMRMLSubjectHierarchyConstants::GetDICOMPatientNameAttributeName().c_str()));
  this->PatientTags.append(QString(vtkMRMLSubjectHierarchyConstants::GetDICOMPatientIDAttributeName().c_str()));
  this->PatientTags.append(QString(vtkMRMLSubjectHierarchyConstants::GetDICOMPatientSexAttributeName().c_str()));
  this->PatientTags.append(QString(vtkMRMLSubjectHierarchyConstants::GetDICOMPatientBirthDateAttributeName().c_str()));
  this->PatientTags.append(QString(vtkMRMLSubjectHierarchyConstants::GetDICOMPatientCommentsAttributeName().c_str()));

  this->StudyTags.append(QString(vtkMRMLSubjectHierarchyConstants::GetDICOMStudyDescriptionAttributeName().c_str()));
  this->StudyTags.append(QString(vtkMRMLSubjectHierarchyConstants::GetDICOMStudyDateAttributeName().c_str()));
  this->StudyTags.append(QString(vtkMRMLSubjectHierarchyConstants::GetDICOMStudyTimeAttributeName().c_str()));
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
  d->setupUi(this);
  d->init();
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

  // Clear tables and inner state
  this->clear();

  // Check if the exportables are in the same study
  vtkMRMLSubjectHierarchyNode* studyNode = NULL;
  foreach (qSlicerDICOMExportable* exportable, exportables)
  {
    vtkMRMLSubjectHierarchyNode* seriesNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
      d->Scene->GetNodeByID(exportable->nodeID().toLatin1().data()) );
    if ( !seriesNode || !seriesNode->IsLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries()) )
    {
      qCritical() << "qSlicerDICOMTagEditorWidget::setExportables: Exportable '" << exportable->name() << "' points to invalid seriesNode '"
        << (seriesNode ? QString("%1 (level %2)").arg(seriesNode->GetNameWithoutPostfix().c_str()).arg(seriesNode->GetLevel()) : "NULL") << "'";
      continue;
    }

    vtkMRMLSubjectHierarchyNode* parentNode = vtkMRMLSubjectHierarchyNode::SafeDownCast( seriesNode->GetParentNode() );
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
    QString error("No patient seriesNode found!");
    qCritical() << "qSlicerDICOMTagEditorWidget::setExportables: " << error;
    return error;
  }
  std::vector<std::string> patientAttributes = patientNode->GetAttributeNames();
  // Add missing patient tags with empty values to seriesNode
  foreach (QString patientTag, d->PatientTags)
  {
    std::string tagAttributeName(patientTag.toLatin1().constData());
    if (std::find(patientAttributes.begin(), patientAttributes.end(), tagAttributeName) == patientAttributes.end())
    {
      patientNode->SetAttribute(tagAttributeName.c_str(), "");
    }
  }
  // Create a row in table widget for each tag
  patientAttributes = patientNode->GetAttributeNames();
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
  // Set fixed height of patient section
  d->PatientTable->setFixedHeight(d->PatientTable->rowCount() * 30 + 26);

  // Populate study section (we already have the study seriesNode, no need to get it here)
  // Add missing patient tags with empty values to seriesNode
  std::vector<std::string> studyAttributes = studyNode->GetAttributeNames();
  foreach (QString studyTag, d->StudyTags)
  {
    std::string tagAttributeName(studyTag.toLatin1().constData());
    if (std::find(studyAttributes.begin(), studyAttributes.end(), tagAttributeName) == studyAttributes.end())
    {
      studyNode->SetAttribute(tagAttributeName.c_str(), "");
    }
  }
  // Create a row in table widget for each tag
  studyAttributes = studyNode->GetAttributeNames();
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
  // Set fixed height of study section
  d->StudyTable->setFixedHeight(d->StudyTable->rowCount() * 30 + 26);

  // Create sections for each exportable
  foreach (qSlicerDICOMExportable* exportable, exportables)
  {
    // Get exportable series seriesNode
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
    seriesHeaderLabels << QString("'%1' series tag").arg(seriesNode->GetNameWithoutPostfix().c_str()) << "Value";
    seriesTable->setHorizontalHeaderLabels(seriesHeaderLabels);
    // Associate exportable object to series table
    seriesTable->setProperty("Exportable", QVariant::fromValue<QObject*>(exportable));

    d->SeriesTables.append(seriesTable);

    // Get tags from exportable and populate table with them
    QMap<QString,QString> exportableTagsMap = exportable->tags();
    foreach (QString tagName, exportableTagsMap.keys())
    {
      int rowCount = seriesTable->rowCount();
      seriesTable->setRowCount(rowCount+1);
      seriesTable->setItem(rowCount, 0, new QTableWidgetItem(tagName));

      // If series seriesNode contains tag then use that value
      const char* tagAttributeValue = seriesNode->GetAttribute(tagName.toLatin1().constData());
      if (tagAttributeValue)
      {
        seriesTable->setItem(rowCount, 1, new QTableWidgetItem(QString(tagAttributeValue)));
      }
      // Use default value from exportable otherwise
      else
      {
        seriesTable->setItem(rowCount, 1, new QTableWidgetItem(exportableTagsMap[tagName]));
      }
    }
  }

  // Set exportable into tag editor widget
  d->Exportables = exportables;

  // Return empty error message indicating success
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

  // Empty patient and study tables
  d->PatientTable->setRowCount(0);
  d->StudyTable->setRowCount(0);

  // Remove series tables
  foreach (QTableWidget* table, d->SeriesTables)
  {
    d->TablesLayout->removeWidget(table);
  }
  d->SeriesTables.clear();

  // Empty exportables list
  d->Exportables.clear();
}

//------------------------------------------------------------------------------
void qSlicerDICOMTagEditorWidget::commitChangesToNodes()
{
  Q_D(qSlicerDICOMTagEditorWidget);

  // Commit changes to exported series
  vtkMRMLSubjectHierarchyNode* studyNode = NULL;
  foreach (QTableWidget* seriesTable, d->SeriesTables)
  {
    // Get exportable for series table
    QVariant exportableVariant = seriesTable->property("Exportable");
    qSlicerDICOMExportable* exportable = qobject_cast<qSlicerDICOMExportable*>(
      exportableVariant.value<QObject*>() );
    if (!exportable)
    {
      qCritical() << "qSlicerDICOMTagEditorWidget::commitChangesToNodes: Failed to get exportable for series tags table!";
      continue;
    }

    // Get subject hierarchy seriesNode from exportable
    vtkMRMLSubjectHierarchyNode* seriesNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
      d->Scene->GetNodeByID(exportable->nodeID().toLatin1().constData()) );
    if (!studyNode)
    {
      studyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast( seriesNode->GetParentNode() );
    }

    // Write tags from table to subject hierarchy series node
    for (int row=0; row<seriesTable->rowCount(); ++row)
    {
      QString tagName = seriesTable->item(row, 0)->text();
      QString tagAttributeName = QString(vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix().c_str()) + tagName;
      QString tagValue = seriesTable->item(row, 1)->text();
      seriesNode->SetAttribute(tagAttributeName.toLatin1().constData(), tagValue.toLatin1().constData());
    }
  }

  // Commit changes to study
  if (!studyNode)
  {
    qCritical() << "qSlicerDICOMTagEditorWidget::commitChangesToNodes: Failed to get study node!";
    return;
  }
  // Write tags from study table to study node
  for (int row=0; row<d->StudyTable->rowCount(); ++row)
  {
    QString tagName = d->StudyTable->item(row, 0)->text();
    QString tagAttributeName = QString(vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix().c_str()) + tagName;
    QString tagValue = d->StudyTable->item(row, 1)->text();
    studyNode->SetAttribute(tagAttributeName.toLatin1().constData(), tagValue.toLatin1().constData());
  }

  // Commit changes to patient
  vtkMRMLSubjectHierarchyNode* patientNode = vtkMRMLSubjectHierarchyNode::SafeDownCast( studyNode->GetParentNode() );
  if (!patientNode)
  {
    qCritical() << "qSlicerDICOMTagEditorWidget::commitChangesToNodes: Failed to get patientNode node!";
    return;
  }
  // Write tags from patient table to patient node
  for (int row=0; row<d->PatientTable->rowCount(); ++row)
  {
    QString tagName = d->PatientTable->item(row, 0)->text();
    QString tagAttributeName = QString(vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix().c_str()) + tagName;
    QString tagValue = d->PatientTable->item(row, 1)->text();
    patientNode->SetAttribute(tagAttributeName.toLatin1().constData(), tagValue.toLatin1().constData());
  }
}
