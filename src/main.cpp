#include "std_include.hpp"

namespace main
{
	static utils::hook entry_point_hook;

	void initialize()
	{
		main::entry_point_hook.uninstall();
		components::loader::initialize();
	}

	void Uninitialize()
	{
		components::loader::uninitialize();
	}
}

__declspec(naked) void entry_point()
{
    __asm
    {
        // This has to be called, otherwise the hook is not uninstalled and we're deadlocking
        call main::initialize

		// same address as entry_point_hook.initialize
        mov eax, 5D92E3h
        jmp eax
    }
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD  ul_reason_for_call, LPVOID /*lpReserved*/)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		DWORD oldProtect;
		VirtualProtect(GetModuleHandle(nullptr), 0x257A000, PAGE_EXECUTE_READWRITE, &oldProtect); // find this with CFF Explorer - Optinal Headers - SizeOfImage

		Beep(523, 100);

		// Adress is the entry adress found in IDA under Exports
		main::entry_point_hook.initialize(0x5D92E3, entry_point)->install(); //This is the entry point for the Linkermod Bo1 Radiant
	}

	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		main::Uninitialize();
	}

	return TRUE;
}
