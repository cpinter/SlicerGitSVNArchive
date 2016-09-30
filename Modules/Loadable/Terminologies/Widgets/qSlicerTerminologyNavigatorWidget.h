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

/// \brief Qt widget for browsing a terminology dictionary.
///   DICOM properties of the selected entry can also be set if enabled.
/// \ingroup SlicerRt_QtModules_Terminologies_Widgets
class Q_SLICER_MODULE_TERMINOLOGIES_WIDGETS_EXPORT qSlicerTerminologyNavigatorWidget : public qMRMLWidget
{
  Q_OBJECT
  Q_PROPERTY(bool anatomicRegionSectionVisible READ anatomicRegionSectionVisible WRITE setAnatomicRegionSectionVisible)

public:
  /// Constructor
  explicit qSlicerTerminologyNavigatorWidget(QWidget* parent = 0);
  /// Destructor
  virtual ~qSlicerTerminologyNavigatorWidget();

  /// Get whether anatomic region section are visible
  bool anatomicRegionSectionVisible() const;

public slots:
  /// Show/hide anatomic region section section
  void setAnatomicRegionSectionVisible(bool);

protected:
  /// Populate terminology combobox from terminology logic
  void populateTerminologyComboBox();
  /// Populate category table based on selected terminology and category search term
  void populateCategoryTable();
  /// Populate type table based on selected category and type search term
  void populateTypeTable();
  /// Populate type modifier combobox from terminology logic
  void populateTypeModifierComboBox();

  /// Populate anatomic region context combobox from terminology logic
  void populateAnatomicContextComboBox();
  /// Populate region table based on selected anatomic region context and type search term
  void populateRegionTable();
  /// Populate region modifier combobox from terminology logic
  void populateRegionModifierComboBox();

  /// Update widget from current terminology and selections
  void updateWidgetFromCurrentTerminology();

protected slots:
  void onTerminologySelectionChanged(int);
  void onCategoryClicked(QTableWidgetItem*);
  void onTypeClicked(QTableWidgetItem*);
  void onTypeModifierSelectionChanged(int);
  void onCategorySearchTextChanged(QString);
  void onTypeSearchTextChanged(QString);

  void onAnatomicContextSelectionChanged(int);
  void onRegionClicked(QTableWidgetItem*);
  void onRegionModifierSelectionChanged(int);
  void onRegionSearchTextChanged(QString);

protected:
  QScopedPointer<qSlicerTerminologyNavigatorWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerTerminologyNavigatorWidget);
  Q_DISABLE_COPY(qSlicerTerminologyNavigatorWidget);
};

#endif
