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

// SubjectHierarchy includes
#include <vtkSlicerSubjectHierarchyModuleLogic.h>
#include <vtkMRMLSubjectHierarchyConstants.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLStorageNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
const char* vtkSlicerSubjectHierarchyModuleLogic::CLONED_NODE_NAME_POSTFIX = " Copy";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSubjectHierarchyModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerSubjectHierarchyModuleLogic::vtkSlicerSubjectHierarchyModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerSubjectHierarchyModuleLogic::~vtkSlicerSubjectHierarchyModuleLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerSubjectHierarchyModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerSubjectHierarchyModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerSubjectHierarchyModuleLogic::RegisterNodes()
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
    }

  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerSubjectHierarchyModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene!");
    return;
    }

  this->Modified();
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode* vtkSlicerSubjectHierarchyModuleLogic::GetSubjectHierarchyNode(vtkMRMLScene* scene)
{
  if (!scene)
    {
    vtkGenericWarningMacro("vtkSlicerSubjectHierarchyModuleLogic::GetSubjectHierarchyNode: Invalid scene given");
    return NULL;
    }
  if (scene->GetNumberOfNodesByClass("vtkMRMLSubjectHierarchyNode") == 0)
    {
    vtkSmartPointer<vtkMRMLSubjectHierarchyNode> newShNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
    newShNode->SetName("SubjectHierarchy");
    scene->AddNode(newShNode);

    vtkDebugWithObjectMacro( newShNode, "vtkSlicerSubjectHierarchyModuleLogic::GetSubjectHierarchyNode: "
      "New subject hierarchy node created as none was found in the scene" );
    return newShNode;
    }

  // Return subject hierarchy node if there is only one
  scene->InitTraversal();
  vtkMRMLSubjectHierarchyNode* firstShNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(
    scene->GetNextNodeByClass("vtkMRMLSubjectHierarchyNode") );
  if (scene->GetNumberOfNodesByClass("vtkMRMLSubjectHierarchyNode") == 1)
    {
    return firstShNode;
    }

  // Do not perform merge operations while the scene is processing
  if (scene->IsBatchProcessing() || scene->IsImporting() || scene->IsClosing())
    {
    vtkWarningWithObjectMacro(scene, "vtkSlicerSubjectHierarchyModuleLogic::GetSubjectHierarchyNode: "
      "Scene is processing, merging subject hierarchies is not possible" );
    return NULL;
    }

  // Merge subject hierarchy nodes into the first one found
  for (vtkMRMLNode* node=NULL; (node=scene->GetNextNodeByClass("vtkMRMLSubjectHierarchyNode")); )
    {
    vtkMRMLSubjectHierarchyNode* currentShNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(node);
    if (currentShNode)
      {
      if (!firstShNode->MergeSubjectHierarchy(currentShNode))
        {
        //TODO: The node will probably be invalid, so it needs to be completely re-built
        vtkErrorWithObjectMacro(scene, "vtkSlicerSubjectHierarchyModuleLogic::GetSubjectHierarchyNode: Failed to merge subject hierarchy nodes");
        return firstShNode;
        }
      }
    }
  // Return the first (and now only) subject hierarchy node into which the others were merged
  return firstShNode;
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkSlicerSubjectHierarchyModuleLogic::InsertDicomSeriesInHierarchy(
  vtkMRMLSubjectHierarchyNode* shNode, const char* patientId, const char* studyInstanceUID, const char* seriesInstanceUID )
{
  if ( !shNode || patientId || !studyInstanceUID || !seriesInstanceUID )
    {
    vtkGenericWarningMacro("vtkSlicerSubjectHierarchyModuleLogic::InsertDicomSeriesInHierarchy: Invalid input arguments!");
    return vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
    }

  SubjectHierarchyItemID patientItemID = vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
  SubjectHierarchyItemID studyItemID = vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
  std::vector<SubjectHierarchyItemID> seriesItemIDs;

  // Find referenced items
  std::vector<SubjectHierarchyItemID> allItemIDs;
  shNode->GetItemChildren(shNode->GetSceneItemID(), allItemIDs, true);
  for (std::vector<SubjectHierarchyItemID>::iterator itemIt=allItemIDs.begin(); itemIt!=allItemIDs.end(); ++itemIt)
    {
    SubjectHierarchyItemID currentItemID = (*itemIt);
    std::string nodeDicomUIDStr = shNode->GetItemUID(currentItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName());
    const char* nodeDicomUID = nodeDicomUIDStr.c_str();
    if (!nodeDicomUID)
      {
      // Having a UID is not mandatory
      continue;
      }
    if (!strcmp(patientId, nodeDicomUID))
      {
      patientItemID = currentItemID;
      }
    else if (!strcmp(studyInstanceUID, nodeDicomUID))
      {
      studyItemID = currentItemID;
      }
    else if (!strcmp(seriesInstanceUID, nodeDicomUID))
      {
      seriesItemIDs.push_back(currentItemID);
      }
    }

  if (seriesItemIDs.empty())
    {
    vtkErrorWithObjectMacro(shNode,
      "vtkSlicerSubjectHierarchyModuleLogic::InsertDicomSeriesInHierarchy: Subject hierarchy item with DICOM UID '"
      << seriesInstanceUID << "' cannot be found!");
    return NULL;
    }

  // Create patient and study nodes if they do not exist yet
  if (patientItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    patientItemID = shNode->CreateSubjectHierarchyItem(
      shNode->GetSceneItemID(), NULL, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelPatient() );
    shNode->SetItemUID(patientItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), patientId);
    shNode->SetItemOwnerPluginName(patientItemID, "DICOM");
    }

  if (studyItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    studyItemID = shNode->CreateSubjectHierarchyItem(
      patientItemID, NULL, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy() );
    shNode->SetItemUID(studyItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMUIDName(), studyInstanceUID);
    shNode->SetItemOwnerPluginName(studyItemID, "DICOM");
    }

  // In some cases there might be multiple subject hierarchy nodes for the same DICOM series,
  // for example if a series contains instances that load to different node types that cannot
  // be simply added under one series folder node. This can happen if for one type the node
  // corresponds to the series, but in the other to the instances.
  for (std::vector<SubjectHierarchyItemID>::iterator seriesIt = seriesItemIDs.begin(); seriesIt != seriesItemIDs.end(); ++seriesIt)
  {
    SubjectHierarchyItemID currentSeriesID = (*seriesIt);
    shNode->SetItemParent(currentSeriesID, studyItemID);
  }

  if (seriesItemIDs.size() > 1)
  {
    vtkDebugWithObjectMacro(shNode,
      "vtkSlicerSubjectHierarchyModuleLogic::InsertDicomSeriesInHierarchy: DICOM UID '"
      << seriesInstanceUID << "' corresponds to multiple series subject hierarchy nodes, but only the first one is returned");
  }

  return *(seriesItemIDs.begin());
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkSlicerSubjectHierarchyModuleLogic::AreItemsInSameBranch(
    vtkMRMLSubjectHierarchyNode* shNode, vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID item1, vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID item2,
    const char* lowestCommonLevel )
{
  if (!shNode)
    {
    vtkGenericWarningMacro("vtkSlicerSubjectHierarchyModuleLogic::AreItemsInSameBranch: Invalid subject hierarchy node given");
    return vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
    }
  if (item1 == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID || item2 == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    vtkErrorWithObjectMacro(shNode, "vtkSlicerSubjectHierarchyModuleLogic::AreItemsInSameBranch: Invalid input items");
    return vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
    }
  if (!lowestCommonLevel)
    {
    vtkErrorWithObjectMacro(shNode, "vtkSlicerSubjectHierarchyModuleLogic::AreItemsInSameBranch: Invalid lowest common level");
    return vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
    }

  // Walk the hierarchy up until we reach the lowest common level
  SubjectHierarchyItemID ancestor1 = item1;
  while (true)
    {
    ancestor1 = shNode->GetItemParent(ancestor1);
    if (ancestor1 == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID || ancestor1 == shNode->GetSceneItemID())
      {
      vtkDebugWithObjectMacro(shNode, "Item ('" << shNode->GetItemName(item1) << "') has no ancestor with level '" << lowestCommonLevel << "'");
      ancestor1 = vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
      break;
      }
    std::string item1Level = shNode->GetItemLevel(ancestor1);
    if (item1Level.empty())
      {
      vtkDebugWithObjectMacro(shNode, "Item ('" << shNode->GetItemName(ancestor1) << "') has invalid level property");
      break;
      }
    if (!item1Level.compare(lowestCommonLevel))
      {
      break;
      }
    }

  SubjectHierarchyItemID ancestor2 = item2;
  while (true)
    {
    ancestor2 = shNode->GetItemParent(ancestor2);
    if (ancestor2 == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID || ancestor2 == shNode->GetSceneItemID())
      {
      vtkDebugWithObjectMacro(shNode, "Item ('" << shNode->GetItemName(item2) << "') has no ancestor with level '" << lowestCommonLevel << "'");
      ancestor2 = vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
      break;
      }
    std::string item2Level = shNode->GetItemLevel(ancestor2);
    if (item2Level.empty())
      {
      vtkDebugWithObjectMacro(shNode, "Item ('" << shNode->GetItemName(ancestor2) << "') has invalid level property");
      break;
      }
    if (!item2Level.compare(lowestCommonLevel))
      {
      break;
      }
    }

  return (ancestor1 == ancestor2 ? ancestor1 : vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID);
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkSlicerSubjectHierarchyModuleLogic::AreNodesInSameBranch(
  vtkMRMLNode* node1, vtkMRMLNode* node2, const char* lowestCommonLevel )
{
  if (!node1 || !node2 || !node1->GetScene() || node1->GetScene() != node2->GetScene())
    {
    vtkGenericWarningMacro("vtkSlicerSubjectHierarchyModuleLogic::AreNodesInSameBranch: Invalid input nodes or they are not in the same scene!");
    return vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
    }

  vtkMRMLSubjectHierarchyNode* shNode = vtkSlicerSubjectHierarchyModuleLogic::GetSubjectHierarchyNode(node1->GetScene());
  SubjectHierarchyItemID item1 = shNode->GetSubjectHierarchyItemByDataNode(node1);
  SubjectHierarchyItemID item2 = shNode->GetSubjectHierarchyItemByDataNode(node2);

  return vtkSlicerSubjectHierarchyModuleLogic::AreItemsInSameBranch(shNode, item1, item2, lowestCommonLevel);
}

//---------------------------------------------------------------------------
bool vtkSlicerSubjectHierarchyModuleLogic::IsPatientTag(std::string tagName)
{
  std::vector<std::string> patientTagNames = vtkMRMLSubjectHierarchyConstants::GetDICOMPatientTagNames();
  for ( std::vector<std::string>::iterator patientTagIt = patientTagNames.begin();
    patientTagIt != patientTagNames.end(); ++patientTagIt )
    {
    if (!tagName.compare(*patientTagIt))
      {
      // Argument was found in patient tag names list, so given tag is a patient tag
      return true;
      }
    }
  return false;
}

//---------------------------------------------------------------------------
bool vtkSlicerSubjectHierarchyModuleLogic::IsStudyTag(std::string tagName)
{
  std::vector<std::string> studyTagNames = vtkMRMLSubjectHierarchyConstants::GetDICOMStudyTagNames();
  for ( std::vector<std::string>::iterator studyTagIt = studyTagNames.begin();
    studyTagIt != studyTagNames.end(); ++studyTagIt )
    {
    if (!tagName.compare(*studyTagIt))
      {
      // Argument was found in study tag names list, so given tag is a study tag
      return true;
      }
    }
  return false;
}

//---------------------------------------------------------------------------
void vtkSlicerSubjectHierarchyModuleLogic::TransformBranch(
  vtkMRMLSubjectHierarchyNode* shNode, SubjectHierarchyItemID itemID, vtkMRMLTransformNode* transformNode, bool hardenExistingTransforms/*=true*/)
{
  if (!shNode)
    {
    vtkGenericWarningMacro("vtkSlicerSubjectHierarchyModuleLogic::TransformBranch: Invalid subject hierarchy node!");
    return;
    }

  // Get all associated data nodes from children nodes (and itself)
  std::vector<SubjectHierarchyItemID> childIDs;
  shNode->GetItemChildren(itemID, childIDs, true);
  childIDs.push_back(itemID);

  for (std::vector<SubjectHierarchyItemID>::iterator childIt=childIDs.begin(); childIt!=childIDs.end(); ++childIt)
    {
    vtkMRMLTransformableNode* transformableNode = vtkMRMLTransformableNode::SafeDownCast(
      shNode->GetItemDataNode(*childIt) );
    if (!transformableNode)
      {
      continue;
      }
    if (transformableNode == transformNode)
      {
      // Transform node cannot be transformed by itself
      continue;
      }

    vtkMRMLTransformNode* parentTransformNode = transformableNode->GetParentTransformNode();
    if (parentTransformNode)
      {
      // Do nothing if the parent transform matches the specified transform to apply
      if (parentTransformNode == transformNode)
        {
        //vtkDebugMacro("TransformBranch: Specified transform " << transformNode->GetName() << " already applied on data node belonging to subject hierarchy node " << this->Name);
        continue;
        }
      // Harden existing parent transform if this option was chosen
      if (hardenExistingTransforms)
        {
        //vtkDebugMacro("TransformBranch: Hardening transform " << transformNode->GetName() << " on node " << transformableNode->GetName());
        transformableNode->HardenTransform();
        }
      }

    // Apply the transform
    transformableNode->SetAndObserveTransformNodeID(transformNode ? transformNode->GetID() : NULL);

    // Trigger update by invoking the modified event for the subject hierarchy item
    shNode->ItemModified(*childIt);
    }
}

//---------------------------------------------------------------------------
void vtkSlicerSubjectHierarchyModuleLogic::HardenTransformOnBranch(
  vtkMRMLSubjectHierarchyNode* shNode, vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID)
{
  if (!shNode)
    {
    vtkGenericWarningMacro("vtkSlicerSubjectHierarchyModuleLogic::HardenTransformOnBranch: Invalid subject hierarchy node!");
    return;
    }

  // Get all associated data nodes from children nodes (and itself)
  std::vector<SubjectHierarchyItemID> childIDs;
  shNode->GetItemChildren(itemID, childIDs, true);
  childIDs.push_back(itemID);

  for (std::vector<SubjectHierarchyItemID>::iterator childIt=childIDs.begin(); childIt!=childIDs.end(); ++childIt)
    {
    vtkMRMLTransformableNode* transformableNode = vtkMRMLTransformableNode::SafeDownCast(
      shNode->GetItemDataNode(*childIt) );
    if (!transformableNode)
      {
      continue;
      }
    transformableNode->HardenTransform();

    // Trigger update by invoking the modified event for the subject hierarchy item
    shNode->ItemModified(*childIt);
    }
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkSlicerSubjectHierarchyModuleLogic::CloneSubjectHierarchyItem(
  vtkMRMLSubjectHierarchyNode* shNode, vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID, const char* name/*=NULL*/)
{
  if (!shNode)
    {
    vtkGenericWarningMacro("vtkSlicerSubjectHierarchyModuleLogic::CloneSubjectHierarchyItem: Invalid subject hierarchy node!");
    return vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
    }
  vtkMRMLScene* scene = shNode->GetScene();
  if (!scene)
    {
    vtkErrorWithObjectMacro( shNode, "vtkSlicerSubjectHierarchyModuleLogic::CloneSubjectHierarchyItem: "
      "Invalid scene for subject hierarchy node " << shNode->GetName() );
    return vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
    }

  SubjectHierarchyItemID clonedSubjectHierarchyItemID = vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
  vtkMRMLNode* associatedDataNode = shNode->GetItemDataNode(itemID);
  if (associatedDataNode)
    {
    // Create data node clone
    vtkSmartPointer<vtkMRMLNode> clonedDataNode;
    clonedDataNode.TakeReference(scene->CreateNodeByClass(associatedDataNode->GetClassName()));
    std::string clonedDataNodeName = ( name ? std::string(name) : std::string(associatedDataNode->GetName()) + std::string(CLONED_NODE_NAME_POSTFIX) );
    scene->AddNode(clonedDataNode);

    // Clone display node
    vtkSmartPointer<vtkMRMLDisplayNode> clonedDisplayNode;
    vtkMRMLDisplayableNode* displayableDataNode = vtkMRMLDisplayableNode::SafeDownCast(associatedDataNode);
    if (displayableDataNode && displayableDataNode->GetDisplayNode())
      {
      // If display node was automatically created by the specific module logic when the data node was added to the scene, then do not create it
      vtkMRMLDisplayableNode* clonedDisplayableDataNode = vtkMRMLDisplayableNode::SafeDownCast(clonedDataNode);
      if (clonedDisplayableDataNode->GetDisplayNode())
        {
        clonedDisplayNode = clonedDisplayableDataNode->GetDisplayNode();
        }
      else
        {
        clonedDisplayNode = vtkSmartPointer<vtkMRMLDisplayNode>::Take( vtkMRMLDisplayNode::SafeDownCast(
          scene->CreateNodeByClass(displayableDataNode->GetDisplayNode()->GetClassName()) ) );
        clonedDisplayNode->Copy(displayableDataNode->GetDisplayNode());
        std::string clonedDisplayNodeName = clonedDataNodeName + "_Display";
        clonedDisplayNode->SetName(clonedDisplayNodeName.c_str());
        scene->AddNode(clonedDisplayNode);
        clonedDisplayableDataNode->SetAndObserveDisplayNodeID(clonedDisplayNode->GetID());
        }
      }

    // Clone storage node
    vtkSmartPointer<vtkMRMLStorageNode> clonedStorageNode;
    vtkMRMLStorableNode* storableDataNode = vtkMRMLStorableNode::SafeDownCast(associatedDataNode);
    if (storableDataNode && storableDataNode->GetStorageNode())
      {
      // If storage node was automatically created by the specific module logic when the data node was added to the scene, then do not create it
      vtkMRMLStorableNode* clonedStorableDataNode = vtkMRMLStorableNode::SafeDownCast(clonedDataNode);
      if (clonedStorableDataNode->GetStorageNode())
        {
        clonedStorageNode = clonedStorableDataNode->GetStorageNode();
        }
      else
        {
        clonedStorageNode = vtkSmartPointer<vtkMRMLStorageNode>::Take( vtkMRMLStorageNode::SafeDownCast(
          scene->CreateNodeByClass(storableDataNode->GetStorageNode()->GetClassName()) ) );
        clonedStorageNode->Copy(storableDataNode->GetStorageNode());
        if (storableDataNode->GetStorageNode()->GetFileName())
          {
          std::string clonedStorageNodeFileName = std::string(storableDataNode->GetStorageNode()->GetFileName()) + std::string(CLONED_NODE_NAME_POSTFIX);
          clonedStorageNode->SetFileName(clonedStorageNodeFileName.c_str());
          }
        scene->AddNode(clonedStorageNode);
        clonedStorableDataNode->SetAndObserveStorageNodeID(clonedStorageNode->GetID());
        }
      }

    // Copy data node
    // Display and storage nodes might be involved in the copy process, so they are needed to be set up before the copy operation
    clonedDataNode->Copy(associatedDataNode);
    clonedDataNode->SetName(clonedDataNodeName.c_str());
    // Copy overwrites display and storage node references too, need to restore
    if (clonedDisplayNode.GetPointer())
      {
      vtkMRMLDisplayableNode::SafeDownCast(clonedDataNode)->SetAndObserveDisplayNodeID(clonedDisplayNode->GetID());
      }
    if (clonedStorageNode.GetPointer())
      {
      vtkMRMLStorableNode::SafeDownCast(clonedDataNode)->SetAndObserveStorageNodeID(clonedStorageNode->GetID());
      }
    // Trigger display update (needed to invoke update of transforms in displayable managers)
    vtkMRMLTransformableNode* transformableClonedNode = vtkMRMLTransformableNode::SafeDownCast(clonedDataNode);
    if (transformableClonedNode && transformableClonedNode->GetParentTransformNode())
    {
      transformableClonedNode->GetParentTransformNode()->Modified();
    }

    // Get hierarchy nodes
    vtkMRMLHierarchyNode* genericHierarchyNode =
      vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(scene, associatedDataNode->GetID());

    // Put data node in the same non-subject hierarchy if any
    if (genericHierarchyNode != node)
      {
      vtkSmartPointer<vtkMRMLHierarchyNode> clonedHierarchyNode;
      clonedHierarchyNode.TakeReference( vtkMRMLHierarchyNode::SafeDownCast(
        scene->CreateNodeByClass(genericHierarchyNode->GetClassName()) ) );
      clonedHierarchyNode->Copy(genericHierarchyNode);
      std::string clonedHierarchyNodeName = std::string(genericHierarchyNode->GetName()) + std::string(CLONED_NODE_NAME_POSTFIX);
      clonedHierarchyNode->SetName(clonedHierarchyNodeName.c_str());
      scene->AddNode(clonedHierarchyNode);
      clonedHierarchyNode->SetAssociatedNodeID(clonedDataNode->GetID());
      }

    // Put data node in the same subject hierarchy branch as current node
    clonedSubjectHierarchyItemID = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyItem(
      shNode->GetItemParent(itemID), clonedDataNode, shNode->GetItemLevel(itemID) );

    // Trigger update by invoking the modified event for the subject hierarchy item
    shNode->ItemModified(clonedSubjectHierarchyItemID);
    }
  else // No associated node
    {
    std::string clonedItemName = ( name ? std::string(name) : std::string(shNode->GetItemName(itemID)) + std::string(CLONED_NODE_NAME_POSTFIX) );

    clonedSubjectHierarchyItemID = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyItem(
      shNode->GetItemParent(itemID), NULL, shNode->GetItemLevel(itemID), clonedItemName );
    }

  return clonedSubjectHierarchyItemID;
}

//---------------------------------------------------------------------------
bool vtkSlicerSubjectHierarchyModuleLogic::MergeSubjectHierarchyNodes(
  vtkMRMLSubjectHierarchyNode* shNodeMerged, vtkMRMLSubjectHierarchyNode* shNodeRemoved)
{
  //TODO:
  vtkGenericWarningMacro("vtkSlicerSubjectHierarchyModuleLogic::MergeSubjectHierarchyNodes: Not implemented!");
  return false;
}
