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

// Terminologies includes
#include "qSlicerTerminologyNavigatorWidget.h"

#include "ui_qSlicerTerminologyNavigatorWidget.h"

#include "vtkSlicerTerminologiesModuleLogic.h"
#include "vtkSlicerTerminologyCategory.h"
#include "vtkSlicerTerminologyType.h"

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include <qSlicerAbstractCoreModule.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>

// Qt includes
#include <QDebug>
#include <QTableWidgetItem>

//-----------------------------------------------------------------------------
class qSlicerTerminologyNavigatorWidgetPrivate: public Ui_qSlicerTerminologyNavigatorWidget
{
  Q_DECLARE_PUBLIC(qSlicerTerminologyNavigatorWidget);

protected:
  qSlicerTerminologyNavigatorWidget* const q_ptr;
public:
  qSlicerTerminologyNavigatorWidgetPrivate(qSlicerTerminologyNavigatorWidget& object);
  ~qSlicerTerminologyNavigatorWidgetPrivate();
  void init();

  /// Get terminology module logic
  vtkSlicerTerminologiesModuleLogic* terminologyLogic();

  /// Reset current category name and container object
  void resetCurrentCategory();
  /// Reset current type name and container object
  void resetCurrentType();
  /// Reset current type modifier name and container object
  void resetCurrentTypeModifier();

  // Set recommended color from current selection to color picker
  // Note: will only set it if does not contain modifiers, because in that case it does not include
  //   recommended RGB color member. If modifier is selected then the color will be set from that.
  void setRecommendedColorFromCurrentSelection();

public:
  /// Name (SegmentationCategoryTypeContextName) of the current terminology
  QString CurrentTerminologyName;
  /// Name (codeMeaning member) of the current category
  QString CurrentCategoryName;
  /// Name (codeMeaning member) of the current type
  QString CurrentTypeName;
  /// Name (codeMeaning member) of the current type modifier
  QString CurrentTypeModifierName;

  /// Object containing the details of the current category
  vtkSlicerTerminologyCategory* CurrentCategoryObject;
  /// Object containing the details of the current type
  vtkSlicerTerminologyType* CurrentTypeObject;
  /// Object containing the details of the current type modifier if any
  vtkSlicerTerminologyType* CurrentTypeModifierObject;
};

//-----------------------------------------------------------------------------
qSlicerTerminologyNavigatorWidgetPrivate::qSlicerTerminologyNavigatorWidgetPrivate(qSlicerTerminologyNavigatorWidget& object)
  : q_ptr(&object)
{
  this->CurrentCategoryObject = vtkSlicerTerminologyCategory::New();
  this->CurrentTypeObject = vtkSlicerTerminologyType::New();
  this->CurrentTypeModifierObject = vtkSlicerTerminologyType::New();
}

