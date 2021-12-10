#ifndef FRAMESTEP_CC_DLL_H
#define FRAMESTEP_CC_DLL_H

typedef struct __attribute__((packed)) MBAA_entity {
	char unk0[0xC];
	unsigned int  seq;          //0xC
	unsigned int  st;           //0x10
	char unk14[9];              //0x14
	unsigned char is_heat_eff;  //0x1D
	char unk1E[0xE6];           //0x1E
	unsigned int  xpos;         //0x104
	unsigned int  ypos;         //0x108
	char unk10C[0x204];         //0x10C
	unsigned char facing_left;  //0x310
	char unk311[0x1B];          //0x311
	void*         unk_animdata; //0x32C
} MBAA_entity;

typedef struct __attribute__((packed)) MBAA_box
{
	signed short left;
	signed short top;
	signed short right;
	signed short bottom;
} MBAA_box;

typedef struct __attribute__((packed)) MBAA_mvst
{
	char unk0[0x40];
	unsigned char type0_count;
	unsigned char type1_count;
	unsigned char hurtb_count;
	unsigned char hitb_count;
	MBAA_box** type0_table;
	MBAA_box** type1_table;
	MBAA_box** hurtb_table;
	MBAA_box** hitb_table;
} MBAA_mvst;

typedef struct D3DVERTEX
{
   float x, y, z, rhw;
   DWORD color;
} D3DVERTEX;

typedef struct __attribute__((packed)) ID3D9_vtbl
{
	void* QueryInterface;
	void* AddRef;
	void* Release;
	void* RegisterSoftwareDevice;
	void* GetAdapterCount;
	void* GetAdapterIdentifier;
	void* GetAdapterModeCount;
	void* EnumAdapterModes;
	void* GetAdapterDisplayMode;
	void* CheckDeviceType;
	void* CheckDeviceFormat;
	void* CheckDeviceMultiSampleType;
	void* CheckDepthStencilMatch;
	void* CheckDeviceFormatConversion;
	void* GetDeviceCaps;
	void* GetAdapterMonitor;
	__attribute__((stdcall)) HRESULT (*CreateDevice)(void*, int, int, void*, int, void*, void*);
} ID3D9_vtbl;

typedef struct __attribute__((packed)) ID3DDev9_vtbl
{
    void* QueryInterface;
    void* AddRef;
    void* Release;
    void* TestCooperativeLevel;
    void* GetAvailableTextureMem;
    void* EvictManagedResources;
    void* GetDirect3D;
    void* GetDeviceCaps;
    void* GetDisplayMode;
    void* GetCreationParameters;
    void* SetCursorProperties;
    void* SetCursorPosition;
    void* ShowCursor;
    void* CreateAdditionalSwapChain;
    void* GetSwapChain;
    void* GetNumberOfSwapChains;
    void* Reset;
    __attribute__((stdcall)) HRESULT (*Present)(void*, const void*, const void*, void*, const void*);
    void* GetBackBuffer;
    void* GetRasterStatus;
    void* SetDialogBoxMode;
    void* SetGammaRamp;
    void* GetGammaRamp;
    void* CreateTexture;
    void* CreateVolumeTexture;
    void* CreateCubeTexture;
    void* CreateVertexBuffer;
    void* CreateIndexBuffer;
    void* CreateRenderTarget;
    void* CreateDepthStencilSurface;
    void* UpdateSurface;
    void* UpdateTexture;
    void* GetRenderTargetData;
    void* GetFrontBufferData;
    void* StretchRect;
    void* ColorFill;
    void* CreateOffscreenPlainSurface;
    void* SetRenderTarget;
    void* GetRenderTarget;
    void* SetDepthStencilSurface;
    void* GetDepthStencilSurface;
    __attribute__((stdcall)) HRESULT (*BeginScene)(void*);
    __attribute__((stdcall)) HRESULT (*EndScene)(void*);
    void* Clear;
    void* SetTransform;
    void* GetTransform;
    void* MultiplyTransform;
    void* SetViewport;
    void* GetViewport;
    void* SetMaterial;
    void* GetMaterial;
    void* SetLight;
    void* GetLight;
    void* LightEnable;
    void* GetLightEnable;
    void* SetClipPlane;
    void* GetClipPlane;
    void* SetRenderState;
    void* GetRenderState;
    void* CreateStateBlock;
    void* BeginStateBlock;
    void* EndStateBlock;
    void* SetClipStatus;
    void* GetClipStatus;
    void* GetTexture;
    void* SetTexture;
    void* GetTextureStageState;
    void* SetTextureStageState;
    void* GetSamplerState;
    void* SetSamplerState;
    void* ValidateDevice;
    void* SetPaletteEntries;
    void* GetPaletteEntries;
    void* SetCurrentTexturePalette;
    void* GetCurrentTexturePalette;
    void* SetScissorRect;
    void* GetScissorRect;
    void* SetSoftwareVertexProcessing;
    void* GetSoftwareVertexProcessing;
    void* SetNPatchMode;
    void* GetNPatchMode;
    void* DrawPrimitive;
    void* DrawIndexedPrimitive;
    void* DrawPrimitiveUP;
    void* DrawIndexedPrimitiveUP;
    void* ProcessVertices;
    void* CreateVertexDeclaration;
    void* SetVertexDeclaration;
    void* GetVertexDeclaration;
    void* SetFVF;
    void* GetFVF;
    void* CreateVertexShader;
    void* SetVertexShader;
    void* GetVertexShader;
    void* SetVertexShaderConstantF;
    void* GetVertexShaderConstantF;
    void* SetVertexShaderConstantI;
    void* GetVertexShaderConstantI;
    void* SetVertexShaderConstantB;
    void* GetVertexShaderConstantB;
    void* SetStreamSource;
    void* GetStreamSource;
    void* SetStreamSourceFreq;
    void* GetStreamSourceFreq;
    void* SetIndices;
    void* GetIndices;
    void* CreatePixelShader;
    void* SetPixelShader;
    void* GetPixelShader;
    void* SetPixelShaderConstantF;
    void* GetPixelShaderConstantF;
    void* SetPixelShaderConstantI;
    void* GetPixelShaderConstantI;
    void* SetPixelShaderConstantB;
    void* GetPixelShaderConstantB;
    void* DrawRectPatch;
    void* DrawTriPatch;
    void* DeletePatch;
    void* CreateQuery;
    void* SetConvolutionMonoKernel;
    void* ComposeRects;
    void* PresentEx;
    void* GetGPUThreadPriority;
    void* SetGPUThreadPriority;
    void* WaitForVBlank;
    void* CheckResourceResidency;
    void* SetMaximumFrameLatency;
    void* GetMaximumFrameLatency;
    void* CheckDeviceState;
    void* CreateRenderTargetEx;
    void* CreateOffscreenPlainSurfaceEx;
    void* CreateDepthStencilSurfaceEx;
    void* ResetEx;
    void* GetDisplayModeEx;
} ID3DDev9_vtbl;

typedef struct __attribute__((packed)) mbaa_d3d_ptrs
{
	IDirect3D9* id3d9;
	IDirect3DDevice9* id3ddev9;
} mbaa_d3d_ptrs;

#endif
