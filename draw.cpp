//================================================================================================
//================================================================================================
#include "stdafx.h"
#include "commonmodule.h"
#include "draw.h"
#include "math.h"
//================================================================================================
//	ワーク実体をここにおくようにして、管理もここで行います。
//================================================================================================
//------------------------------------------------------------------------------------------------
//	defines
//------------------------------------------------------------------------------------------------
#define	DRAW_SIZE			700		//ワークの描画大きさです。
#define	DRAW_CENT_X			500		//ワーク描画中心X
#define	DRAW_CENT_Y			360		//ワーク描画中心X
#define	DRAW_XZ_CENT_Y		850		//XZ描画中心
#define	DRAW_XZ_WIDTH		50		//ワーク厚み描画用

//------------------------------------------------------------------------------------------------
//	global
//------------------------------------------------------------------------------------------------
static int		_nm_per_pix;		// 1 pixel 当たりの長さ
static int		_nm_per_pix_xz;		// 1 pixel 当たりの長さ(XZウィンドウ用)


const	static	POINT	_draw_cent			=	{	DRAW_CENT_X	,	DRAW_CENT_Y		};		//描画の中心
const	static	POINT	_draw_xz_cent[2]	=	{	{	DRAW_CENT_X	,	DRAW_XZ_CENT_Y						}	,
													{	DRAW_CENT_X	,	DRAW_XZ_CENT_Y + DRAW_XZ_WIDTH+30	}
												};		//XZ描画中心
static	RECT	_rectwork;											//描画用矩形
static	RECT	_rect_xz[2];										//

//------------------------------------------------------------------------------------------------
//	sub routine prototype
//------------------------------------------------------------------------------------------------
static	POINT	_rot(POINT	p , POINT org , int	theta );


//------------------------------------------------------------------------------------------------
//	api
//------------------------------------------------------------------------------------------------
void init_draw(void)
{
	//_rectworkは
	//各矩形は描画の際に確定
								//
}	//描画

//描画用の座標変換です.与えられた座標は、
//	X/Y	：	ワーク中心からの
//	Z/W	：	CT上面からの距離（ハイト量)
//	θ	：	原点位置を0°とする
_motion_set &	_motion_for_draw(_motion_set &m)
{
	POINT	_cent = _center(_rectwork);
	int i;for(i=_X ; i < _AXIS_IDX_MAX ; i++ ) {
		switch(i){
		case	_X:				m.s[i].pos	=	(m.s[i].pos / _nm_per_pix)+_cent.x;		break;
		case	_Y:	case _V:	m.s[i].pos	=	(m.s[i].pos / _nm_per_pix)+_cent.y;		break;
//Zは、表示の矩形厚みをワークサイズとなるように
//tokutoku
//一貫性がないか。CT上面からの相対距離のみを記録します。
		case	_Z:	case	_W:
			m.s[i].pos	=	_rect_xz[i-_Z].bottom	-	(m.s[i].pos / _nm_per_pix_xz);	break;
//		case 	_T:				m.s[[i]
		default:;
		}
	}
	//ブレード位置の計算
	

	return m;
}

