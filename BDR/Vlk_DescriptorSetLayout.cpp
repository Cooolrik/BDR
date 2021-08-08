#include "Vlk_Common.inl"
#include "Vlk_DescriptorSetLayout.h"
#include "Vlk_Renderer.h"
#include "Vlk_Image.h"

Vlk::DescriptorSetLayout::~DescriptorSetLayout()
	{
	if( this->DescriptorSetLayoutHandle != nullptr )
		{
		vkDestroyDescriptorSetLayout( this->Module->GetDevice(), this->DescriptorSetLayoutHandle, nullptr );
		}
	}

uint Vlk::DescriptorSetLayoutTemplate::AddUniformBufferBinding( VkShaderStageFlags shaderStages, uint arrayCount )
	{
	uint bindingIndex = (uint)this->Bindings.size();
	this->Bindings.emplace_back();
	this->Bindings[bindingIndex].binding = bindingIndex;
	this->Bindings[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	this->Bindings[bindingIndex].descriptorCount = arrayCount;
	this->Bindings[bindingIndex].stageFlags = shaderStages;

	return bindingIndex;
	}

uint Vlk::DescriptorSetLayoutTemplate::AddStorageBufferBinding( VkShaderStageFlags shaderStages, uint arrayCount )
	{
	uint bindingIndex = (uint)this->Bindings.size();
	this->Bindings.emplace_back();
	this->Bindings[bindingIndex].binding = bindingIndex;
	this->Bindings[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	this->Bindings[bindingIndex].descriptorCount = arrayCount;
	this->Bindings[bindingIndex].stageFlags = shaderStages;

	return bindingIndex;
	}

uint Vlk::DescriptorSetLayoutTemplate::AddSamplerBinding( VkShaderStageFlags shaderStages, uint arrayCount )
	{
	uint bindingIndex = (uint)this->Bindings.size();
	this->Bindings.emplace_back();
	this->Bindings[bindingIndex].binding = bindingIndex;
	this->Bindings[bindingIndex].descriptorCount = arrayCount;
	this->Bindings[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	this->Bindings[bindingIndex].stageFlags = shaderStages;

	return bindingIndex;
	}

uint Vlk::DescriptorSetLayoutTemplate::AddAccelerationStructureBinding( VkShaderStageFlags shaderStages, uint arrayCount )
	{
	uint bindingIndex = (uint)this->Bindings.size();
	this->Bindings.emplace_back();
	this->Bindings[bindingIndex].binding = bindingIndex;
	this->Bindings[bindingIndex].descriptorCount = arrayCount;
	this->Bindings[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	this->Bindings[bindingIndex].stageFlags = shaderStages;

	return bindingIndex;
	}

uint Vlk::DescriptorSetLayoutTemplate::AddStoredImageBinding( VkShaderStageFlags shaderStages, uint arrayCount )
	{
	uint bindingIndex = (uint)this->Bindings.size();
	this->Bindings.emplace_back();
	this->Bindings[bindingIndex].binding = bindingIndex;
	this->Bindings[bindingIndex].descriptorCount = arrayCount;
	this->Bindings[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	this->Bindings[bindingIndex].stageFlags = shaderStages;

	return bindingIndex;
	}
