#include "Vlk_Common.inl"

#include "Vlk_DescriptorPool.h"
#include "Vlk_DescriptorLayout.h"
#include "Vlk_Renderer.h"
#include "Vlk_Image.h"
#include "Vlk_RayTracingAccBuffer.h"
#include "Vlk_Buffer.h"

using namespace std;


VkDescriptorSet Vlk::DescriptorPool::BeginDescriptorSet( DescriptorLayout *descriptorLayout )
	{
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout layouts[1] = { descriptorLayout->GetDescriptorSetLayout() };

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = this->Pool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = layouts;

	VLK_CALL( vkAllocateDescriptorSets( this->Parent->GetDevice(), &descriptorSetAllocateInfo, &descriptorSet ) );

	// allocate the bindings descriptors
	std::vector<VkDescriptorSetLayoutBinding> Bindings = descriptorLayout->GetBindings();
	
	this->WriteDescriptorSets.resize( Bindings.size() );
	this->WriteDescriptorInfos.resize( Bindings.size() );

	// set up all the bindings
	for( size_t bindingIndex = 0; bindingIndex < Bindings.size(); ++bindingIndex )
		{
		this->WriteDescriptorSets[bindingIndex] = {};
		this->WriteDescriptorSets[bindingIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		this->WriteDescriptorSets[bindingIndex].dstSet = descriptorSet;
		this->WriteDescriptorSets[bindingIndex].dstBinding = (uint32_t)bindingIndex;
		this->WriteDescriptorSets[bindingIndex].dstArrayElement = 0;
		this->WriteDescriptorSets[bindingIndex].descriptorType = Bindings[bindingIndex].descriptorType;
		this->WriteDescriptorSets[bindingIndex].descriptorCount = Bindings[bindingIndex].descriptorCount;

		VkWriteDescriptorSet* writeDescriptorSet = &this->WriteDescriptorSets[bindingIndex];
		DescriptorInfo* descriptorInfo = &this->WriteDescriptorInfos[bindingIndex];
		uint descriptorCount = Bindings[bindingIndex].descriptorCount;

		// allocate write descriptors
		switch( Bindings[bindingIndex].descriptorType )
			{
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				descriptorInfo->DescriptorBufferInfo.resize( descriptorCount );
				writeDescriptorSet->pBufferInfo = descriptorInfo->DescriptorBufferInfo.data();
				break;
			
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				descriptorInfo->DescriptorImageInfo.resize( descriptorCount );
				writeDescriptorSet->pImageInfo = descriptorInfo->DescriptorImageInfo.data();
				break;

			// acceleration structure for the ray tracing extension
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:

				// set up extension write structure, add to linked list of extensions
				descriptorInfo->WriteDescriptorSetAccelerationStructureKHR = {};
				descriptorInfo->WriteDescriptorSetAccelerationStructureKHR.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
				descriptorInfo->WriteDescriptorSetAccelerationStructureKHR.accelerationStructureCount = descriptorCount;
				descriptorInfo->WriteDescriptorSetAccelerationStructureKHR.pNext = writeDescriptorSet->pNext;
				writeDescriptorSet->pNext = &descriptorInfo->WriteDescriptorSetAccelerationStructureKHR;

				descriptorInfo->AccelerationStructureKHR.resize( descriptorCount );
				descriptorInfo->WriteDescriptorSetAccelerationStructureKHR.pAccelerationStructures = descriptorInfo->AccelerationStructureKHR.data();
				break;
			}
		}

	return descriptorSet;
	}

void Vlk::DescriptorPool::SetBuffer( uint bindingIndex, Buffer* buffer, uint byteOffset )
	{
	this->SetBufferInArray( bindingIndex, 0, buffer, byteOffset );
	}

void Vlk::DescriptorPool::SetBufferInArray( uint bindingIndex, uint arrayIndex, Buffer* buffer, uint byteOffset )
	{
	if( this->WriteDescriptorSets[bindingIndex].pBufferInfo == nullptr )
		{
		throw runtime_error( "Error: SetBufferInArray(): The bound index is not set up for a buffer" );
		}
	if( arrayIndex >= this->WriteDescriptorSets[bindingIndex].descriptorCount )
		{
		throw runtime_error( "Error: SetBufferInArray(): arrayIndex is out of range" );
		}

	// set up the info at {bindingIndex,arrayIndex} 
	this->WriteDescriptorInfos[bindingIndex].DescriptorBufferInfo[arrayIndex].buffer = buffer->GetBuffer();
	this->WriteDescriptorInfos[bindingIndex].DescriptorBufferInfo[arrayIndex].offset = byteOffset;
	this->WriteDescriptorInfos[bindingIndex].DescriptorBufferInfo[arrayIndex].range = buffer->GetBufferSize() - byteOffset;
	}

void Vlk::DescriptorPool::SetImage( uint bindingIndex, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout )
	{
	this->SetImageInArray( bindingIndex, 0, imageView, sampler , imageLayout );
	}

void Vlk::DescriptorPool::SetImageInArray( uint bindingIndex, uint arrayIndex, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout )
	{
	if(this->WriteDescriptorSets[bindingIndex].pImageInfo == nullptr)
		{
		throw runtime_error( "Error: SetImageInArray(): The bound index is not set up for an image" );
		}
	if(arrayIndex >= this->WriteDescriptorSets[bindingIndex].descriptorCount)
		{
		throw runtime_error( "Error: SetImageInArray(): arrayIndex is out of range" );
		}

	// set up the info at {bindingIndex,arrayIndex} 
	VkDescriptorImageInfo imageInfo;
	imageInfo.imageView = imageView;
	imageInfo.sampler = sampler;
	imageInfo.imageLayout = imageLayout;
	this->WriteDescriptorInfos[bindingIndex].DescriptorImageInfo[arrayIndex] = imageInfo;
	}

void Vlk::DescriptorPool::SetAccelerationStructure( uint bindingIndex, RayTracingAccBuffer* accelerationStructure )
	{
	this->SetAccelerationStructureInArray( bindingIndex, 0, accelerationStructure );
	}

void Vlk::DescriptorPool::SetAccelerationStructureInArray( uint bindingIndex, uint arrayIndex, RayTracingAccBuffer* accelerationStructure )
	{
	if( this->WriteDescriptorSets[bindingIndex].descriptorType != VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR )
		{
		throw runtime_error( "Error: SetAccelerationStructure(): Invalid type bound, this should not be an acceleration structure" );
		}

	// set up the info at {bindingIndex,arrayIndex} 
	this->WriteDescriptorInfos[bindingIndex].AccelerationStructureKHR[arrayIndex] = accelerationStructure->GetAccelerationStructure();
	}

void Vlk::DescriptorPool::EndDescriptorSet()
	{
	vkUpdateDescriptorSets( this->Parent->GetDevice(), (uint32_t)this->WriteDescriptorSets.size(), this->WriteDescriptorSets.data(), 0, nullptr );
	}


void Vlk::DescriptorPool::ResetDescriptorPool()
	{
	vkResetDescriptorPool( this->Parent->GetDevice(), this->Pool , 0 );
	}

Vlk::DescriptorPool::~DescriptorPool()
	{
	vkDestroyDescriptorPool( this->Parent->GetDevice(), this->Pool, nullptr );
	}