static __aho;
//ワーク上の軌跡を描画する
void work_draw(CDC *pdc , work &w)
{
	__aho=0;

	COLORREF col =	RGB(255,255,255);		//白のブラシです。
	CBrush	sav_b,b;	{	b.CreateSolidBrush(col);	CBrush *p_sav_b = pdc->SelectObject(&b);	}	//塗りつぶしブラシを設定します

	//-------------------------------------------------------------------------------------------------------------
	//	ワーク自身の描画
	//-------------------------------------------------------------------------------------------------------------

	//-------------------------------------------------------------------------------------------------------------
	//	カットライン描画用
	//-------------------------------------------------------------------------------------------------------------
	//色の定義です。
	const	COLORREF	_red	=	RGB(255,0,0) , 	_blue	=	RGB(0,0,255),	_grn	=	RGB(0,255,0),
							_z_red[2]	= 	{	RGB(250,60,0)	,	RGB(200,150,0)		}
						,	_z_blue[2]	=	{	RGB(0,60,250)	,	RGB(0,150,200)		};


	CPen	penLine[2];	
	penLine[0].CreatePen( PS_SOLID , 2 , RGB(255,0,0));	//Z1は青系
	penLine[1].CreatePen( PS_SOLID , 2 , RGB(96,255,96));	//Z2は緑系

	//-------------------------------------------------------------------------------------------------------------
	//	ワークの描画。
	//-------------------------------------------------------------------------------------------------------------
	const vm_database& _vm	=	w.get_vm();
	{	
		if( _vm.dev.cut_pat == _CUTPAT_ROUND ) {
			_rectwork=_rect( _draw_cent	,	DRAW_SIZE,DRAW_SIZE);	
			_nm_per_pix =_vm.dev.work_size_r /  DRAW_SIZE;	//nm→描画用ピクセル数に変換します。
			pdc->Ellipse(&_rectwork);
		} else {
			_nm_per_pix =_vm.dev.work_size_1 /  DRAW_SIZE;	//nm→描画用ピクセル数に変換します。
			pdc->Rectangle(&_rectwork);	//toku この場合正方形だけになってしまう
		}
		//XZ方向のピクセルサイズを計算します。
		_nm_per_pix_xz	=	(_vm.dev.work_thick	+	_vm.dev.tape_thick) / DRAW_XZ_WIDTH	;
	}
	//-------------------------------------------------------------------------------------------------------------
	//	ブレードの矩形の幅を
	//-------------------------------------------------------------------------------------------------------------
	int	blade_dia[2];
	{
		{int i;for(i=0;i<2;i++)	{	blade_dia[i] = _vm.mac.blade_dia[i] / _nm_per_pix_xz/20;	}	}
	}
	//-------------------------------------------------------------------------------------------------------------
	//	
	//-------------------------------------------------------------------------------------------------------------
#if	1
	{
		const _multi_motion*p =w._line.multi_adr();	//
		_motion_set	m;	interval	l;
		for( l = w._line.get_next_line(l) ; l.valid() ; l=w._line.get_next_line(l) ) {
			//---------------------------------------------------------------------------------
			//	このラインの場合のワーク断面図を書きます。
			//---------------------------------------------------------------------------------
			{
				w._line.get_motion_set(m , l.start);	//このチックのモーションです。
				//Y軸の位置から、ワークの弧の長さを計算します。
				{
					int	_w;
					int i;for(i=0 ; i<2; i++) {
						if( _vm.dev.cut_pat == _CUTPAT_ROUND ) {
							//Y位置がワークの範囲内にあるときだけにします。
							_single_motion &y = (i==0)?m.s[_Y]:m.s[_V];
							double	r = _vm.dev.work_size_r/2;
							if	(f_eq_less(	fabs(y.pos), r) ) {				//yが半径以内であれば
								_w		=	(int)((sqrt( (r*r) - (y.pos * y.pos)) ) / _nm_per_pix ) *	2;
							} else {
								_w		=	(r / _nm_per_pix ) *	2;
							}
						} else {
							_w	=	_vm.dev.work_size_1;
						}
						_rect_xz[i] = _rect( _draw_xz_cent[i] , DRAW_XZ_WIDTH , _w );
						pdc->Rectangle(&_rect_xz[i]);
					}
				}
			}
			//---------------------------------------------------------------------------------
			//	イチラインずつ処理していきます。
			//---------------------------------------------------------------------------------
			interval	line_t(	(p+l.start)->t	,	(p+l.end)->t	);
			int t;	for( t = line_t.start ; t < line_t.end ; t++ ) {			//一ライン分全チックについてモーションを取得していきます。
				w._line.get_motion_set(m , (double)t);	//このチックのモーションです。
#if 1
if( m.flg.cut_z || m.flg.cut_w){
	__aho=1;
}

if( m.flg.ch==2 && m.flg.line==5){
	__aho=__aho*1;
}
if( m.flg.cut_w){
	__aho=1;
}

#endif
				m = _motion_for_draw(m);
				//Z1側切れているときとそうでないときを区別しようかしら
				pdc->SetPixel(m.s[_X].pos , m.s[_Y].pos	,	m.flg.cut_z ? _red:_blue);					//ワークZ1
				pdc->SetPixel(m.s[_X].pos , m.s[_V].pos ,	m.flg.cut_w ? _z_red[1]:_z_blue[1]	);		//ワークZ2

				//XZも書きます。（最初のブレードが入るまでじゃまなので
				if( __aho)
				{
					pdc->SetPixel(m.s[_X].pos	,	m.s[_Z].pos , m.flg.cut_z?_z_red[0]:_z_blue[0]);		//ワークZ1
/*
					//ブレードも書いてみる。
					int r = blade_dia[0]/2;
					RECT	rect_bld = _rect( _pt(m.s[_X].pos , m.s[_Z].pos - r ) , r*2 , r*2);
					pdc->Ellipse(&rect_bld);
//					pdc->Arc(&rect_bld,_pt(0,0),_pt(0,0));
*/
 					pdc->SetPixel(m.s[_X].pos	,	m.s[_W].pos , m.flg.cut_w?_z_red[1]:_z_blue[1]);		//ワークZ1
/*

					//ブレードも書いてみる。
					r = blade_dia[1]/2;
					rect_bld = _rect( _pt(m.s[_X].pos , m.s[_W].pos - r ) , r*2 , r*2);
					pdc->Ellipse(&rect_bld);
//					pdc->Arc(&rect_bld,_pt(0,0),_pt(0,0));
*/


				}
			}
		}
	}
#endif
}
//ワークを再描画する必要があるかどうか
static	bool	_redraw	=false;
bool	work_redraw_req(void)	{	bool r=_redraw;	_redraw=false;	return	r;	}

