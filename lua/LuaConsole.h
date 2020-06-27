#ifndef LUA_CONSOLE_H
#define LUA_CONSOLE_H

#include "luaDefine.h"

#ifdef LUA_CONSOLE


#include "../r4300/r4300.h"

//識別子衝突対策
//本当はヘッダ分割すべきか
#ifndef LUACONSOLE_H_NOINCLUDE_WINDOWS_H
#include <Windows.h>
bool IsLuaConsoleMessage(MSG* msg);
void InitializeLuaDC(HWND mainWnd);
void NewLuaScript(void(*callback)());
void LuaWindowMessage(HWND, UINT, WPARAM, LPARAM);
#endif
void LuaReload();
void CloseAllLuaScript();
void AtUpdateScreenLuaCallback();
void AtVILuaCallback();
void GetLuaMessage();
void AtInputLuaCallback(int n);
void AtIntervalLuaCallback();

void LuaBreakpointSyncPure();
void LuaBreakpointSyncInterp();
void LuaDCUpdate(int redraw);
void LuaTraceLoggingPure();
void LuaTraceLoggingInterpOps();
void LuaTraceLogState();

//無理やりinline関数に
namespace LuaEngine {
void PCBreak(void*,unsigned long);
extern void *pcBreakMap_[0x800000/4];
}

inline void LuaPCBreakPure(){
	void *p = LuaEngine::pcBreakMap_[(interp_addr&0x7FFFFF)>>2];
	if(p)LuaEngine::PCBreak(p, interp_addr);
}
inline void LuaPCBreakInterp(){
	void *p = LuaEngine::pcBreakMap_[(PC->addr&0x7FFFFF)>>2];
	if(p)LuaEngine::PCBreak(p, PC->addr);
}

extern unsigned long lastInputLua[4];
extern unsigned long rewriteInputLua[4];
extern bool rewriteInputFlagLua[4];
extern bool enableTraceLog;
extern bool traceLogMode;
extern bool enablePCBreak;
extern bool maximumSpeedMode;



#endif

#endif
