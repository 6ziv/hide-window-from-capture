#include <Windows.h>
#include <strsafe.h>
#include <detours/detours.h>
#include <boost/dll/runtime_symbol_info.hpp>
int main(int argc, char** argv)
{
	if (argc != 3)return ERROR_INVALID_PARAMETER;
	DWORD pid = strtoul(argv[1], nullptr, 10);
	uintptr_t ptr = static_cast<uintptr_t>(strtoull(argv[2], nullptr, 16));

	if (pid == 0 || ptr == 0)return ERROR_INVALID_PARAMETER;;

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess == NULL) {
		return GetLastError();
	}
	HANDLE hThread = CreateRemoteThreadEx(hProcess, NULL, NULL, (PTHREAD_START_ROUTINE)LoadLibraryW, reinterpret_cast<LPVOID>(ptr), NULL, NULL, NULL);
	if (hThread == NULL) {
		return GetLastError();
	}
	WaitForSingleObject(hThread, INFINITE);
	DWORD ec;
	GetExitCodeThread(hThread, &ec);
	CloseHandle(hThread);
	return (ec != 0) ? ERROR_SUCCESS : ERROR_DLL_INIT_FAILED;
}
