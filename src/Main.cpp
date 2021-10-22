
#include <SDL.h>
#include <iostream>
#include <vulkan/vulkan.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cout << "Couldn't initialise SDL\n";
		return 1;
	}

	std::cout << "SDL initialised successfully\n";

	SDL_Quit();

	VkResult result = VK_SUCCESS;

	const auto createInfo = VkInstanceCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
	    .pApplicationInfo = {},
	    .enabledLayerCount = 0,
	    .ppEnabledLayerNames = nullptr,
	    .enabledExtensionCount = 0,
	    .ppEnabledExtensionNames = nullptr};

	VkInstance instance = {};
	result = vkCreateInstance(&createInfo, nullptr, &instance);
	if (result == VK_SUCCESS)
	{
		std::cout << "Vulkan initialised sucessfully\n";
	}

	return 0;
}
