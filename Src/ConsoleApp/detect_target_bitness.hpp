#pragma once
#include <Windows.h>
#include <sdkddkver.h>
#include <stdint.h>
enum ProcessBitness :int8_t
{
	PROCESS_UNKNOWN = -1,
	PROCESS_X86 = 0,
	PROCESS_X64 = 1
};
ProcessBitness detect_bitness(HANDLE hProcess) {
	BOOL ok;
#if _WIN32_WINNT >= 0x0602   //Windows 8
	PROCESS_MACHINE_INFORMATION mi;
	ok = GetProcessInformation(hProcess, ProcessMachineTypeInfo, &mi, sizeof(PROCESS_MACHINE_INFORMATION));
	if (ok) {
		USHORT sys = mi.ProcessMachine;
		if (sys == IMAGE_FILE_MACHINE_I386)return PROCESS_X86;
		if (sys == IMAGE_FILE_MACHINE_ARM)return PROCESS_X86;
		if (sys == IMAGE_FILE_MACHINE_IA64)return PROCESS_X64;
		if (sys == IMAGE_FILE_MACHINE_AMD64 || sys == IMAGE_FILE_MACHINE_ARM64)return PROCESS_X64;
		//return PROCESS_UNKNOWN;
	}
#endif
#if NTDDI_VERSION >= 0x0A000001 //Windows 10 1511 
	USHORT target, host;
	ok = IsWow64Process2(hProcess, &target, &host);
	if (ok) {
		USHORT sys = target;
		if (target == IMAGE_FILE_MACHINE_UNKNOWN || target == IMAGE_FILE_MACHINE_TARGET_HOST)sys = host;

		if (sys == IMAGE_FILE_MACHINE_I386)return PROCESS_X86;
		if (sys == IMAGE_FILE_MACHINE_ARM)return PROCESS_X86;
		if (sys == IMAGE_FILE_MACHINE_IA64)return PROCESS_X64;
		if (sys == IMAGE_FILE_MACHINE_AMD64 || sys == IMAGE_FILE_MACHINE_ARM64)return PROCESS_X64;
		//return PROCESS_UNKNOWN;
	}
#endif
#if _WIN32_WINNT  >= 0x0501  //Windows XP
	SYSTEM_INFO si;

	GetNativeSystemInfo(&si);
	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM) {
		return PROCESS_X86;
	}
#endif
#if NTDDI_VERSION >= 0x05010200 //XP SP2
	BOOL Wow64;
	ok = IsWow64Process(hProcess, &Wow64);
	if (ok) {
		return Wow64 ? PROCESS_X86 : PROCESS_X64;
	}
#endif
	return PROCESS_UNKNOWN;
}