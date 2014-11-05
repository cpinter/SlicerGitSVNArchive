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
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

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

public:
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

  // Clear tag editor tables and exportables
  q->clear();

  // Make connections for setting edited values for common tags
  QObject::connect( this->PatientTable, SIGNAL(cellChanged(int,int)), q, SLOT(patientTableCellChanged(int,int)) );
  QObject::connect( this->StudyTable, SIGNAL(cellChanged(int,int)), q, SLOT(studyTableCellChanged(int,int)) );
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
  this->PatientTable->setColumnWidth(0, 250);
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
  this->StudyTable->setColumnWidth(0, 250);
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

  // Set exportable in class
  d->Exportables = exportables;
  // Clear argument exportables to prevent modifying the wrong list
  exportables.clear();

  // Check if the exportables are in the same study
  vtkMRMLSubjectHierarchyNode* studyNode = NULL;
  foreach (qSlicerDICOMExportable* exportable, d->Exportables)
    {
    vtkMRMLSubjectHierarchyNode* seriesNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
      d->Scene->GetNodeByID(exportable->nodeID().toLatin1().data()) );
    if ( !seriesNode || !seriesNode->IsLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries()) )
      {
      qCritical() << "qSlicerDICOMTagEditorWidget::setExportables: Exportable '" << exportable->name() << "' points to invalid series node '"
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

  // Get patient node
  vtkMRMLSubjectHierarchyNode* patientNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
    studyNode->GetParentNode() );
  if (!patientNode)
    {
    QString error("No patient node found!");
    qCritical() << "qSlicerDICOMTagEditorWidget::setExportables: " << error;
    return error;
    }
  // Add missing patient tags with empty values to patient node so that they are displayed in the table
  std::vector<std::string> patientNodeAttributeNames = patientNode->GetAttributeNames();
  std::vector<std::string> patientTagNames = vtkMRMLSubjectHierarchyConstants::GetDICOMPatientTagNames();
  for ( std::vector<std::string>::iterator patientTagIt = patientTagNames.begin();
    patientTagIt != patientTagNames.end(); ++patientTagIt )
    {
    std::string tagAttributeName = vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix() + (*patientTagIt);
    if (std::find(patientNodeAttributeNames.begin(), patientNodeAttributeNames.end(), tagAttributeName) == patientNodeAttributeNames.end())
      {
      patientNode->SetAttribute(tagAttributeName.c_str(), "");
      }
    }
  // Get attribute names again in case some were missing
  patientNodeAttributeNames = patientNode->GetAttributeNames();
  // Create a row in table widget for each tag and populate exportables with patient tags
  // (all tags are acquired from the exportable on export)
  for (std::vector<std::string>::iterator it = patientNodeAttributeNames.begin();
    it != patientNodeAttributeNames.end(); ++it)
    {
    std::string attributeName = (*it);
    std::string attributePrefix = attributeName.substr(0, vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix().size());
    QString tagName(attributeName.substr(vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix().size()).c_str());
    QString tagValue(patientNode->GetAttribute(attributeName.c_str()));
    // If DICOM tag attribute (i.e. has the prefix), then add to the table and exportable
    if (!attributePrefix.compare(vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix()))
      {
      // Add patient tag in a new row in the patient table
      int rowCount = d->PatientTable->rowCount();
      d->PatientTable->setRowCount(rowCount+1);
      d->PatientTable->setItem(rowCount, 0, new QTableWidgetItem(tagName));
      d->PatientTable->setItem(rowCount, 1, new QTableWidgetItem(tagValue));
      // Make sure tag name is not edited
      d->PatientTable->item(rowCount, 0)->setFlags(Qt::ItemIsEnabled);

      // Also add it to the exportables (needed there for export)
      foreach (qSlicerDICOMExportable* exportable, d->Exportables)
        {
        exportable->setTag(tagName, tagValue);
        }
      }
    }
  // Set fixed height of patient section (row height * number of rows + header height + padding for the frames)
  d->PatientTable->setFixedHeight(
    d->PatientTable->rowHeight(0) * d->PatientTable->rowCount()
    + d->PatientTable->horizontalHeader()->height() + 5);


  // Populate study section (we already have the study node, no need to get it here)

  // Add missing study tags with empty values to study node so that they are displayed in the table
  std::vector<std::string> studyNodeAttributeNames = studyNode->GetAttributeNames();
  std::vector<std::string> studyTagNames = vtkMRMLSubjectHierarchyConstants::GetDICOMStudyTagNames();
  for ( std::vector<std::string>::iterator studyTagIt = studyTagNames.begin();
    studyTagIt != studyTagNames.end(); ++studyTagIt )
    {
    std::string tagAttributeName = vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix() + (*studyTagIt);
    if (std::find(studyNodeAttributeNames.begin(), studyNodeAttributeNames.end(), tagAttributeName) == studyNodeAttributeNames.end())
      {
      studyNode->SetAttribute(tagAttributeName.c_str(), "");
      }
    }
  // Get attribute names again in case some were missing
  studyNodeAttributeNames = studyNode->GetAttributeNames();
  // Create a row in table widget for each tag and populate exportables with study tags
  // (all tags are acquired from the exportable on export)
  for (std::vector<std::string>::iterator it = studyNodeAttributeNames.begin();
    it != studyNodeAttributeNames.end(); ++it)
    {
    std::string attributeName = (*it);
    std::string attributePrefix = attributeName.substr(0, vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix().size());
    QString tagName(attributeName.substr(vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix().size()).c_str());
    QString tagValue(studyNode->GetAttribute(attributeName.c_str()));
    // If DICOM tag attribute (i.e. has the prefix), then add to the table and exportable
    if (!attributePrefix.compare(vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix()))
      {
      // Add study tag in a new row in the study table
      int rowCount = d->StudyTable->rowCount();
      d->StudyTable->setRowCount(rowCount+1);
      d->StudyTable->setItem(rowCount, 0, new QTableWidgetItem(tagName));
      d->StudyTable->setItem(rowCount, 1, new QTableWidgetItem(tagValue));
      // Make sure tag name is not edited
      d->StudyTable->item(rowCount, 0)->setFlags(Qt::ItemIsEnabled);

      // Also add it to the exportables (needed there for export)
      foreach (qSlicerDICOMExportable* exportable, d->Exportables)
        {
        exportable->setTag(tagName, tagValue);
        }
      }
    }
  // Set fixed height of study section (row height * number of rows + header height + padding for the frames)
  d->StudyTable->setFixedHeight(
    d->StudyTable->rowHeight(0) * d->StudyTable->rowCount()
    + d->StudyTable->horizontalHeader()->height() + 5);


  // Create sections for each exportable
  foreach (qSlicerDICOMExportable* exportable, d->Exportables)
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
    seriesTable->setColumnWidth(0, 250);
    seriesTable->setSelectionMode(QAbstractItemView::NoSelection);
    seriesTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // One scrollbar for all the tables
    seriesTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->TablesLayout->addWidget(seriesTable);
    QStringList seriesHeaderLabels;
    seriesHeaderLabels << QString("'%1' series tag").arg(seriesNode->GetNameWithoutPostfix().c_str()) << "Value";
    seriesTable->setHorizontalHeaderLabels(seriesHeaderLabels);
    // Associate exportable object to series table
    seriesTable->setProperty("Exportable", QVariant::fromValue<QObject*>(exportable));
    // Make connection to set edited tag for series
    QObject::connect( seriesTable, SIGNAL(cellChanged(int,int)), this, SLOT(seriesTableCellChanged(int,int)) );

    // Save series table to internal list
    d->SeriesTables.append(seriesTable);

    // Get series tags from exportable and populate table with them
    QMap<QString,QString> exportableTagsMap = exportable->tags();
    foreach (QString tagName, exportableTagsMap.keys())
      {
      // Only use series tags
      if ( vtkSlicerSubjectHierarchyModuleLogic::IsPatientTag(tagName.toLatin1().constData())
        || vtkSlicerSubjectHierarchyModuleLogic::IsStudyTag(tagName.toLatin1().constData()) )
        {
        continue;
        }

      // Add new row in series table for series tag
      int rowCount = seriesTable->rowCount();
      seriesTable->setRowCount(rowCount+1);
      seriesTable->setItem(rowCount, 0, new QTableWidgetItem(tagName));
      // Make sure tag name is not edited
      seriesTable->item(rowCount, 0)->setFlags(Qt::ItemIsEnabled);

      // If series node contains tag then use that value
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

    // Set fixed height of current series section (row height * number of rows + header height + padding for the frames)
    seriesTable->setFixedHeight(
      seriesTable->rowHeight(0) * seriesTable->rowCount()
      + seriesTable->horizontalHeader()->height() + 5);
    }

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

    // Get subject hierarchy series node from exportable
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

