//
// SilicaTeam 2.0  @dots_tb @SilicaAndPina ‏
// RePatch -- PATCHING WITH FREEDOM

// Based off ioPlus by @dots_tb: https://github.com/CelesteBlue-dev/PSVita-RE-tools/tree/master/ioPlus/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <vitasdkkern.h>
#include <taihen.h>

#define printf ksceDebugPrintf

static int hooks_uid[3];
static tai_hook_ref_t ref_hooks[3];

static int ksceIoOpen_patched(const char *filename, int flag, SceIoMode mode) {
	int ret = -1, state;
	ENTER_SYSCALL(state);
	SceUID pid = ksceKernelGetProcessId();
	char titleid[32];
	ksceKernelGetProcessTitleId(pid, titleid, sizeof(titleid));
	
	if (memcmp("main", titleid, sizeof("main") - 1)==0) {
		if (memcmp("ux0:/rePatch", filename, sizeof("ux0:/rePatch") - 1)!=0) {
			if ((strstr(filename, "eboot.bin") != NULL) && (strstr(filename, ":/eboot.bin") == NULL)) {
				char new_path[128];
				char *old_path = strchr(filename, '/');
				if (old_path != NULL) {
					old_path++;
					old_path = strchr(old_path, '/');
					if (old_path != NULL) {
						old_path++;
						if(old_path[0] == '/')
							old_path++;
						snprintf(new_path, sizeof(new_path), "ux0:/rePatch/%s",old_path);
						ret = ksceIoOpen(new_path, flag, mode);
					}
				}
			}
		}
	}
	
	if(ret < 0) ret = TAI_CONTINUE(int, ref_hooks[2], filename, flag, mode);
	EXIT_SYSCALL(state);
	return ret;
}

static int ksceIoOpenForPid_patched(SceUID pid, const char *filename, int flag, SceIoMode mode) {
	int ret = -1, state;
	ENTER_SYSCALL(state);
	if (memcmp("app0:", filename, sizeof("app0:") - 1)==0) {
		char new_path[128];
		char titleid[32];
		ret = ksceKernelGetProcessTitleId(pid, titleid, sizeof(titleid));
		if(ret > -1) {
			char *old_path = strchr(filename, ':') + 1;
			if(old_path[0] == '/')
				old_path++;
			snprintf(new_path, sizeof(new_path), "ux0:/rePatch/%s/%s",titleid,old_path);
			ret = ksceIoOpen(new_path, flag, mode);
		}
			
	}
	if(ret < 0) ret = TAI_CONTINUE(int, ref_hooks[0], pid, filename, flag, mode);
	EXIT_SYSCALL(state);
	return ret;
}

static int _sceIoGetstat_patched(const char *file, SceIoStat *stat, int r2) {
	int ret = -1, state;
	SceIoStat k_stat;
	char filename[256];
	ksceKernelStrncpyUserToKernel(&filename, (uintptr_t)file, sizeof(filename));
	ENTER_SYSCALL(state);
	if (memcmp("app0:", filename, sizeof("app0:") - 1)==0) {
		char new_path[128];
		char titleid[32];
		ret = ksceKernelGetProcessTitleId(ksceKernelGetProcessId(), titleid, sizeof(titleid));
		if(ret > -1) {
			char *old_path = strchr(filename, ':') + 1;
			if(old_path[0] == '/')
				old_path++;
			snprintf(new_path, sizeof(new_path), "ux0:/rePatch/%s/%s",titleid,old_path);
			ret = ksceIoGetstat(new_path, &k_stat);
			if(ret > -1) {
				ksceKernelMemcpyKernelToUser((uintptr_t)stat, &k_stat, sizeof(k_stat));
			}
		}

	}
	if(ret < 0) ret = TAI_CONTINUE(int, ref_hooks[1], file, stat, r2);
	EXIT_SYSCALL(state);
	return ret;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	hooks_uid[0] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[0], "SceIofilemgr", TAI_ANY_LIBRARY, 0xC3D34965, ksceIoOpenForPid_patched);
	hooks_uid[1] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[1], "SceIofilemgr", TAI_ANY_LIBRARY, 0x8E7E11F2, _sceIoGetstat_patched);
	hooks_uid[2] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[2], "SceIofilemgr", TAI_ANY_LIBRARY, 0x75192972, ksceIoOpen_patched);
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	if (hooks_uid[0] >= 0) taiHookReleaseForKernel(hooks_uid[0], ref_hooks[0]);   
	if (hooks_uid[1] >= 0) taiHookReleaseForKernel(hooks_uid[1], ref_hooks[1]);   
	if (hooks_uid[2] >= 0) taiHookReleaseForKernel(hooks_uid[2], ref_hooks[2]);  
	return SCE_KERNEL_STOP_SUCCESS;
}