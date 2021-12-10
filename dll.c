#define WINVER _WIN32_WINNT_WINXP

#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <shlwapi.h>
#include <d3d9.h>
#include "write.h"
#include "dll.h"

#define FRAME_COUNT_ADDR  0x0055D1D4

#define DMRLOAD_FUNC_ADDR 0x004783A0

enum { NOT_EFFECT = 0, IS_EFFECT = 1 };

static __attribute__((stdcall)) int (*gameupdate_orig)() = NULL;

static __attribute__((stdcall)) void (*dmrloadfunc)() = NULL;

static __attribute__((stdcall)) IDirect3D9* (*orig_Direct3DCreate9)(UINT) = NULL;
static ID3D9_vtbl old_id3d9_vtbl;
static IDirect3D9* mbaa_id3d9 = NULL;
static ID3DDev9_vtbl old_id3ddev9_vtbl;
static IDirect3DDevice9* mbaa_id3ddev9 = NULL;

static int pause_enabled = 0;
static int pause_released = 1;
static int slow_enabled = 0;
static int slow_released = 1;
static int load_released = 1;
static int inputfocus_enabled = 1;
static int inputfocus_released = 1;
static int box_enabled = 0;
static int box_released = 1;
static int hurtb_enabled = 1;
//static int hurtb_released = 1;
static int hitb_enabled = 1;
//static int hitb_released = 1;
//static int otherb_enabled = 1;
//static int otherb_released = 1;
static int p1box_enabled = 1;
//static int p1box_released = 1;
static int p2box_enabled = 1;
//static int p2box_released = 1;
static int efbox_enabled = 1;
//static int efbox_released = 1;

static void failure(const char* error)
{
	MessageBoxA(0, error, "framestep error", MB_OK | MB_ICONEXCLAMATION);
	ExitProcess(1);
}

#define DRAW_D3D_PRIMITIVE(device, vertex_obj_ptr, vertex_buf_ptr,                          \
                           vertices, vertex_count, prim_count, prim_type)                   \
do {                                                                                        \
	(vertex_obj_ptr)->lpVtbl->Lock((vertex_obj_ptr), 0, (vertex_count)*sizeof(D3DVERTEX),   \
	                               &(vertex_buf_ptr), 0);                                   \
	memcpy((vertex_buf_ptr), (vertices), (vertex_count)*sizeof(D3DVERTEX));                 \
	(vertex_obj_ptr)->lpVtbl->Unlock((vertex_obj_ptr));                                     \
	(device)->lpVtbl->SetStreamSource((device), 0, (vertex_obj_ptr), 0, sizeof(D3DVERTEX)); \
	(device)->lpVtbl->DrawPrimitive((device), (prim_type), 0, (prim_count));                \
} while (0)

static void draw_rect(IDirect3DDevice9* device, RECT* rect, DWORD color)
{
	DWORD trans = 0x30000000;
	D3DVERTEX vertices[6] = { { rect->left,   rect->top,    0, 1.0f, trans|color },
	                          { rect->right,  rect->top,    0, 1.0f, trans|color },
	                          { rect->right,  rect->bottom, 0, 1.0f, trans|color },
	                          { rect->left,   rect->bottom, 0, 1.0f, trans|color },
	                          { rect->left,   rect->top,    0, 1.0f, trans|color },
	                          { rect->right,  rect->bottom, 0, 1.0f, trans|color } };
	LPDIRECT3DVERTEXBUFFER9 vertex_obj_ptr = NULL;
	void* vertex_buf_ptr = NULL;
	device->lpVtbl->CreateVertexBuffer(device, 6*sizeof(D3DVERTEX), 0,
		D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vertex_obj_ptr, NULL);
	
	DRAW_D3D_PRIMITIVE(device, vertex_obj_ptr, vertex_buf_ptr, vertices, 6, 2, D3DPT_TRIANGLELIST);
	
	trans = 0xFF000000;
	int i;
	for (i = 0; i < 5; i++)	vertices[i].color = trans | color;
	DRAW_D3D_PRIMITIVE(device, vertex_obj_ptr, vertex_buf_ptr, vertices, 5, 4, D3DPT_LINESTRIP);
	
	vertex_obj_ptr->lpVtbl->Release(vertex_obj_ptr);
	return;
}

