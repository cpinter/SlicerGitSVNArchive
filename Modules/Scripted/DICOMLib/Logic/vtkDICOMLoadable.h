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

#ifndef __vtkDICOMLoadable_h
#define __vtkDICOMLoadable_h

// VTK includes
#include <vtkObject.h>
#include <vtkStringArray.h>

// DICOMLib includes
#include "vtkSlicerDICOMLibModuleLogicExport.h"

/// Container class for things that can be loaded from DICOM files into Slicer.
/// Each plugin returns a list of instances from its evaluate method and accepts
/// a list of these in its load method corresponding to the things the user has
/// selected for loading
class VTK_SLICER_DICOMLIB_LOGIC_EXPORT vtkDICOMLoadable : public vtkObject
{
public:
  static vtkDICOMLoadable *New();
  vtkTypeMacro(vtkDICOMLoadable,vtkObject);

public:
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  vtkSetStringMacro(Tooltip);
  vtkGetStringMacro(Tooltip);

  vtkSetStringMacro(Warning);
  vtkGetStringMacro(Warning);

  vtkSetObjectMacro(Files, vtkStringArray);
  vtkGetObjectMacro(Files, vtkStringArray);

  vtkGetMacro(Selected, bool);
  vtkSetMacro(Selected, bool);
  vtkBooleanMacro(Selected, bool);

  vtkGetMacro(Confidence, double);
  vtkSetMacro(Confidence, double);

protected:
  vtkDICOMLoadable();
  virtual ~vtkDICOMLoadable();

private:
  vtkDICOMLoadable(const vtkDICOMLoadable&); // Not implemented
  void operator=(const vtkDICOMLoadable&);   // Not implemented

protected:
  /// Name exposed to the user for the node
  char* Name;

  /// Extra information the user sees on mouse over of the thing
  char* Tooltip;

  /// Things the user should know before loading this data
  char* Warning;

  /// The file list of the data to be loaded
  vtkStringArray* Files;

  /// Is the object checked for loading by default
  bool Selected;

  /// Confidence - from 0 to 1 where 0 means low chance
  /// that the user actually wants to load their data this
  /// way up to 1, which means that the plugin is very confident
  /// that this is the best way to load the data.
  /// When more than one plugin marks the same series as
  /// selected, the one with the highest confidence is
  /// actually selected by default.  In the case of a tie,
  /// both series are selected for loading.
  double Confidence;
};

#endif
