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

#ifndef __vtkDICOMExportable_h
#define __vtkDICOMExportable_h

// VTK includes
#include <vtkObject.h>
#include <vtkStringArray.h>

// DICOMLib includes
#include "vtkSlicerDICOMLibModuleLogicExport.h"

/// Container class for ways of exporting Slicer data into DICOM.
/// Each plugin returns a list of instances of this from its examineForExport
/// method so the DICOM module can build an appropriate interface to offer
/// user the options to export and perform the exporting operation.
class VTK_SLICER_DICOMLIB_LOGIC_EXPORT vtkDICOMExportable : public vtkObject
{
public:
  static vtkDICOMExportable *New();
  vtkTypeMacro(vtkDICOMExportable,vtkObject);

public:
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  vtkSetStringMacro(Tooltip);
  vtkGetStringMacro(Tooltip);

  vtkGetMacro(Confidence, double);
  vtkSetMacro(Confidence, double);

protected:
  vtkDICOMExportable();
  virtual ~vtkDICOMExportable();

private:
  vtkDICOMExportable(const vtkDICOMExportable&); // Not implemented
  void operator=(const vtkDICOMExportable&);     // Not implemented

protected:
  /// Name exposed to the user for the export method
  char* Name;

  /// Extra information the user sees on mouse over of the export option
  char* Tooltip;

  /// Confidence - from 0 to 1 where 0 means that the plugin
  /// cannot export the given node, up to 1 that means that the
  /// plugin considers itself the best plugin to export the node
  /// (in case of specialized objects, e.g. RT dose volume)
  double Confidence;
};

#endif