static void draw_origin(IDirect3DDevice9* device, RECT* rect, DWORD color)
{
	DWORD trans = 0xFF000000;
	int center = (rect->left+rect->right)/2;
	D3DVERTEX vertices[4] = { { rect->left,  rect->bottom, 0, 1.0f, trans|color },
	                          { rect->right, rect->bottom, 0, 1.0f, trans|color },
	                          { center,      rect->bottom, 0, 1.0f, trans|color },
							  { center,      rect->top,    0, 1.0f, trans|color } };
	LPDIRECT3DVERTEXBUFFER9 vertex_obj_ptr = NULL;
	void* vertex_buf_ptr = NULL;
	device->lpVtbl->CreateVertexBuffer(device, 4*sizeof(D3DVERTEX), 0,
		D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vertex_obj_ptr, NULL);
	
	DRAW_D3D_PRIMITIVE(device, vertex_obj_ptr, vertex_buf_ptr, vertices, 4, 2, D3DPT_LINELIST);
	
	vertex_obj_ptr->lpVtbl->Release(vertex_obj_ptr);
	return;
}

#undef DRAW_D3D_PRIMITIVE

static inline void* get_anim_ptr(MBAA_entity* ent, unsigned int seq)
{
	if (ent) {
		void* lvl0 = ent->unk_animdata;
		if (lvl0) {
			unsigned int lvl1 = *(unsigned int*)lvl0;
			if (lvl1) {
				unsigned int lvl2 = *(unsigned int*)(lvl1+4);
				if (lvl2) {
					unsigned int* anim_ptrs = *(unsigned int**)(lvl2+4);
					if (anim_ptrs) return (void*)anim_ptrs[seq];
				}
			}
		}
	}
	return NULL;
}

static inline MBAA_mvst* get_state_ptr(void* anim_ptr, unsigned int state)
{
	if (anim_ptr) {
		unsigned int lvl0 = *(unsigned int*)((unsigned int)anim_ptr+0x34);
		if (lvl0) {
			struct {char d[0x54];}* statetbl = *(void**)(lvl0+4);
			if (statetbl) return (MBAA_mvst*)&statetbl[state];
		}
	}
	return NULL;
}

static inline MBAA_mvst* state_ptr_from_entity(MBAA_entity* ent)
{
	return get_state_ptr(get_anim_ptr(ent, ent->seq), ent->st);
}

static void draw_boxes(IDirect3DDevice9* device, MBAA_entity* ent, int is_effect)
{
	signed int camera_x = *(signed int*)0x55DEC4;
	signed int camera_y = *(signed int*)0x55DEC8;
	float      camera_zoom = *(float*)0x54EB70;
	float      mbaa_width = (float)*(unsigned int*)0x54D048;
	float      mbaa_height = (float)*(unsigned int*)0x54D04C;
	float      mbaa_ar_setting = *(unsigned int*)0x554138;
	
	signed int x = ent->xpos;
	signed int y = ent->ypos;
	MBAA_mvst* state_ptr = state_ptr_from_entity(ent);
	unsigned int box_center_x = (unsigned int)((mbaa_width/2)+(mbaa_width/640)*(((x-camera_x)*camera_zoom)/128.));
	unsigned int box_base_y = (unsigned int)(mbaa_height+(mbaa_height/480)*(((y-camera_y)*camera_zoom/128.)-49));
	
	if (state_ptr) {
		int i;
		DWORD color;
		RECT rect;
		signed int flip = 1;
		if (ent->facing_left) flip = -1;
		if (hurtb_enabled && state_ptr->hurtb_table) {
			for (i = 0; i < state_ptr->hurtb_count; i++) {
				if (state_ptr->hurtb_table[i]) {
					if (i == 0) color = 0xD0D0D0;
					else if (i == 9 || i == 10) color = 0xFF00FF;
					else if (i == 11) color = 0xFFFF00;
					else if (i > 11) color = 0x0000FF;
					else color = 0x00FF00;
					rect.left = box_center_x + (mbaa_width/640)*flip*camera_zoom*2*state_ptr->hurtb_table[i]->left;
					rect.right = box_center_x + (mbaa_width/640)*flip*camera_zoom*2*state_ptr->hurtb_table[i]->right;
					rect.top = box_base_y + (mbaa_height/480)*camera_zoom*(2*state_ptr->hurtb_table[i]->top);
					rect.bottom = box_base_y + (mbaa_height/480)*camera_zoom*2*state_ptr->hurtb_table[i]->bottom;
					draw_rect(device, &rect, color);
				}
			}
		}
		
		if (hitb_enabled && state_ptr->hitb_table) {
			color = 0xFF0000;
			for (i = 0; i < state_ptr->hitb_count; i++) {
				if (state_ptr->hitb_table[i]) {
					rect.left = box_center_x + (mbaa_width/640)*flip*camera_zoom*2*state_ptr->hitb_table[i]->left;
					rect.right = box_center_x + (mbaa_width/640)*flip*camera_zoom*(2*state_ptr->hitb_table[i]->right-1);
					rect.top = box_base_y + (mbaa_height/480)*camera_zoom*(2*state_ptr->hitb_table[i]->top+1);
					rect.bottom = box_base_y + (mbaa_height/480)*camera_zoom*(2*state_ptr->hitb_table[i]->bottom);
					draw_rect(device, &rect, color);
				}
			}
		}

		if (!is_effect) {
			rect.left = box_center_x-(mbaa_width/640)*camera_zoom*5;
			rect.right = box_center_x+(mbaa_width/640)*camera_zoom*5;
			rect.top = box_base_y-(mbaa_width/640)*camera_zoom*5;
			rect.bottom = box_base_y;
			draw_origin(device, &rect, 0x00FFFF);
		}
	}
	return;
}

