#ifndef __RSP_MENU_H__
#define __RSP_MENU_H__

#define RSP_MENU_MenuCmd_MoveTo 0
#define RSP_MENU_MenuCmd_LineTo 1
#define RSP_MENU_MenuCmd_Mtx 2
#define RSP_MENU_MenuCmd_MtxPop 3
#define RSP_MENU_MenuCmd_SetStack 4
#define RSP_MENU_MenuCmd_SetAttrFlags 5
#define RSP_MENU_MenuCmd_SetViewport 6
#define RSP_MENU_MenuCmd_SetUVFx 7
#define RSP_MENU_MenuCmd_VTX 8

#define RSP_MENU_TEST 0x0
#define RSP_MENU_MTX_TOP 0x20
#define RSP_MENU_MTX_STACK_PTR 0x40
#define RSP_MENU_MTX_STACK_SIZE 0x44
#define RSP_MENU_MTX_STACK_MAX 0x45
#define RSP_MENU_MTX_TOP_UV 0x50
#define RSP_MENU_MTX_STACK_PTR_UV 0x70
#define RSP_MENU_MTX_STACK_SIZE_UV 0x74
#define RSP_MENU_MTX_STACK_MAX_UV 0x75
#define RSP_MENU_CAP_VECTORS 0x80
#define RSP_MENU_ATTR_FLAGS 0x90
#define RSP_MENU_VTX_FX_FLAGS 0x91
#define RSP_MENU_VIEWPORT 0x94
#define RSP_MENU_VERTEX_POINTS 0xa0
#define RSP_MENU_NEXT_VERTEX 0xc0
#define RSP_MENU_RDPQ_TRI_DATA 0xd0

#endif