//------------------------------------------------------------------------------
void qSlicerDICOMTagEditorWidget::patientTableCellChanged(int row, int column)
{
  Q_D(qSlicerDICOMTagEditorWidget);
  if (column == 1)
    {
    // Set new tag value in each exportable (patient tags are common)
    foreach (qSlicerDICOMExportable* exportable, d->Exportables)
      {
      exportable->setTag(d->PatientTable->item(row,0)->text(), d->PatientTable->item(row,1)->text());
      }
    }
}

//------------------------------------------------------------------------------
void qSlicerDICOMTagEditorWidget::studyTableCellChanged(int row, int column)
{
  Q_D(qSlicerDICOMTagEditorWidget);
  if (column == 1)
    {
    // Set new tag value in each exportable (study tags are common)
    foreach (qSlicerDICOMExportable* exportable, d->Exportables)
      {
      exportable->setTag(d->StudyTable->item(row,0)->text(), d->StudyTable->item(row,1)->text());
      }
    }
}

//------------------------------------------------------------------------------
void qSlicerDICOMTagEditorWidget::seriesTableCellChanged(int row, int column)
{
  if (column == 1)
    {
    // Get edited series table
    QTableWidget* seriesTable = dynamic_cast<QTableWidget*>(sender());
    if (!seriesTable)
      {
      qCritical() << "qSlicerDICOMTagEditorWidgetPrivate::seriesTableCellChanged: Unable to get edited series table widget!";
      return;
      }

    // Get exportable for series table
    QVariant exportableVariant = seriesTable->property("Exportable");
    qSlicerDICOMExportable* exportable = qobject_cast<qSlicerDICOMExportable*>(
      exportableVariant.value<QObject*>() );
    if (!exportable)
      {
      qCritical() << "qSlicerDICOMTagEditorWidgetPrivate::seriesTableCellChanged: Failed to get exportable for series tags table!";
      return;
      }

    // Set tag in exportable
    exportable->setTag(seriesTable->item(row,0)->text(), seriesTable->item(row,1)->text());
    }
}
