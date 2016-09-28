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

#ifndef __qSlicerTerminologyNavigatorWidget_h
#define __qSlicerTerminologyNavigatorWidget_h

// MRMLWidgets includes
#include "qMRMLWidget.h"

#include "qSlicerTerminologiesModuleWidgetsExport.h"

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKObject.h>

class qSlicerTerminologyNavigatorWidgetPrivate;

class QTableWidgetItem;

/// \brief Qt widget for selecting a single segment from a segmentation.
///   If multiple segments are needed, then use \sa qMRMLSegmentsTableView instead in SimpleListMode
/// \ingroup SlicerRt_QtModules_Segmentations_Widgets
class Q_SLICER_MODULE_TERMINOLOGIES_WIDGETS_EXPORT qSlicerTerminologyNavigatorWidget : public qMRMLWidget
{
  Q_OBJECT
  Q_PROPERTY(bool optionalPropertiesVisible READ optionalPropertiesVisible WRITE setOptionalPropertiesVisible)
  Q_PROPERTY(bool dicomPropertiesVisible READ dicomPropertiesVisible WRITE setDicomPropertiesVisible)

public:
  /// Constructor
  explicit qSlicerTerminologyNavigatorWidget(QWidget* parent = 0);
  /// Destructor
  virtual ~qSlicerTerminologyNavigatorWidget();

  /// Get whether optional properties are visible
  bool optionalPropertiesVisible() const;
  /// Get whether DICOM properties are visible
  bool dicomPropertiesVisible() const;

public slots:
  /// Show/hide optional properties section
  void setOptionalPropertiesVisible(bool);
  /// Show/hide DICOM properties section
  void setDicomPropertiesVisible(bool);

protected:
  /// Populate terminology combobox from terminology logic
  void populateTerminologyComboBox();
  /// Populate category table based on selected terminology and category search term
  void populateCategoryTable();
  /// Populate type table based on selected category and type search term
  void populateTypeTable();
  /// Populate type modifier combobox from terminology logic
  void populateTypeModifierComboBox();

  /// Update widget from current terminology and selections
  void updateWidgetFromCurrentTerminology();

protected slots:
  void onTerminologySelectionChanged(int);
  void onCategoryClicked(QTableWidgetItem*);
  void onTypeClicked(QTableWidgetItem*);
  void onTypeModifierSelectionChanged(int);
  void onCategorySearchTextChanged(QString);
  void onTypeSearchTextChanged(QString);

protected:
  QScopedPointer<qSlicerTerminologyNavigatorWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerTerminologyNavigatorWidget);
  Q_DISABLE_COPY(qSlicerTerminologyNavigatorWidget);
};

#endif