static HRESULT __attribute__((stdcall)) my_Present(IDirect3DDevice9* this, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
	HRESULT retval = D3DERR_INVALIDCALL;
	if (box_enabled) {
		this->lpVtbl->SetTexture(this, 0, NULL);
		this->lpVtbl->SetFVF(this, D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
		this->lpVtbl->SetRenderState(this, D3DRS_ALPHABLENDENABLE, TRUE);
		this->lpVtbl->SetRenderState(this, D3DRS_BLENDOP, D3DBLENDOP_ADD);
		this->lpVtbl->SetRenderState(this, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		this->lpVtbl->SetRenderState(this, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		
		this->lpVtbl->BeginScene(this);
		
		struct {char d[0xAFC];}* pobj_tbl = (void*)0x555134;
		if (p2box_enabled) draw_boxes(this, (MBAA_entity*)&pobj_tbl[3], NOT_EFFECT); //P4
		if (p1box_enabled) draw_boxes(this, (MBAA_entity*)&pobj_tbl[2], NOT_EFFECT); //P3
		if (p2box_enabled) draw_boxes(this, (MBAA_entity*)&pobj_tbl[1], NOT_EFFECT); //P2
		if (p1box_enabled) draw_boxes(this, (MBAA_entity*)&pobj_tbl[0], NOT_EFFECT); //P1
		
		struct {char d[0x33C];}* efobj_tbl = (void*)0x67BDE8;
		unsigned int i;
		for (i = 0; i < 1000; i++) {
			unsigned int ef_active = *(unsigned int*)&efobj_tbl[i];
			if (ef_active) {
				MBAA_entity* ef_obj = (MBAA_entity*)(&efobj_tbl[i].d[4]);
				//byte at 0x1D should be set only for components of the heat/max trailing effect
				if (!ef_obj->is_heat_eff && efbox_enabled) draw_boxes(this, ef_obj, IS_EFFECT);
			}
		}
		
		this->lpVtbl->EndScene(this);
	}
	retval = old_id3ddev9_vtbl.Present(this, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
	
	return retval;
}

static __attribute__((stdcall)) HRESULT my_CreateDevice(IDirect3D9* this, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
{
	HRESULT retval = old_id3d9_vtbl.CreateDevice(this, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
	
	memcpy(&old_id3ddev9_vtbl, (*ppReturnedDeviceInterface)->lpVtbl, sizeof(ID3DDev9_vtbl));
	(*ppReturnedDeviceInterface)->lpVtbl->Present = (void*)my_Present;
	mbaa_id3ddev9 = *ppReturnedDeviceInterface;
	
	return retval;
}

static __attribute__((stdcall)) IDirect3D9* WINAPI my_Direct3DCreate9(UINT SDKVersion)
{
	IDirect3D9* retval = orig_Direct3DCreate9(SDKVersion);
	
	DWORD old;
	memcpy(&old_id3d9_vtbl, retval->lpVtbl, sizeof(ID3D9_vtbl));
	VirtualProtect(&old_id3d9_vtbl, sizeof(ID3D9_vtbl), PAGE_EXECUTE_READWRITE, &old);
	
	VirtualProtect(retval->lpVtbl, sizeof(ID3D9_vtbl), PAGE_EXECUTE_READWRITE, &old);
	retval->lpVtbl->CreateDevice = (void*)my_CreateDevice;
	mbaa_id3d9 = retval;

	return retval;
}

static inline int game_is_active()
{
	HANDLE active = GetActiveWindow();
	HANDLE foreground = GetForegroundWindow();
	return !memcmp(&active, &foreground, sizeof(HANDLE));
}

static __attribute__((stdcall)) VOID WINAPI my_Sleep(DWORD ms)
{
	//IDirect3DDevice9 vtable gets reset occasionally for some reason, so here's a lazy fix
	if (mbaa_id3ddev9) mbaa_id3ddev9->lpVtbl->Present = (void*)my_Present;
	Sleep(ms);
	return;
}

static __attribute__((stdcall)) int my_gameupdate()
{
	#define TOGGLE_IF_PRESSED(enabled, released, keycode)     \
	do {                                                      \
		if (!GetAsyncKeyState((keycode))) (released) = 1;     \
		else if (GetAsyncKeyState((keycode)) && (released)) { \
			(enabled) ^= 1;                                   \
			(released) = 0;                                   \
		}                                                     \
	} while (0)
	
	if (game_is_active() || !inputfocus_enabled) {
		if (GetAsyncKeyState(VK_CONTROL)) {
			TOGGLE_IF_PRESSED(inputfocus_enabled, inputfocus_released, 0x49); //I
			TOGGLE_IF_PRESSED(slow_enabled, slow_released, 0x4F); //O
			if (!GetAsyncKeyState(0x4C)) load_released = 1; //L
			else if (GetAsyncKeyState(0x4C) && load_released) {
				load_released = 0;
				if (dmrloadfunc) (*dmrloadfunc)();
			}
		}
		
		TOGGLE_IF_PRESSED(pause_enabled, pause_released, 0x50); //P
		if (pause_enabled) {
			while ((!(game_is_active() || !inputfocus_enabled) ||
			        !(GetAsyncKeyState(VK_SPACE) & 0x1)) && pause_enabled) {
				MSG msg;
				PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
				Sleep(1);
				
				if (game_is_active() || !inputfocus_enabled)
					//TOGGLE_IF_PRESSED(box_enabled, box_released, 0x48); //H
					TOGGLE_IF_PRESSED(pause_enabled, pause_released, 0x50); //P
			}
		}
		
		TOGGLE_IF_PRESSED(box_enabled, box_released, 0x48); //H
		
		if (box_enabled) {
			/*
			TOGGLE_IF_PRESSED(hurtb_enabled, hurtb_released, 0x54); //T
			TOGGLE_IF_PRESSED(hitb_enabled, hitb_released, 0x59); //Y
			TOGGLE_IF_PRESSED(otherb_enabled, otherb_released, 0x55); //U
			TOGGLE_IF_PRESSED(p1box_enabled, p1box_released, 0x31); //1
			TOGGLE_IF_PRESSED(p2box_enabled, p2box_released, 0x32); //2
			TOGGLE_IF_PRESSED(efbox_enabled, efbox_released, 0x33); //3
			*/
		}
	}
	
	if (slow_enabled) Sleep(50);
	
	#undef TOGGLE_IF_PRESSED
	return gameupdate_orig();
}

static int scan(BYTE* target, int size, DWORD* pos, DWORD max)
{
	BYTE* ptr = (BYTE*)*pos;
	int offset = 0;
	while (ptr + offset < (BYTE*)max) {
		if (ptr[offset] == target[offset]) {
			offset++;
			if (offset == size) {
				*pos = (DWORD)ptr;
				return 1;
			}
		} else {
			offset = 0;
			ptr++;
		}
	}
	return 0;
}

BOOL APIENTRY DllMain(HANDLE hMod, DWORD reason, LPVOID nothing)
{
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)0x400000;
	DWORD textSection = (DWORD)NULL, textSectionSize = 0;
	//DWORD rdataSection = (DWORD)NULL, rdataSectionSize = 0;
	WORD fail = 0;

	switch (reason) {
	case DLL_PROCESS_DETACH:
		break;
	case DLL_PROCESS_ATTACH: ;
		HMODULE hdll;
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_PIN, "framestep", &hdll);
		
		if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
			PIMAGE_NT_HEADERS32 pNTHeader = (PIMAGE_NT_HEADERS32)((DWORD)dosHeader + dosHeader->e_lfanew);
			WORD numOfSections = pNTHeader->FileHeader.NumberOfSections;
			PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION(pNTHeader);
			DWORD i;
			for (i = 0; i < numOfSections; i++) {
				if (!strcmp(".text", (char*)sections[i].Name)) {
					textSection = (DWORD)dosHeader + sections[i].VirtualAddress;
					textSectionSize = sections[i].SizeOfRawData;
				} else if (!strcmp(".rdata", (char*)sections[i].Name)) {
					//rdataSection = (DWORD)dosHeader + sections[i].VirtualAddress;
					//rdataSectionSize = sections[i].SizeOfRawData;
				}
			}
		} else fail = 1;
		
		if (fail) failure("Could not determine sections of executable.");
		
		//replace a jz with jmp so that the initial dialog box is submitted instantly
		BYTE jz_opcode_target[10] = {0x74, 0x11, 0x83, 0xE8, 0x01, 0x75, 0x08, 0x50, 0x56, 0xFF};
		DWORD jz_opcode_pos = textSection;
		if (scan(jz_opcode_target, sizeof(jz_opcode_target), &jz_opcode_pos, textSection + textSectionSize)) {
			write_byte(jz_opcode_pos, 0xEB);
			write_byte(jz_opcode_pos-9, 0xEB); //replace the jnz slightly before it too
			write_byte(jz_opcode_pos-8, 0xE); //and modify the offset it jumps to
		}
		
		//hook the call to Sleep
		BYTE sleep_target[16] = {0x35, 0xFF, 0xFF, 0x83, 0xC4, 0x04, 0x85, 0xC0, 0x74, 0x04, 0x6A, 0x08, 0xEB, 0x02, 0x6A, 0x02};
		DWORD sleep_pos = textSection;
		if (scan(sleep_target, sizeof(sleep_target), &sleep_pos, textSection + textSectionSize)) {
			sleep_pos += sizeof(sleep_target);
			write_byte(sleep_pos, 0xE8); sleep_pos++;
			write_call(sleep_pos, (void*)my_Sleep); sleep_pos += 4;
			write_byte(sleep_pos, 0x90);
		}
		
		//hook frame update call
		DWORD update_pos = 0x40D350;
		write_byte(update_pos, 0xE8); update_pos++;
		int offset = *(unsigned int*)update_pos;
		gameupdate_orig = (void*)((update_pos+4) + offset);
		write_call(update_pos, (void*)my_gameupdate);
		
		//find the function that loads dummy recordings
		BYTE dmrload_target[49] = {0x6A, 0xFF, 0x68, 0xE8, 0x8B, 0x51, 0x00, 0x64, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x50, 0x83, 0xEC, 0x24, 0xA1, 0x58, 0xB4, 0x54, 0x00, 0x33, 0xC4, 0x89, 0x44, 0x24, 0x20, 0x56, 0x57, 0xA1, 0x58, 0xB4, 0x54, 0x00, 0x33, 0xC4, 0x50, 0x8D, 0x44, 0x24, 0x30, 0x64, 0xA3, 0x00, 0x00, 0x00, 0x00, 0x8D};
		dmrloadfunc = (void*)textSection;
		if (!scan(dmrload_target, sizeof(dmrload_target), (DWORD*)&dmrloadfunc, textSection + textSectionSize)) dmrloadfunc = NULL;
		
		//replace pointer to Direct3DCreate9
		orig_Direct3DCreate9 = (void*)*(unsigned int*)(*(unsigned int*)(0x4DE550));
		DWORD old;
		VirtualProtect(&orig_Direct3DCreate9, 4, PAGE_EXECUTE_READWRITE, &old);
		write_ptr((*(unsigned int*)0x4DE550), (void*)my_Direct3DCreate9);
		
		//patch jnz to jmp to allow multiple game instances
		DWORD die_if_open_jnz = 0x40D25A;
		write_byte(die_if_open_jnz, 0xEB);
		break;
	}
	
	return TRUE;
}