//-----------------------------------------------------------------------------
qSlicerTerminologyNavigatorWidgetPrivate::~qSlicerTerminologyNavigatorWidgetPrivate()
{
  if (this->CurrentCategoryObject)
    {
    this->CurrentCategoryObject->Delete();
    this->CurrentCategoryObject = NULL;
    }
  if (this->CurrentTypeObject)
    {
    this->CurrentTypeObject->Delete();
    this->CurrentTypeObject = NULL;
    }
  if (this->CurrentTypeModifierObject)
    {
    this->CurrentTypeModifierObject->Delete();
    this->CurrentTypeModifierObject = NULL;
    }
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidgetPrivate::init()
{
  Q_Q(qSlicerTerminologyNavigatorWidget);
  this->setupUi(q);

  // Make connections
  QObject::connect(this->ComboBox_Terminology, SIGNAL(currentIndexChanged(int)),
    q, SLOT(onTerminologySelectionChanged(int)) );
  QObject::connect(this->tableWidget_Category, SIGNAL(itemClicked(QTableWidgetItem*)),
    q, SLOT(onCategoryClicked(QTableWidgetItem*)) );
  QObject::connect(this->tableWidget_Type, SIGNAL(itemClicked(QTableWidgetItem*)),
    q, SLOT(onTypeClicked(QTableWidgetItem*)) );
  QObject::connect(this->ComboBox_Modifier, SIGNAL(currentIndexChanged(int)),
    q, SLOT(onTypeModifierSelectionChanged(int)) );
  QObject::connect(this->SearchBox_Category, SIGNAL(textChanged(QString)),
    q, SLOT(onCategorySearchTextChanged(QString)) );
  QObject::connect(this->SearchBox_Type, SIGNAL(textChanged(QString)),
    q, SLOT(onTypeSearchTextChanged(QString)) );

  // Set default settings for widgets
  this->tableWidget_Category->setEnabled(false);
  this->tableWidget_Type->setEnabled(false);
  this->ComboBox_Modifier->setEnabled(false);
  this->ColorPickerButton_RecommendedRGB->setEnabled(false);
  q->setDicomPropertiesVisible(false);

  // Hide anatomic region context combobox until it is given purpose
  this->ComboBox_AnatomicRegionContext->setVisible(false); //TODO:
  // Hide optional properties section until it is given purpose
  q->setOptionalPropertiesVisible(false); //TODO:

  // Populate terminologies combobox with the loaded terminologies
  q->populateTerminologyComboBox();
}

//-----------------------------------------------------------------------------
vtkSlicerTerminologiesModuleLogic* qSlicerTerminologyNavigatorWidgetPrivate::terminologyLogic()
{
  qSlicerAbstractCoreModule* terminologiesModule =
    qSlicerCoreApplication::application()->moduleManager()->module("Terminologies");
  if (terminologiesModule)
  {
    vtkSlicerTerminologiesModuleLogic* terminologyLogic =
      vtkSlicerTerminologiesModuleLogic::SafeDownCast(terminologiesModule->logic());
    return terminologyLogic;
  }

  qCritical() << Q_FUNC_INFO << ": Terminologies module is not found";
  return NULL;
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidgetPrivate::resetCurrentCategory()
{
  this->CurrentCategoryName = QString();

  if (this->CurrentCategoryObject)
    {
    this->CurrentCategoryObject->Delete();
    this->CurrentCategoryObject = NULL;
    }
  this->CurrentCategoryObject = vtkSlicerTerminologyCategory::New();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidgetPrivate::resetCurrentType()
{
  this->CurrentTypeName = QString();

  if (this->CurrentTypeObject)
    {
    this->CurrentTypeObject->Delete();
    this->CurrentTypeObject = NULL;
    }
  this->CurrentTypeObject = vtkSlicerTerminologyType::New();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidgetPrivate::resetCurrentTypeModifier()
{
  this->CurrentTypeModifierName = QString();

  if (this->CurrentTypeModifierObject)
    {
    this->CurrentTypeModifierObject->Delete();
    this->CurrentTypeModifierObject = NULL;
    }
  this->CurrentTypeModifierObject = vtkSlicerTerminologyType::New();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidgetPrivate::setRecommendedColorFromCurrentSelection()
{
  // Set 'invalid' gray color if type is not selected,
  // or the selected type has modifiers but no modifier is selected
  if ( this->CurrentTypeName.isEmpty() ||
       (this->CurrentTypeObject->GetHasModifiers() && this->CurrentTypeModifierName.isEmpty()) )
    {
    this->ColorPickerButton_RecommendedRGB->setColor(QColor(127,127,127));
    this->ColorPickerButton_RecommendedRGB->setEnabled(false);
    return;
    }

  // Valid color is present, enable color picker
  this->ColorPickerButton_RecommendedRGB->setEnabled(true);

  // If the current type has no modifiers then set color form the type
  unsigned char r, g, b;
  if (!this->CurrentTypeObject->GetHasModifiers())
    {
    this->CurrentTypeObject->GetRecommendedDisplayRGBValue(r, g, b);
    }
  else
    {
    this->CurrentTypeModifierObject->GetRecommendedDisplayRGBValue(r, g, b);
    }
  this->ColorPickerButton_RecommendedRGB->setColor(QColor(r,g,b));
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// qSlicerTerminologyNavigatorWidget methods

//-----------------------------------------------------------------------------
qSlicerTerminologyNavigatorWidget::qSlicerTerminologyNavigatorWidget(QWidget* _parent)
  : qMRMLWidget(_parent)
  , d_ptr(new qSlicerTerminologyNavigatorWidgetPrivate(*this))
{
  Q_D(qSlicerTerminologyNavigatorWidget);
  d->init();
  this->updateWidgetFromCurrentTerminology();
}

//-----------------------------------------------------------------------------
qSlicerTerminologyNavigatorWidget::~qSlicerTerminologyNavigatorWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::updateWidgetFromCurrentTerminology()
{
  Q_D(qSlicerTerminologyNavigatorWidget);

}

//-----------------------------------------------------------------------------
bool qSlicerTerminologyNavigatorWidget::optionalPropertiesVisible() const
{
  Q_D(const qSlicerTerminologyNavigatorWidget);

  return d->CollapsibleGroupBox_Optional->isVisible();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::setOptionalPropertiesVisible(bool visible)
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  d->CollapsibleGroupBox_Optional->setVisible(visible);
}

//-----------------------------------------------------------------------------
bool qSlicerTerminologyNavigatorWidget::dicomPropertiesVisible() const
{
  Q_D(const qSlicerTerminologyNavigatorWidget);

  return d->CollapsibleGroupBox_DICOM->isVisible();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::setDicomPropertiesVisible(bool visible)
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  d->CollapsibleGroupBox_DICOM->setVisible(visible);
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::populateTerminologyComboBox()
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  d->ComboBox_Terminology->clear();

  vtkSlicerTerminologiesModuleLogic* logic = d->terminologyLogic();
  if (!logic)
    {
    return;
    }

  std::vector<std::string> terminologyNames;
  logic->GetLoadedTerminologyNames(terminologyNames);
  for (std::vector<std::string>::iterator termIt=terminologyNames.begin(); termIt!=terminologyNames.end(); ++termIt)
    {
    d->ComboBox_Terminology->addItem(termIt->c_str());
    }
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::populateCategoryTable()
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  d->tableWidget_Category->clearContents();

  if (d->CurrentTerminologyName.isEmpty())
    {
    return;
    }

  vtkSlicerTerminologiesModuleLogic* logic = d->terminologyLogic();
  if (!logic)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access terminology logic";
    return;
    }

  // Get category names containing the search string. If no search string then add every category
  vtkSmartPointer<vtkStringArray> categoryNamesArray = vtkSmartPointer<vtkStringArray>::New();
  logic->FindCategoryNamesInTerminology(
    d->CurrentTerminologyName.toLatin1().constData(), categoryNamesArray, d->SearchBox_Category->text().toLatin1().constData() );

  d->tableWidget_Category->setRowCount(categoryNamesArray->GetNumberOfValues());
  for (int index=0; index<categoryNamesArray->GetNumberOfValues(); ++index)
    {
    QString currentCategoryName( categoryNamesArray->GetValue(index).c_str() );
    QTableWidgetItem* currentCategoryItem = new QTableWidgetItem(currentCategoryName);
    d->tableWidget_Category->setItem(index, 0, currentCategoryItem);
    }
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::populateTypeTable()
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  d->tableWidget_Type->clearContents();

  if (d->CurrentTerminologyName.isEmpty() || d->CurrentCategoryName.isEmpty())
    {
    return;
    }

  vtkSlicerTerminologiesModuleLogic* logic = d->terminologyLogic();
  if (!logic)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access terminology logic";
    return;
    }

  // Get type names containing the search string. If no search string then add every type
  vtkSmartPointer<vtkStringArray> typeNamesArray = vtkSmartPointer<vtkStringArray>::New();
  logic->FindTypeNamesInTerminologyCategory(
    d->CurrentTerminologyName.toLatin1().constData(), d->CurrentCategoryName.toLatin1().constData(),
    typeNamesArray, d->SearchBox_Type->text().toLatin1().constData() );

  d->tableWidget_Type->setRowCount(typeNamesArray->GetNumberOfValues());
  for (int index=0; index<typeNamesArray->GetNumberOfValues(); ++index)
    {
    QString currentTypeName( typeNamesArray->GetValue(index).c_str() );
    QTableWidgetItem* currentTypeItem = new QTableWidgetItem(currentTypeName);
    d->tableWidget_Type->setItem(index, 0, currentTypeItem);
    }
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::populateTypeModifierComboBox()
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  d->ComboBox_Modifier->clear();

  if (d->CurrentTerminologyName.isEmpty() || d->CurrentCategoryName.isEmpty() || d->CurrentTypeName.isEmpty())
    {
    d->ComboBox_Modifier->setEnabled(false);
    return;
    }
  // If current type has no modifiers then leave it empty and disable
  if (!d->CurrentTypeObject->GetHasModifiers())
    {
    d->ComboBox_Modifier->setEnabled(false);
    return;
    }

  vtkSlicerTerminologiesModuleLogic* logic = d->terminologyLogic();
  if (!logic)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access terminology logic";
    return;
    }

  // Get type modifier names
  vtkSmartPointer<vtkStringArray> typeModifierNamesArray = vtkSmartPointer<vtkStringArray>::New();
  logic->GetTypeModifierNamesInTerminologyType(
    d->CurrentTerminologyName.toLatin1().constData(), d->CurrentCategoryName.toLatin1().constData(),
    d->CurrentTypeName.toLatin1().constData(), typeModifierNamesArray );

  for (int index=0; index<typeModifierNamesArray->GetNumberOfValues(); ++index)
    {
    QString currentTypeModifierName( typeModifierNamesArray->GetValue(index).c_str() );
    d->ComboBox_Modifier->addItem(currentTypeModifierName);
    }
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onTerminologySelectionChanged(int index)
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  // Reset current category, type, and type modifier
  d->resetCurrentCategory();
  d->resetCurrentType();
  d->resetCurrentTypeModifier();

  // Set current terminology
  d->CurrentTerminologyName = d->ComboBox_Terminology->itemText(index);

  // Populate category table
  this->populateCategoryTable();

  // Only enable category table if there are items in it
  if (d->tableWidget_Category->rowCount() == 0)
    {
    d->tableWidget_Category->setEnabled(false);
    d->tableWidget_Type->setEnabled(false);
    d->ComboBox_Modifier->setEnabled(false);
    }
  else
    {
    d->tableWidget_Category->setEnabled(true);
    }
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onCategoryClicked(QTableWidgetItem* item)
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  // Reset current type and type modifier
  d->resetCurrentType();
  d->resetCurrentTypeModifier();

  // Set current category
  d->CurrentCategoryName = item->text();
  // Get current category object
  vtkSlicerTerminologiesModuleLogic* logic = d->terminologyLogic();
  if (!logic)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access terminology logic";
    return;
    }
  logic->GetCategoryInTerminology(
    d->CurrentTerminologyName.toLatin1().constData(), d->CurrentCategoryName.toLatin1().constData(),
    d->CurrentCategoryObject );

  // Populate type table
  this->populateTypeTable();

  // Only enable type table if there are items in it
  if (d->tableWidget_Type->rowCount() == 0)
    {
    d->tableWidget_Type->setEnabled(false);
    d->ComboBox_Modifier->setEnabled(false);
    }
  else
    {
    d->tableWidget_Type->setEnabled(true);
    }
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onTypeClicked(QTableWidgetItem* item)
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  // Reset current type modifier
  d->resetCurrentTypeModifier();

  // Set current type
  d->CurrentTypeName = item->text();
  // Get current type container object
  vtkSlicerTerminologiesModuleLogic* logic = d->terminologyLogic();
  if (!logic)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access terminology logic";
    return;
    }
  logic->GetTypeInTerminologyCategory(
    d->CurrentTerminologyName.toLatin1().constData(), d->CurrentCategoryName.toLatin1().constData(),
    d->CurrentTypeName.toLatin1().constData(), d->CurrentTypeObject );

  // Populate type modifier combobox
  this->populateTypeModifierComboBox();

  // Only enable type modifier combobox if there are items in it
  d->ComboBox_Modifier->setEnabled(d->ComboBox_Modifier->count());

  // Set recommended color to color picker (will only set it if does not contain modifiers)
  d->setRecommendedColorFromCurrentSelection();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onTypeModifierSelectionChanged(int index)
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  // Set current type modifier
  d->CurrentTypeModifierName = d->ComboBox_Modifier->currentText();
  // Get current type modifier container object
  vtkSlicerTerminologiesModuleLogic* logic = d->terminologyLogic();
  if (!logic)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access terminology logic";
    return;
    }
  logic->GetTypeModifierInTerminologyType(
    d->CurrentTerminologyName.toLatin1().constData(), d->CurrentCategoryName.toLatin1().constData(),
    d->CurrentTypeName.toLatin1().constData(), d->CurrentTypeModifierName.toLatin1().constData(),
    d->CurrentTypeModifierObject );

  // Set recommended color to color picker
  d->setRecommendedColorFromCurrentSelection();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onCategorySearchTextChanged(QString)
{
  this->populateCategoryTable();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onTypeSearchTextChanged(QString)
{
  this->populateTypeTable();
}
