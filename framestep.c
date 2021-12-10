#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <shlwapi.h>

static int getbase(HANDLE hnd, DWORD* address, WORD* orig_code)
{
	DWORD peAddress = 0x400000;
	IMAGE_DOS_HEADER dosHeader;
	IMAGE_NT_HEADERS32 NTHeader;
	
	ReadProcessMemory(hnd, (LPCVOID)peAddress, &dosHeader, sizeof(dosHeader), NULL);
	ReadProcessMemory(hnd, (LPCVOID)(peAddress + dosHeader.e_lfanew), &NTHeader, sizeof(NTHeader), NULL);
	
	*address = peAddress + NTHeader.OptionalHeader.AddressOfEntryPoint;
	ReadProcessMemory(hnd, (LPCVOID)*address, orig_code, 2, NULL);
	
	return 1;
}

static int hookDLL(const WCHAR* dll_path, const PROCESS_INFORMATION* pi)
{
	HMODULE hKernel32 = GetModuleHandle("Kernel32");
	LPTHREAD_START_ROUTINE pLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
	
	void* dll_addr = VirtualAllocEx(pi->hProcess, 0, 2048, MEM_COMMIT, PAGE_READWRITE);
	WriteProcessMemory(pi->hProcess, dll_addr, dll_path, 2048, 0);
	
	HANDLE hThread = CreateRemoteThread(pi->hProcess, 0, 0, pLoadLibrary, dll_addr, 0, NULL);
	if (!hThread) {
		VirtualFreeEx(pi->hProcess, dll_addr, 0, MEM_RELEASE);
		TerminateProcess(pi->hProcess, -1);
		printf("Could not create remote thread.\n");
		return 0;
	}
	
	HMODULE hookedDLL;
	HMODULE* addr = &hookedDLL;
	
	WaitForSingleObject(hThread, INFINITE);
	GetExitCodeThread(hThread, (DWORD*)addr);
	CloseHandle(hThread);
	
	VirtualFreeEx(pi->hProcess, dll_addr, 0, MEM_RELEASE);
	
	if (!hookedDLL) {
		TerminateProcess(pi->hProcess, -1);
		printf("Could not hook dll after all.\n");
		return 0;
	}
	
	return 1;
}

static int hook(PROCESS_INFORMATION* pi)
{
	unsigned short current_dir[1024];
	unsigned short exe_path[1024];
	unsigned short ini_path[1024];
	unsigned short dll_path[1024];
	STARTUPINFOW si;
	WIN32_FILE_ATTRIBUTE_DATA exe_info, dat_info;
	
	memset(&si, 0, sizeof(si));
	memset(pi, 0, sizeof(*pi));
	si.cb = sizeof(si);
	
	GetModuleFileNameW(0, current_dir, 1010);
	PathRemoveFileSpecW(current_dir);
	
	memcpy(dll_path, current_dir, 2048);
	lstrcatW(dll_path, L"\\framestep.dll");
	
	//memcpy(ini_path, current_dir, 2048);
	//lstrcatW(ini_path, L"\\framestep.ini");
	
	GetPrivateProfileStringW( L"MAIN", L"mbaaccPath", L"", exe_path, 2040, ini_path);
	
	if (lstrlenW(exe_path) < 2) {
		memcpy(exe_path, current_dir, 2048);
		lstrcatW(exe_path, L"\\MBAA.exe");
	} else {
		unsigned short tmp_path[1024];
		PathCombineW(tmp_path, current_dir, exe_path);
		PathCanonicalizeW(current_dir, tmp_path);
		PathRemoveFileSpecW(current_dir);
	}
	
	if (GetFileAttributesExW(exe_path, GetFileExInfoStandard, &exe_info) == 0) {
		printf("Could not find MBAACC executable. framestep.exe should be contained in the same folder as MBAA.exe.\n");
		return 0;
	}
	
	if (GetFileAttributesExW(dll_path, GetFileExInfoStandard, &dat_info) == 0) {
		printf("Could not find framestep.dll.\n");
		return 0;
	}
	
	if (!CreateProcessW(exe_path, 0, 0, 0, TRUE, CREATE_SUSPENDED, NULL, current_dir, &si, pi)) {
		printf("Could not create process.\n");
		return 0;
	}
	
	WORD lock_code = 0xFEEB;
	WORD orig_code;
	DWORD address;
	
	if (!getbase(pi->hProcess, &address, &orig_code)) {
		printf("Could not find entry point.\n");
		return 0;
	}
	
	WriteProcessMemory(pi->hProcess, (void*)address, (char*)&lock_code, 2, 0);
	
	CONTEXT ct;
	ct.ContextFlags = CONTEXT_CONTROL;
	int tries = 0;
	do {
		retry:
		ResumeThread(pi->hThread);
		Sleep(10);
		SuspendThread(pi->hThread);
		
		if (!GetThreadContext(pi->hThread, &ct)) {
			if (tries++ < 500)
				goto retry;
			Sleep(100);
			TerminateProcess(pi->hProcess, -1);
			printf("Could not get thread context.\n");
			return 0;
		}
		//printf("ct.Eip: %.8X\n", ct.Eip);
	} while (ct.Eip != address);
	
	if (!hookDLL(dll_path, pi)) return 0;
	
	WriteProcessMemory(pi->hProcess, (void*)address, (char*)&orig_code, 2, 0);
	FlushInstructionCache(pi->hProcess, (void*)address, 2);
	
	return 1;
}

int main(int argc, char** argv)
{	
	PROCESS_INFORMATION pi;
	if (!hook(&pi)) {
		printf("Could not hook into MBAA.exe. Do you have GameGuard or something running?\n\n");
		return 0;
	}
	
	ResumeThread(pi.hThread);

	return 0;
}
