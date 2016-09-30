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

  /// Reset current region name and container object
  void resetCurrentRegion();
  /// Reset current region modifier name and container object
  void resetCurrentRegionModifier();

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

  /// Name (AnatomicContextName) of the current anatomic context
  QString CurrentAnatomicContextName;
  /// Name (codeMeaning member) of the current region
  QString CurrentRegionName;
  /// Name (codeMeaning member) of the current region modifier
  QString CurrentRegionModifierName;

  /// Object containing the details of the current region
  vtkSlicerTerminologyType* CurrentRegionObject;
  /// Object containing the details of the current region modifier if any
  vtkSlicerTerminologyType* CurrentRegionModifierObject;
};

//-----------------------------------------------------------------------------
qSlicerTerminologyNavigatorWidgetPrivate::qSlicerTerminologyNavigatorWidgetPrivate(qSlicerTerminologyNavigatorWidget& object)
  : q_ptr(&object)
{
  this->CurrentCategoryObject = vtkSlicerTerminologyCategory::New();
  this->CurrentTypeObject = vtkSlicerTerminologyType::New();
  this->CurrentTypeModifierObject = vtkSlicerTerminologyType::New();

  this->CurrentRegionObject = vtkSlicerTerminologyType::New();
  this->CurrentRegionModifierObject = vtkSlicerTerminologyType::New();
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

  if (this->CurrentRegionObject)
    {
    this->CurrentRegionObject->Delete();
    this->CurrentRegionObject = NULL;
    }
  if (this->CurrentRegionModifierObject)
    {
    this->CurrentRegionModifierObject->Delete();
    this->CurrentRegionModifierObject = NULL;
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
  QObject::connect(this->ComboBox_TypeModifier, SIGNAL(currentIndexChanged(int)),
    q, SLOT(onTypeModifierSelectionChanged(int)) );
  QObject::connect(this->SearchBox_Category, SIGNAL(textChanged(QString)),
    q, SLOT(onCategorySearchTextChanged(QString)) );
  QObject::connect(this->SearchBox_Type, SIGNAL(textChanged(QString)),
    q, SLOT(onTypeSearchTextChanged(QString)) );

  QObject::connect(this->tableWidget_AnatomicRegion, SIGNAL(itemClicked(QTableWidgetItem*)),
    q, SLOT(onRegionClicked(QTableWidgetItem*)) );
  QObject::connect(this->ComboBox_AnatomicRegionModifier, SIGNAL(currentIndexChanged(int)),
    q, SLOT(onRegionModifierSelectionChanged(int)) );
  QObject::connect(this->SearchBox_AnatomicRegion, SIGNAL(textChanged(QString)),
    q, SLOT(onRegionSearchTextChanged(QString)) );

  // Set default settings for widgets
  this->tableWidget_Category->setEnabled(false);
  this->SearchBox_Category->setEnabled(false);
  this->tableWidget_Type->setEnabled(false);
  this->SearchBox_Type->setEnabled(false);
  this->ComboBox_TypeModifier->setEnabled(false);
  this->ColorPickerButton_RecommendedRGB->setEnabled(false);

  this->SearchBox_AnatomicRegion->setEnabled(false);
  this->tableWidget_AnatomicRegion->setEnabled(false);
  this->ComboBox_AnatomicRegionModifier->setEnabled(false);

  // Populate terminology combobox with the loaded terminologies
  q->populateTerminologyComboBox();
  // Populate anatomic context combobox with the loaded anatomic contexts
  q->populateAnatomicContextComboBox();
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
  unsigned char r=127, g=127, b=127;
  if ( this->CurrentTypeName.isEmpty() ||
       (this->CurrentTypeObject->GetHasModifiers() && this->CurrentTypeModifierName.isEmpty()) )
    {
    this->ColorPickerButton_RecommendedRGB->setColor(QColor(r,g,b));
    this->ColorPickerButton_RecommendedRGB->setEnabled(false);
    return;
    }

  // Valid color is present, enable color picker
  this->ColorPickerButton_RecommendedRGB->setEnabled(true);

  // If the current type has no modifiers then set color form the type
  if (!this->CurrentTypeObject->GetHasModifiers())
    {
    this->CurrentTypeObject->GetRecommendedDisplayRGBValue(r,g,b);
    }
  else
    {
    this->CurrentTypeModifierObject->GetRecommendedDisplayRGBValue(r,g,b);
    }
  this->ColorPickerButton_RecommendedRGB->setColor(QColor(r,g,b));
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidgetPrivate::resetCurrentRegion()
{
  this->CurrentRegionName = QString();

  if (this->CurrentRegionObject)
    {
    this->CurrentRegionObject->Delete();
    this->CurrentRegionObject = NULL;
    }
  this->CurrentRegionObject = vtkSlicerTerminologyType::New();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidgetPrivate::resetCurrentRegionModifier()
{
  this->CurrentRegionModifierName = QString();

  if (this->CurrentRegionModifierObject)
    {
    this->CurrentRegionModifierObject->Delete();
    this->CurrentRegionModifierObject = NULL;
    }
  this->CurrentRegionModifierObject = vtkSlicerTerminologyType::New();
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

  //TODO:
}

//-----------------------------------------------------------------------------
bool qSlicerTerminologyNavigatorWidget::anatomicRegionSectionVisible() const
{
  Q_D(const qSlicerTerminologyNavigatorWidget);

  return d->CollapsibleGroupBox_AnatomicRegion->isVisible();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::setAnatomicRegionSectionVisible(bool visible)
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  d->CollapsibleGroupBox_AnatomicRegion->setVisible(visible);
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

  QTableWidgetItem* selectedItem = NULL;
  d->tableWidget_Category->setRowCount(categoryNamesArray->GetNumberOfValues());
  for (int index=0; index<categoryNamesArray->GetNumberOfValues(); ++index)
    {
    QString currentCategoryName( categoryNamesArray->GetValue(index).c_str() );
    QTableWidgetItem* currentCategoryItem = new QTableWidgetItem(currentCategoryName);
    d->tableWidget_Category->setItem(index, 0, currentCategoryItem);

    if (!currentCategoryName.compare(d->CurrentCategoryName))
      {
      selectedItem = currentCategoryItem;
      }
    }

  // Select category if selection was valid and item shows up in search
  if (selectedItem)
    {
    d->tableWidget_Category->setCurrentItem(selectedItem);
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

  QTableWidgetItem* selectedItem = NULL;
  d->tableWidget_Type->setRowCount(typeNamesArray->GetNumberOfValues());
  for (int index=0; index<typeNamesArray->GetNumberOfValues(); ++index)
    {
    QString currentTypeName( typeNamesArray->GetValue(index).c_str() );
    QTableWidgetItem* currentTypeItem = new QTableWidgetItem(currentTypeName);
    d->tableWidget_Type->setItem(index, 0, currentTypeItem);

    if (!currentTypeName.compare(d->CurrentTypeName))
      {
      selectedItem = currentTypeItem;
      }
    }

  // Select type if selection was valid and item shows up in search
  if (selectedItem)
    {
    d->tableWidget_Type->setCurrentItem(selectedItem);
    }
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::populateTypeModifierComboBox()
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  d->ComboBox_TypeModifier->clear();

  if (d->CurrentTerminologyName.isEmpty() || d->CurrentCategoryName.isEmpty() || d->CurrentTypeName.isEmpty())
    {
    d->ComboBox_TypeModifier->setEnabled(false);
    return;
    }
  // If current type has no modifiers then leave it empty and disable
  if (!d->CurrentTypeObject->GetHasModifiers())
    {
    d->ComboBox_TypeModifier->setEnabled(false);
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
    d->ComboBox_TypeModifier->addItem(currentTypeModifierName);
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
  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
  this->populateCategoryTable();
  QApplication::restoreOverrideCursor();

  // Only enable category table if there are items in it
  if (d->tableWidget_Category->rowCount() == 0)
    {
    d->tableWidget_Category->setEnabled(false);
    d->SearchBox_Category->setEnabled(false);
    d->tableWidget_Type->setEnabled(false);
    d->SearchBox_Type->setEnabled(false);
    d->ComboBox_TypeModifier->setEnabled(false);
    }
  else
    {
    d->tableWidget_Category->setEnabled(true);
    d->SearchBox_Category->setEnabled(true);
    }
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onCategoryClicked(QTableWidgetItem* item)
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  // Reset current type and type modifier
  d->resetCurrentType();
  d->resetCurrentTypeModifier();
  this->populateTypeModifierComboBox();
  // Reset anatomic region information as well
  d->resetCurrentRegion();
  d->resetCurrentRegionModifier();
  d->tableWidget_AnatomicRegion->setCurrentItem(NULL);
  this->populateRegionModifierComboBox();

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

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
    d->SearchBox_Type->setEnabled(false);
    d->ComboBox_TypeModifier->setEnabled(false);
    }
  else
    {
    d->tableWidget_Type->setEnabled(true);
    d->SearchBox_Type->setEnabled(true);
    }

  // Enable anatomic region controls if related flag is on
  d->ComboBox_AnatomicContext->setEnabled(d->CurrentCategoryObject->GetShowAnatomy());
  d->tableWidget_AnatomicRegion->setEnabled(d->CurrentCategoryObject->GetShowAnatomy());
  d->SearchBox_AnatomicRegion->setEnabled(d->CurrentCategoryObject->GetShowAnatomy());
  d->ComboBox_AnatomicRegionModifier->setEnabled(d->CurrentCategoryObject->GetShowAnatomy());

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onTypeClicked(QTableWidgetItem* item)
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  // Reset current type modifier
  d->resetCurrentTypeModifier();

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

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
  d->ComboBox_TypeModifier->setEnabled(d->ComboBox_TypeModifier->count());

  // Set recommended color to color picker (will only set it if does not contain modifiers)
  d->setRecommendedColorFromCurrentSelection();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onTypeModifierSelectionChanged(int index)
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Set current type modifier
  d->CurrentTypeModifierName = d->ComboBox_TypeModifier->currentText();
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

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onCategorySearchTextChanged(QString search)
{
  Q_UNUSED(search);

  this->populateCategoryTable();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onTypeSearchTextChanged(QString search)
{
  Q_UNUSED(search);

  this->populateTypeTable();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::populateAnatomicContextComboBox()
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  d->ComboBox_AnatomicContext->clear();

  vtkSlicerTerminologiesModuleLogic* logic = d->terminologyLogic();
  if (!logic)
    {
    return;
    }

  std::vector<std::string> anatomicRegionContextNames;
  logic->GetLoadedAnatomicContextNames(anatomicRegionContextNames);
  for (std::vector<std::string>::iterator anIt=anatomicRegionContextNames.begin(); anIt!=anatomicRegionContextNames.end(); ++anIt)
    {
    d->ComboBox_AnatomicContext->addItem(anIt->c_str());
    }

  // Hide anatomic context combobox if there is only one option
  if (d->ComboBox_AnatomicContext->count() == 1)
    {
    this->onAnatomicContextSelectionChanged(0);
    d->ComboBox_AnatomicContext->setVisible(false);
    }
  else if (d->ComboBox_AnatomicContext->count() > 1)
    {
    d->ComboBox_AnatomicContext->setVisible(true);
    }
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::populateRegionTable()
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  d->tableWidget_AnatomicRegion->clearContents();

  if (d->CurrentAnatomicContextName.isEmpty())
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
  vtkSmartPointer<vtkStringArray> regionNamesArray = vtkSmartPointer<vtkStringArray>::New();
  logic->FindRegionNamesInAnatomicContext(
    d->CurrentAnatomicContextName.toLatin1().constData(), regionNamesArray,
    d->SearchBox_Type->text().toLatin1().constData() );

  QTableWidgetItem* selectedItem = NULL;
  d->tableWidget_AnatomicRegion->setRowCount(regionNamesArray->GetNumberOfValues());
  for (int index=0; index<regionNamesArray->GetNumberOfValues(); ++index)
    {
    QString currentRegionName( regionNamesArray->GetValue(index).c_str() );
    QTableWidgetItem* currentRegionItem = new QTableWidgetItem(currentRegionName);
    d->tableWidget_AnatomicRegion->setItem(index, 0, currentRegionItem);

    if (!currentRegionName.compare(d->CurrentRegionName))
      {
      selectedItem = currentRegionItem;
      }
    }

  // Select type if selection was valid and item shows up in search
  if (selectedItem)
    {
    d->tableWidget_AnatomicRegion->setCurrentItem(selectedItem);
    }
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::populateRegionModifierComboBox()
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  d->ComboBox_AnatomicRegionModifier->clear();

  if (d->CurrentAnatomicContextName.isEmpty() || d->CurrentRegionName.isEmpty())
    {
    d->ComboBox_AnatomicRegionModifier->setEnabled(false);
    return;
    }
  // If current region has no modifiers then leave it empty and disable
  if (!d->CurrentRegionObject->GetHasModifiers())
    {
    d->ComboBox_AnatomicRegionModifier->setEnabled(false);
    return;
    }

  vtkSlicerTerminologiesModuleLogic* logic = d->terminologyLogic();
  if (!logic)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access terminology logic";
    return;
    }

  // Get region modifier names
  vtkSmartPointer<vtkStringArray> regionModifierNamesArray = vtkSmartPointer<vtkStringArray>::New();
  logic->GetRegionModifierNamesInAnatomicRegion(
    d->CurrentAnatomicContextName.toLatin1().constData(), d->CurrentRegionName.toLatin1().constData(), regionModifierNamesArray );

  for (int index=0; index<regionModifierNamesArray->GetNumberOfValues(); ++index)
    {
    QString currentRegionModifierName( regionModifierNamesArray->GetValue(index).c_str() );
    d->ComboBox_AnatomicRegionModifier->addItem(currentRegionModifierName);
    }
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onAnatomicContextSelectionChanged(int index)
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  // Reset current region and region modifier
  d->resetCurrentRegion();
  d->resetCurrentRegionModifier();

  // Set current anatomic context
  d->CurrentAnatomicContextName = d->ComboBox_AnatomicContext->itemText(index);

  // Populate region table
  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
  this->populateRegionTable();
  QApplication::restoreOverrideCursor();

  // Only enable region table if there are items in it
  if (d->tableWidget_AnatomicRegion->rowCount() == 0)
    {
    d->tableWidget_AnatomicRegion->setEnabled(false);
    d->SearchBox_AnatomicRegion->setEnabled(false);
    d->ComboBox_AnatomicRegionModifier->setEnabled(false);
    }
  else if (d->CurrentCategoryObject->GetShowAnatomy())
    {
    d->tableWidget_AnatomicRegion->setEnabled(true);
    d->SearchBox_AnatomicRegion->setEnabled(true);
    }
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onRegionClicked(QTableWidgetItem* item)
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  // Reset current region modifier
  d->resetCurrentRegionModifier();

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Set current region
  d->CurrentRegionName = item->text();
  // Get current region container object
  vtkSlicerTerminologiesModuleLogic* logic = d->terminologyLogic();
  if (!logic)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access terminology logic";
    return;
    }
  logic->GetRegionInAnatomicContext(
    d->CurrentAnatomicContextName.toLatin1().constData(), d->CurrentRegionName.toLatin1().constData(), d->CurrentRegionObject );

  // Populate region modifier combobox
  this->populateRegionModifierComboBox();

  // Only enable region modifier combobox if there are items in it
  d->ComboBox_AnatomicRegionModifier->setEnabled(d->ComboBox_AnatomicRegionModifier->count());

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onRegionModifierSelectionChanged(int index)
{
  Q_D(qSlicerTerminologyNavigatorWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Set current region modifier
  d->CurrentRegionModifierName= d->ComboBox_AnatomicRegionModifier->currentText();
  // Get current region modifier container object
  vtkSlicerTerminologiesModuleLogic* logic = d->terminologyLogic();
  if (!logic)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to access terminology logic";
    return;
    }
  logic->GetRegionModifierInAnatomicRegion(
    d->CurrentAnatomicContextName.toLatin1().constData(), d->CurrentRegionName.toLatin1().constData(),
    d->CurrentRegionModifierName.toLatin1().constData(), d->CurrentRegionModifierObject );

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologyNavigatorWidget::onRegionSearchTextChanged(QString search)
{
  Q_UNUSED(search);

  this->populateRegionTable();
}