//------------------------------------------------------------------------------------------------
//	sub routines
//------------------------------------------------------------------------------------------------
static	POINT	_rot(POINT	p , POINT org , int	theta )
{
	double	r = ((double)theta/1000000)*3.1415926535/180;	//ラジアンに変換です
	{p.x -=	org.x;	p.y	-=	org.y;}		//回転の元になる座標を原点相対位置にします。

	//回転させます
	POINT	_p;
	_p.x	=	(int)(((double)p.x*cos(r))	-	((double)p.y*sin(r)));
	_p.y	=	(int)(((double)p.x*sin(r))	+	((double)p.y*cos(r)));

	//回転させた座標を原点
	{_p.x += org.x;	_p.y+=org.y;	}	//原点をオフセットします。

	return _p;
}

//typedef	struct {
//	POINT	Yst;	POINT	Yed;		//Y軸の軌跡
//	POINT	Vst;	POINT	Ved;		//V軸の軌跡
//} CUTLINE_FOR_DRAW;
//
//static	CUTLINE_FOR_DRAW	_cutline_for_draw( int w , int l , int kijun_t )
//{
//	_Assert( l < _work[w].p_cut  , "[work] cutline_for_draw line over(%d/%d)" ,l , _work[w].p_cut);
//	int	nm_per_pix = _work[w].work_size /  DRAW_SIZE;	//nm→描画用ピクセル数に変換します。
//
//	POINT	_cent = _center(_rectwork);
//	CUTLINE_FOR_DRAW draw;
//	{
//		int	xs,xe,y,v;
//		xs			=	((_work[w].cut[l].start_x										-	_work[w].spdl_posx	)	/	nm_per_pix)	+	_cent.x;
//		xe			=	((_work[w].cut[l].end_x											-	_work[w].spdl_posx	)	/	nm_per_pix)	+	_cent.x;
//		y			=	((	(_work[w].cut[l].y			-	_work[w].cut[l].bld_scp	)	-	_work[w].alu_cy		)	/	nm_per_pix)	+	_cent.y;
//		v			=	((	(_work[w].cut[l].bld_scp_v	-	_work[w].cut[l].v		)	-	_work[w].alu_cy		)	/	nm_per_pix)	+	_cent.y + 2;		//Y2はY1と重なるため、少しずらして描画します
//		//各座業をつくってθにならって回転させます。θ軸は原点位置からの相対角度とします
//		{
//			int t = _work[w].cut[l].theta - kijun_t;
//
////_printf("     ***** cutline_for_draw [%d] y=%d  cut_y=%d , bld_scp=%d , alucy=%d nm_pix=%d cent=%d", l , 
////	y,
////	_work[w].cut[l].y ,
////	_work[w].cut[l].bld_scp,
////	_work[w].alu_cy	,
////	nm_per_pix,
////	_cent.y);
////_printf("     ---- cutline_for_draw [%d] t=%d(%d,%d)",l,t ,  _work[w].cut[l].theta - kijun_t);
//			{	draw.Yst=_rot(_pt(xs,y), _cent,t);	draw.Yed = _rot( _pt(xe,y),_cent,t);	}		//Y1ライン
//			{	draw.Vst=_rot(_pt(xs,v), _cent,t);	draw.Ved = _rot( _pt(xe,v),_cent,t);	}		//Y2ライン
//		}
//	}
//	return draw;
//}
////カットライン数の最大(全chでのライン数)
//#define	CUTLINE_MAX			2096
//
//カットライン情報です
//typedef struct {
//	//X軸情報
//	int		start_x;
//	int		end_x;
//	int		spd;
//	//θ軸情報
//	int		theta;
//	//Z軸情報
//	int		y;
//	int		z;
//	int		ct_pos_z;
//	//W軸情報
//	int		v;
//	int		w;
//	int		ct_pos_w;
//	//ヘアライン値
//	int		bld_scp;
//	int		bld_scp_v;
//} CUTLINE;
//
////ワーク本体です。
//typedef	struct {
//	int			work_size;				//ワークの大きさ
//	int			work_thick;				//ワーク厚み
//	int			tape_thick;				//テープ厚み
//	//マシンデータ情報
//	int			alu_cy;					//顕微鏡中心Y
//	int			alu_cx;					//顕微鏡中心Y
//	int			spdl_posx;				//スピンドル中心X
//	int			home_pos_t;				//θ原点位置
//	//
//	int			p_cut;					//カットラインポインタ
//	CUTLINE		cut[CUTLINE_MAX];		//カットライン履歴
//} WORK;
////////////////////////////