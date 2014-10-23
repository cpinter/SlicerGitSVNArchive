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

#ifndef __vtkMRMLSubjectHierarchyConstants_h
#define __vtkMRMLSubjectHierarchyConstants_h

#include "vtkSlicerSubjectHierarchyModuleMRMLExport.h"

// STD includes
#include <cstdlib>
#include <string>

class VTK_SLICER_SUBJECTHIERARCHY_MODULE_MRML_EXPORT vtkMRMLSubjectHierarchyConstants
{
public:
  //----------------------------------------------------------------------------
  // Constant strings (std::string types for easy concatenation)
  //----------------------------------------------------------------------------

  // Subject hierarchy constants
  static const std::string GetSubjectHierarchyNodeNamePostfix()
    { return "_SubjectHierarchy"; };
  static const std::string GetSubjectHierarchyAttributePrefix()
    { return "SubjectHierarchy."; };
  static const std::string GetSubjectHierarchyExcludeFromPotentialNodesListAttributeName()
    { return vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyAttributePrefix() + "ExcludeFromPotentialNodesList"; };
  static const std::string GetSubjectHierarchyNewNodeNamePrefix()
    { return "New"; };

  static const char* GetSubjectHierarchyLevelSubject()
    { return "Subject"; };
  static const char* GetSubjectHierarchyLevelStudy()
    { return "Study"; };

  // DICOM plugin constants
  static const char* GetDICOMLevelSeries()
    { return "Series"; };
  static const char* GetDICOMLevelSubseries()
    { return "Subseries"; };

  static const std::string GetDICOMAttributePrefix()
    { return "DICOM."; };
  static const std::string GetDICOMPatientNameAttributeName()
    { return vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix() + "PatientName"; };
  static const std::string GetDICOMPatientIDAttributeName()
    { return vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix() + "PatientID"; };
  static const std::string GetDICOMPatientSexAttributeName()
    { return vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix() + "PatientSex"; };
  static const std::string GetDICOMPatientBirthDateAttributeName()
    { return vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix() + "PatientBirthDate"; };
  static const std::string GetDICOMPatientCommentsAttributeName()
    { return vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix() + "PatientComments"; };
  static const std::string GetDICOMStudyDescriptionAttributeName()
    { return vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix() + "StudyDescription"; };
  static const std::string GetDICOMStudyDateAttributeName()
    { return vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix() + "StudyDate"; };
  static const std::string GetDICOMStudyTimeAttributeName()
    { return vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix() + "StudyTime"; };
  static const std::string GetDICOMSeriesModalityAttributeName()
    { return vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix() + "SeriesModality"; };
  static const std::string GetDICOMSeriesNumberAttributeName()
    { return vtkMRMLSubjectHierarchyConstants::GetDICOMAttributePrefix() + "SeriesNumber"; };
  static const char* GetDICOMUIDName()
    { return "DICOM"; };
};

#endif
