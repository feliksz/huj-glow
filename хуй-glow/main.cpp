#include <chrono>     // std::chrono::milliseconds
#include <thread>     // std::this_thread::sleep_for

#include <Windows.h>
#include <TlHelp32.h>

int main()
{
	DWORD     process_id{ NULL };
	HANDLE    process_handle{ NULL };
	HWND		 window{ NULL };

	uintptr_t client_addr{ NULL };
	uintptr_t client_state_addr{ NULL };
	uintptr_t glow_object_manager_addr{ NULL };

	int32_t   game_state{ NULL };

	const bool render_when_occluded{ true };
	const float color[4]{ 1.00f, 0.00f, 1.00f, 1.00f };

	while (window == NULL)
	{
		// Get window handle
		window = FindWindowA(nullptr, "Counter-Strike: Global Offensive");

		// Sleep to avoid high CPU usage
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// Get process ID
	GetWindowThreadProcessId(window, &process_id);

	// Get process handle
	process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);

	// Function for getting address of certain module
	auto get_module_address = [](DWORD process_id, const char* module_name)
	{
		HANDLE th32_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process_id);

		if (th32_snapshot == INVALID_HANDLE_VALUE)
			return 0UL;

		MODULEENTRY32 module_entry;
		module_entry.dwSize = sizeof(MODULEENTRY32);

		if (Module32First(th32_snapshot, &module_entry))
		{
			if (!strcmp(module_entry.szModule, module_name))
			{
				CloseHandle(th32_snapshot);
				return reinterpret_cast<DWORD>(module_entry.modBaseAddr);
			}
		}

		while (Module32Next(th32_snapshot, &module_entry))
		{
			if (!strcmp(module_entry.szModule, module_name))
			{
				CloseHandle(th32_snapshot);
				return reinterpret_cast<DWORD>(module_entry.modBaseAddr);
			}
		}

		CloseHandle(th32_snapshot);
		return 0UL;
	};

	// Get client.dll Address
	client_addr = get_module_address(process_id, "client.dll");

	// Get ClientState Address
	ReadProcessMemory(process_handle,
		reinterpret_cast<void*>(get_module_address(process_id, "engine.dll") + 0x589DD4),
		&client_state_addr, sizeof(int32_t), 0);

	// Get GlowObjectManager Address
	ReadProcessMemory(process_handle, reinterpret_cast<void*>(client_addr + 0x5298078), &glow_object_manager_addr, sizeof(uintptr_t), 0);


	// Get ClientState->GameState
	ReadProcessMemory(process_handle, reinterpret_cast<void*>(client_state_addr + 0x108), &game_state, sizeof(int32_t), 0);

	for (;;)
	{
		// Is player fully connected to server?
		if (game_state != 6)
		{
			// Get ClientState->GameState
			ReadProcessMemory(process_handle, reinterpret_cast<void*>(client_state_addr + 0x108), &game_state, sizeof(int32_t), 0);
			continue;
		}

		for (int32_t i = 0; i < 32; i++)
		{
			uintptr_t entity;
			ReadProcessMemory(process_handle, reinterpret_cast<void*>(client_addr + 0x4D5022C + i * 0x10), &entity, sizeof(uintptr_t), 0);

			if (entity == NULL)
				continue;

			bool entity_dormant;
			ReadProcessMemory(process_handle, reinterpret_cast<void*>(entity + 0xED), &entity_dormant, sizeof(bool), 0);

			if (entity_dormant != NULL)
				continue;

			int32_t glow_index;
			ReadProcessMemory(process_handle, reinterpret_cast<void*>(entity + 0xA438), &glow_index, sizeof(int32_t), 0);

			// Set vGlowColor->flRed
			WriteProcessMemory(process_handle,
				reinterpret_cast<void*>(glow_object_manager_addr + glow_index * 0x38 + 0x4),
				&color[0],
				sizeof(float),
				0);

			// Set vGlowColor->flGreen
			WriteProcessMemory(process_handle,
				reinterpret_cast<void*>(glow_object_manager_addr + glow_index * 0x38 + 0x8),
				&color[1],
				sizeof(float),
				0);

			// Set vGlowColor->flBlue
			WriteProcessMemory(process_handle,
				reinterpret_cast<void*>(glow_object_manager_addr + glow_index * 0x38 + 0xC),
				&color[2],
				sizeof(float),
				0);

			// Set vGlowColor->flAlpha
			WriteProcessMemory(process_handle,
				reinterpret_cast<void*>(glow_object_manager_addr + glow_index * 0x38 + 0x10),
				&color[3],
				sizeof(float),
				0);

			// Set bRenderWhenOccluded
			WriteProcessMemory(process_handle,
				reinterpret_cast<void*>(glow_object_manager_addr + glow_index * 0x38 + 0x24),
				&render_when_occluded,
				sizeof(bool),
				0);

		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return 0;
}
