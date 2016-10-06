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

#ifndef __qSlicerTerminologySelectorDialog_h
#define __qSlicerTerminologySelectorDialog_h

// Qt includes
#include <QObject>

// Terminologies includes
#include "qSlicerTerminologiesModuleWidgetsExport.h"

class qSlicerTerminologySelectorDialogPrivate;

class vtkSlicerTerminologyEntry;

/// \brief Qt dialog for selecting a terminology entry
/// \ingroup SlicerRt_QtModules_Terminologies_Widgets
class Q_SLICER_MODULE_TERMINOLOGIES_WIDGETS_EXPORT qSlicerTerminologySelectorDialog : public QObject
{
public:
  Q_OBJECT

public:
  typedef QObject Superclass;
  qSlicerTerminologySelectorDialog(vtkSlicerTerminologyEntry* initialTerminology, QObject* parent = NULL);
  virtual ~qSlicerTerminologySelectorDialog();

public:
  /// Convenience function to start dialog, initialized with a terminology entry
  /// \param terminology Initial terminology shown by the dialog. The selected terminology is set to this as well.
  /// \return Success flag
  static bool getTerminology(vtkSlicerTerminologyEntry* terminology, QObject* parent);

  /// Show dialog
  /// \param nodeToSelect Node is selected in the tree if given
  /// \return Success flag (if dialog result is not Accepted then false)
  virtual bool exec();

  /// Python compatibility function for showing dialog (calls \a exec)
  Q_INVOKABLE bool execDialog() { return this->exec(); };

protected:
  /// Populate output terminology entry (same as what was passed in the argument of \a exec) from
  /// terminology and anatomy selection in the terminology navigator widget
  /// \return Success flag
  bool updateTerminologyEntryFromWidget();

protected:
  QScopedPointer<qSlicerTerminologySelectorDialogPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerTerminologySelectorDialog);
  Q_DISABLE_COPY(qSlicerTerminologySelectorDialog);
};

#endif
