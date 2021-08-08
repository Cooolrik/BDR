#include "Vlk_Common.inl"
#include "Vlk_DescriptorLayout.h"
#include "Vlk_Renderer.h"
#include "Vlk_Image.h"

Vlk::DescriptorLayout::~DescriptorLayout()
	{
	if( this->DescriptorSetLayout != nullptr )
		{
		vkDestroyDescriptorSetLayout( this->Parent->GetDevice(), this->DescriptorSetLayout, nullptr );
		}
	}

uint Vlk::DescriptorLayout::GetBindingCount() const
	{
	return (uint)Bindings.size();
	}

uint Vlk::DescriptorLayout::AddUniformBufferBinding( VkShaderStageFlags stageFlags, uint arrayCount )
	{
	if( this->DescriptorSetLayout != nullptr )
		{
		throw runtime_error( "Error: Add[...]Binding: The descriptor layout is already built." );
		}

	uint bindingIndex = (uint)this->Bindings.size();
	this->Bindings.push_back( {} );
	this->Bindings[bindingIndex].binding = bindingIndex;
	this->Bindings[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	this->Bindings[bindingIndex].descriptorCount = arrayCount;
	this->Bindings[bindingIndex].stageFlags = stageFlags;

	return bindingIndex;
	}

uint Vlk::DescriptorLayout::AddStorageBufferBinding( VkShaderStageFlags stageFlags, uint arrayCount )
	{
	if( this->DescriptorSetLayout != nullptr )
		{
		throw runtime_error( "Error: Add[...]Binding: The descriptor layout is already built." );
		}

	uint bindingIndex = (uint)this->Bindings.size();
	this->Bindings.push_back( {} );
	this->Bindings[bindingIndex].binding = bindingIndex;
	this->Bindings[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	this->Bindings[bindingIndex].descriptorCount = arrayCount;
	this->Bindings[bindingIndex].stageFlags = stageFlags;

	return bindingIndex;
	}

uint Vlk::DescriptorLayout::AddSamplerBinding( VkShaderStageFlags stageFlags, uint arrayCount )
	{
	if( this->DescriptorSetLayout != nullptr )
		{
		throw runtime_error( "Error: Add[...]Binding: The descriptor layout is already built." );
		}

	uint bindingIndex = (uint)this->Bindings.size();
	this->Bindings.push_back( {} );
	this->Bindings[bindingIndex].binding = bindingIndex;
	this->Bindings[bindingIndex].descriptorCount = arrayCount;
	this->Bindings[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	this->Bindings[bindingIndex].stageFlags = stageFlags;

	return bindingIndex;
	}

uint Vlk::DescriptorLayout::AddAccelerationStructureBinding( VkShaderStageFlags stageFlags, uint arrayCount )
	{
	if( this->DescriptorSetLayout != nullptr )
		{
		throw runtime_error( "Error: Add[...]Binding: The descriptor layout is already built." );
		}

	uint bindingIndex = (uint)this->Bindings.size();
	this->Bindings.push_back( {} );
	this->Bindings[bindingIndex].binding = bindingIndex;
	this->Bindings[bindingIndex].descriptorCount = arrayCount;
	this->Bindings[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	this->Bindings[bindingIndex].stageFlags = stageFlags;

	return bindingIndex;
	}

uint Vlk::DescriptorLayout::AddStoredImageBinding( VkShaderStageFlags stageFlags, uint arrayCount )
	{
	if( this->DescriptorSetLayout != nullptr )
		{
		throw runtime_error( "Error: Add[...]Binding: The descriptor layout is already built." );
		}

	uint bindingIndex = (uint)this->Bindings.size();
	this->Bindings.push_back( {} );
	this->Bindings[bindingIndex].binding = bindingIndex;
	this->Bindings[bindingIndex].descriptorCount = arrayCount;
	this->Bindings[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	this->Bindings[bindingIndex].stageFlags = stageFlags;

	return bindingIndex;
	}

void Vlk::DescriptorLayout::BuildDescriptorSetLayout()
	{
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = (uint32_t)this->Bindings.size();
	layoutInfo.pBindings = this->Bindings.data();
	VLK_CALL( vkCreateDescriptorSetLayout( this->Parent->GetDevice(), &layoutInfo, nullptr, &this->DescriptorSetLayout ) );
	}
