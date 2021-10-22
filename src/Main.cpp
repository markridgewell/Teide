
#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <string>
#include <vector>

static const vk::Optional<const vk::AllocationCallbacks> s_allocator = nullptr;

struct SDLWindowDeleter
{
	void operator()(SDL_Window* window) { SDL_DestroyWindow(window); }
};

using UniqueSDLWindow = std::unique_ptr<SDL_Window, SDLWindowDeleter>;


vk::UniqueSurfaceKHR CreateVulkanSurface(SDL_Window* window, const vk::Instance& instance)
{
	VkSurfaceKHR surfaceTmp = {};
	if (!SDL_Vulkan_CreateSurface(window, instance, &surfaceTmp))
	{
		return {};
	}
	return vk::UniqueSurfaceKHR(
	    surfaceTmp, vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderStatic>(instance, s_allocator, {}));
}

void EnableVulkanLayer(
    std::vector<const char*>& enabledLayers, const std::vector<vk::LayerProperties>& layerProperties, const char* layerName)
{
	std::string_view layerNameSV = layerName;
	if (std::ranges::find_if(layerProperties, [layerNameSV](const auto& layer) { return layer.layerName == layerNameSV; })
	    != layerProperties.end())
	{
		enabledLayers.push_back(layerName);
	}
	else
	{
		std::cout << "Warning! Vulkan layer " << layerName << " not enabled!\n";
	}
}

void LogVulkanError(std::string_view message, const vk::Result& result)
{
	std::cout << message << '\n';
	std::cout << "Error code : " << static_cast<int>(result) << " (" << to_string(result) << ")\n";
}

bool OnEvent(const SDL_Event& event)
{
	switch (event.type)
	{
		case SDL_QUIT:
			return false;

		default:
			return true;
	}
}

bool OnUpdate()
{
	return true;
}

void OnRender()
{}

int Run()
{
	const auto window = UniqueSDLWindow(
	    SDL_CreateWindow("Teide", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN), {});
	if (!window)
	{
		std::cout << "Couldn't create SDL window\n";
		std::cout << SDL_GetError() << "\n";
		return 1;
	}

	vk::ApplicationInfo applicationInfo{
	    .pApplicationName = "Teide", .applicationVersion = 1, .pEngineName = "Teide", .engineVersion = 1, .apiVersion = VK_API_VERSION_1_1};

	uint32_t extensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(window.get(), &extensionCount, nullptr);
	auto extensions = std::vector<const char*>(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(window.get(), &extensionCount, extensions.data());

	const auto layerProperties = vk::enumerateInstanceLayerProperties();
	if (layerProperties.result != vk::Result::eSuccess)
	{
		LogVulkanError("Couldn't enumerate instance layer properties", layerProperties.result);
		return 1;
	}

	std::vector<const char*> layers;
#if _DEBUG
	EnableVulkanLayer(layers, layerProperties.value, "VK_LAYER_KHRONOS_validation");
#endif

	const auto createInfo = vk::InstanceCreateInfo{
	    .pApplicationInfo = &applicationInfo,
	    .enabledLayerCount = static_cast<uint32_t>(layers.size()),
	    .ppEnabledLayerNames = layers.data(),
	    .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
	    .ppEnabledExtensionNames = extensions.data()};

	const auto [instanceResult, instance] = vk::createInstanceUnique(createInfo, s_allocator);
	if (instanceResult != vk::Result::eSuccess)
	{
		LogVulkanError("Couldn't initialise Vulkan", instanceResult);
		return 1;
	}

	std::cout << "Vulkan initialised sucessfully\n";

	const auto surface = CreateVulkanSurface(window.get(), instance.get());
	if (!surface)
	{
	    std::cout << "Couldn't create Vulkan surface\n";
	    std::cout << SDL_GetError() << "\n";
	    return 1;
	}

	SDL_Event event;
	bool running = true;
	while (running)
	{
	    while (SDL_PollEvent(&event))
	    {
	        running &= OnEvent(event);
	    }

	    running &= OnUpdate();
	    OnRender();
	}

	return 0;
}

int SDL_main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cout << "Couldn't initialise SDL\n";
		std::cout << SDL_GetError() << "\n";
		return 1;
	}

	std::cout << "SDL initialised successfully\n";

	int retcode = Run();

	SDL_Quit();

	return retcode;
}
