#ifndef		__DRAW_H_INCLUDE__
#define		__DRAW_H_INCLUDE__
#include	"stdafx.h"	//MFCに依存するのはCDCを使用する部分です。（描画関連)
#include	"work.h"	
//------------------------------------------------------------------------------------------------
//	defines
//------------------------------------------------------------------------------------------------
void init_draw(void);
void work_draw(CDC *pdc , work &w);
//------------------------------------------------------------------------------------------------
#endif	
