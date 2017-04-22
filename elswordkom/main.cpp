#include "generic.hpp"
#include "main_form.hpp"

int __stdcall WinMain(HINSTANCE instance, HINSTANCE prev_instance, char* command_line, int command_show)
{
#ifdef PRINT_DEBUG_INFO
	AllocConsole();
	SetConsoleTitle("Terminal");
	AttachConsole(GetCurrentProcessId());
	
	FILE* pFile = nullptr;
	freopen_s(&pFile, "CON", "r", stdin);
	freopen_s(&pFile, "CON", "w", stdout);
	freopen_s(&pFile, "CON", "w", stderr);
#endif
	
	try
	{
		elswordkom::gui::main_form::get_instance().show(true);
		elswordkom::execute();
	}
	catch (std::exception& exception)
	{
		MessageBox(0, exception.what(), "An exception occured!", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
	}

#ifdef PRINT_DEBUG_INFO
	FreeConsole();
#endif
	return 0;
}