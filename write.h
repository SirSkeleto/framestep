#ifndef FRAMESTEP_CC_WRITE_H
#define FRAMESTEP_CC_WRITE_H

static inline void write_code(DWORD address, const char* data, int len) {
	DWORD old, z;
	VirtualProtect((void*)address, len, PAGE_EXECUTE_READWRITE, &old);
	memcpy((char*)address, data, len);
	VirtualProtect((void*)address, len, old, &z);
}

static inline void write_byte(DWORD address, unsigned char value) {
	write_code(address, (const char*)&value, 1);
}
static inline void write_int(DWORD address, int value) {
	write_code(address, (const char*)&value, 4);
}
static inline void write_ptr(DWORD address, const char *value) {
	write_code(address, (const char*)&value, 4);
}
static inline void write_float(DWORD address, float value) {
	write_code(address, (const char*)&value, 4);
}

static inline void write_call(DWORD address, void *ptr) {
	DWORD value = (DWORD)ptr - (address+0x4);
	write_code(address, (const char*)&value, 4);
}

#endif
