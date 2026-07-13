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

#define RSP_MENU_TEST 0x0
#define RSP_MENU_MTX_TOP 0x20
#define RSP_MENU_MTX_TOP_UV 0x40
#define RSP_MENU_CAP_VECTORS 0x60
#define RSP_MENU_MTX_STACK_PTR 0x70
#define RSP_MENU_MTX_STACK_SIZE 0x74
#define RSP_MENU_MTX_STACK_MAX 0x75
#define RSP_MENU_MTX_STACK_PTR_UV 0x78
#define RSP_MENU_MTX_STACK_SIZE_UV 0x7c
#define RSP_MENU_MTX_STACK_MAX_UV 0x7d
#define RSP_MENU_ATTR_FLAGS 0x7e
#define RSP_MENU_VTX_FX_FLAGS 0x7f
#define RSP_MENU_VIEWPORT 0x80
#define RSP_MENU_VERTEX_POINTS 0x90
#define RSP_MENU_NEXT_VERTEX 0xb0

#endif
