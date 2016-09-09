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

#ifndef __vtkSlicerTerminologiesModuleLogic_h
#define __vtkSlicerTerminologiesModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerTerminologiesModuleLogicExport.h"

/// \ingroup Slicer_QtModules_Terminologies
class VTK_SLICER_TERMINOLOGIES_LOGIC_EXPORT vtkSlicerTerminologiesModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerTerminologiesModuleLogic *New();
  vtkTypeMacro(vtkSlicerTerminologiesModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Load default terminology dictionary from JSON
  void LoadDefaultTerminology();

protected:
  vtkSlicerTerminologiesModuleLogic();
  virtual ~vtkSlicerTerminologiesModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  virtual void OnMRMLSceneEndClose();

private:
  vtkSlicerTerminologiesModuleLogic(const vtkSlicerTerminologiesModuleLogic&); // Not implemented
  void operator=(const vtkSlicerTerminologiesModuleLogic&);              // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
  friend class vtkInternal;
};

#endif
