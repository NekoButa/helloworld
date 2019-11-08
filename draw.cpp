//================================================================================================
//================================================================================================
#include "stdafx.h"
#include "commonmodule.h"
#include "draw.h"
#include "math.h"
//================================================================================================
//	���[�N���̂������ɂ����悤�ɂ��āA�Ǘ��������ōs���܂��B
//================================================================================================
//------------------------------------------------------------------------------------------------
//	defines
//------------------------------------------------------------------------------------------------
#define	DRAW_SIZE			700		//���[�N�̕`��傫���ł��B
#define	DRAW_CENT_X			500		//���[�N�`�撆�SX
#define	DRAW_CENT_Y			360		//���[�N�`�撆�SX
#define	DRAW_XZ_CENT_Y		850		//XZ�`�撆�S
#define	DRAW_XZ_WIDTH		50		//���[�N���ݕ`��p

//------------------------------------------------------------------------------------------------
//	global
//------------------------------------------------------------------------------------------------
static int		_nm_per_pix;		// 1 pixel ������̒���
static int		_nm_per_pix_xz;		// 1 pixel ������̒���(XZ�E�B���h�E�p)


const	static	POINT	_draw_cent			=	{	DRAW_CENT_X	,	DRAW_CENT_Y		};		//�`��̒��S
const	static	POINT	_draw_xz_cent[2]	=	{	{	DRAW_CENT_X	,	DRAW_XZ_CENT_Y						}	,
													{	DRAW_CENT_X	,	DRAW_XZ_CENT_Y + DRAW_XZ_WIDTH+30	}
												};		//XZ�`�撆�S
static	RECT	_rectwork;											//�`��p��`
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
	//_rectwork��
	//�e��`�͕`��̍ۂɊm��
								//
}	//�`��

//�`��p�̍��W�ϊ��ł�.�^����ꂽ���W�́A
//	X/Y	�F	���[�N���S�����
//	Z/W	�F	CT��ʂ���̋����i�n�C�g��)
//	��	�F	���_�ʒu��0���Ƃ���
_motion_set &	_motion_for_draw(_motion_set &m)
{
	POINT	_cent = _center(_rectwork);
	int i;for(i=_X ; i < _AXIS_IDX_MAX ; i++ ) {
		switch(i){
		case	_X:				m.s[i].pos	=	(m.s[i].pos / _nm_per_pix)+_cent.x;		break;
		case	_Y:	case _V:	m.s[i].pos	=	(m.s[i].pos / _nm_per_pix)+_cent.y;		break;
//Z�́A�\���̋�`���݂����[�N�T�C�Y�ƂȂ�悤��
//tokutoku
//��ѐ����Ȃ����BCT��ʂ���̑��΋����݂̂��L�^���܂��B
		case	_Z:	case	_W:
			m.s[i].pos	=	_rect_xz[i-_Z].bottom	-	(m.s[i].pos / _nm_per_pix_xz);	break;
//		case 	_T:				m.s[[i]
		default:;
		}
	}
	//�u���[�h�ʒu�̌v�Z
	

	return m;
}

static __aho;
//���[�N��̋O�Ղ�`�悷��
void work_draw(CDC *pdc , work &w)
{
	__aho=0;

	COLORREF col =	RGB(255,255,255);		//���̃u���V�ł��B
	CBrush	sav_b,b;	{	b.CreateSolidBrush(col);	CBrush *p_sav_b = pdc->SelectObject(&b);	}	//�h��Ԃ��u���V��ݒ肵�܂�

	//-------------------------------------------------------------------------------------------------------------
	//	���[�N���g�̕`��
	//-------------------------------------------------------------------------------------------------------------

	//-------------------------------------------------------------------------------------------------------------
	//	�J�b�g���C���`��p
	//-------------------------------------------------------------------------------------------------------------
	//�F�̒�`�ł��B
	const	COLORREF	_red	=	RGB(255,0,0) , 	_blue	=	RGB(0,0,255),	_grn	=	RGB(0,255,0),
							_z_red[2]	= 	{	RGB(250,60,0)	,	RGB(200,150,0)		}
						,	_z_blue[2]	=	{	RGB(0,60,250)	,	RGB(0,150,200)		};


	CPen	penLine[2];	
	penLine[0].CreatePen( PS_SOLID , 2 , RGB(255,0,0));	//Z1�͐n
	penLine[1].CreatePen( PS_SOLID , 2 , RGB(96,255,96));	//Z2�͗Όn

	//-------------------------------------------------------------------------------------------------------------
	//	���[�N�̕`��B
	//-------------------------------------------------------------------------------------------------------------
	const vm_database& _vm	=	w.get_vm();
	{	
		if( _vm.dev.cut_pat == _CUTPAT_ROUND ) {
			_rectwork=_rect( _draw_cent	,	DRAW_SIZE,DRAW_SIZE);	
			_nm_per_pix =_vm.dev.work_size_r /  DRAW_SIZE;	//nm���`��p�s�N�Z�����ɕϊ����܂��B
			pdc->Ellipse(&_rectwork);
		} else {
			_nm_per_pix =_vm.dev.work_size_1 /  DRAW_SIZE;	//nm���`��p�s�N�Z�����ɕϊ����܂��B
			pdc->Rectangle(&_rectwork);	//toku ���̏ꍇ�����`�����ɂȂ��Ă��܂�
		}
		//XZ�����̃s�N�Z���T�C�Y���v�Z���܂��B
		_nm_per_pix_xz	=	(_vm.dev.work_thick	+	_vm.dev.tape_thick) / DRAW_XZ_WIDTH	;
	}
	//-------------------------------------------------------------------------------------------------------------
	//	�u���[�h�̋�`�̕���
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
			//	���̃��C���̏ꍇ�̃��[�N�f�ʐ}�������܂��B
			//---------------------------------------------------------------------------------
			{
				w._line.get_motion_set(m , l.start);	//���̃`�b�N�̃��[�V�����ł��B
				//Y���̈ʒu����A���[�N�̌ʂ̒������v�Z���܂��B
				{
					int	_w;
					int i;for(i=0 ; i<2; i++) {
						if( _vm.dev.cut_pat == _CUTPAT_ROUND ) {
							//Y�ʒu�����[�N�͈͓̔��ɂ���Ƃ������ɂ��܂��B
							_single_motion &y = (i==0)?m.s[_Y]:m.s[_V];
							double	r = _vm.dev.work_size_r/2;
							if	(f_eq_less(	fabs(y.pos), r) ) {				//y�����a�ȓ��ł����
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
			//	�C�`���C�����������Ă����܂��B
			//---------------------------------------------------------------------------------
			interval	line_t(	(p+l.start)->t	,	(p+l.end)->t	);
			int t;	for( t = line_t.start ; t < line_t.end ; t++ ) {			//�ꃉ�C�����S�`�b�N�ɂ��ă��[�V�������擾���Ă����܂��B
				w._line.get_motion_set(m , (double)t);	//���̃`�b�N�̃��[�V�����ł��B
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
				//Z1���؂�Ă���Ƃ��Ƃ����łȂ��Ƃ�����ʂ��悤������
				pdc->SetPixel(m.s[_X].pos , m.s[_Y].pos	,	m.flg.cut_z ? _red:_blue);					//���[�NZ1
				pdc->SetPixel(m.s[_X].pos , m.s[_V].pos ,	m.flg.cut_w ? _z_red[1]:_z_blue[1]	);		//���[�NZ2

				//XZ�������܂��B�i�ŏ��̃u���[�h������܂ł���܂Ȃ̂�
				if( __aho)
				{
					pdc->SetPixel(m.s[_X].pos	,	m.s[_Z].pos , m.flg.cut_z?_z_red[0]:_z_blue[0]);		//���[�NZ1
/*
					//�u���[�h�������Ă݂�B
					int r = blade_dia[0]/2;
					RECT	rect_bld = _rect( _pt(m.s[_X].pos , m.s[_Z].pos - r ) , r*2 , r*2);
					pdc->Ellipse(&rect_bld);
//					pdc->Arc(&rect_bld,_pt(0,0),_pt(0,0));
*/
 					pdc->SetPixel(m.s[_X].pos	,	m.s[_W].pos , m.flg.cut_w?_z_red[1]:_z_blue[1]);		//���[�NZ1
/*

					//�u���[�h�������Ă݂�B
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
//���[�N���ĕ`�悷��K�v�����邩�ǂ���
static	bool	_redraw	=false;
bool	work_redraw_req(void)	{	bool r=_redraw;	_redraw=false;	return	r;	}

//------------------------------------------------------------------------------------------------
//	sub routines
//------------------------------------------------------------------------------------------------
static	POINT	_rot(POINT	p , POINT org , int	theta )
{
	double	r = ((double)theta/1000000)*3.1415926535/180;	//���W�A���ɕϊ��ł�
	{p.x -=	org.x;	p.y	-=	org.y;}		//��]�̌��ɂȂ���W�����_���Έʒu�ɂ��܂��B

	//��]�����܂�
	POINT	_p;
	_p.x	=	(int)(((double)p.x*cos(r))	-	((double)p.y*sin(r)));
	_p.y	=	(int)(((double)p.x*sin(r))	+	((double)p.y*cos(r)));

	//��]���������W�����_
	{_p.x += org.x;	_p.y+=org.y;	}	//���_���I�t�Z�b�g���܂��B

	return _p;
}

//typedef	struct {
//	POINT	Yst;	POINT	Yed;		//Y���̋O��
//	POINT	Vst;	POINT	Ved;		//V���̋O��
//} CUTLINE_FOR_DRAW;
//
//static	CUTLINE_FOR_DRAW	_cutline_for_draw( int w , int l , int kijun_t )
//{
//	_Assert( l < _work[w].p_cut  , "[work] cutline_for_draw line over(%d/%d)" ,l , _work[w].p_cut);
//	int	nm_per_pix = _work[w].work_size /  DRAW_SIZE;	//nm���`��p�s�N�Z�����ɕϊ����܂��B
//
//	POINT	_cent = _center(_rectwork);
//	CUTLINE_FOR_DRAW draw;
//	{
//		int	xs,xe,y,v;
//		xs			=	((_work[w].cut[l].start_x										-	_work[w].spdl_posx	)	/	nm_per_pix)	+	_cent.x;
//		xe			=	((_work[w].cut[l].end_x											-	_work[w].spdl_posx	)	/	nm_per_pix)	+	_cent.x;
//		y			=	((	(_work[w].cut[l].y			-	_work[w].cut[l].bld_scp	)	-	_work[w].alu_cy		)	/	nm_per_pix)	+	_cent.y;
//		v			=	((	(_work[w].cut[l].bld_scp_v	-	_work[w].cut[l].v		)	-	_work[w].alu_cy		)	/	nm_per_pix)	+	_cent.y + 2;		//Y2��Y1�Əd�Ȃ邽�߁A�������炵�ĕ`�悵�܂�
//		//�e���Ƃ������ăƂɂȂ���ĉ�]�����܂��B�Ǝ��͌��_�ʒu����̑��Ίp�x�Ƃ��܂�
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
//			{	draw.Yst=_rot(_pt(xs,y), _cent,t);	draw.Yed = _rot( _pt(xe,y),_cent,t);	}		//Y1���C��
//			{	draw.Vst=_rot(_pt(xs,v), _cent,t);	draw.Ved = _rot( _pt(xe,v),_cent,t);	}		//Y2���C��
//		}
//	}
//	return draw;
//}
////�J�b�g���C�����̍ő�(�Sch�ł̃��C����)
//#define	CUTLINE_MAX			2096
//
//�J�b�g���C�����ł�
//typedef struct {
//	//X�����
//	int		start_x;
//	int		end_x;
//	int		spd;
//	//�Ǝ����
//	int		theta;
//	//Z�����
//	int		y;
//	int		z;
//	int		ct_pos_z;
//	//W�����
//	int		v;
//	int		w;
//	int		ct_pos_w;
//	//�w�A���C���l
//	int		bld_scp;
//	int		bld_scp_v;
//} CUTLINE;
//
////���[�N�{�̂ł��B
//typedef	struct {
//	int			work_size;				//���[�N�̑傫��
//	int			work_thick;				//���[�N����
//	int			tape_thick;				//�e�[�v����
//	//�}�V���f�[�^���
//	int			alu_cy;					//���������SY
//	int			alu_cx;					//���������SY
//	int			spdl_posx;				//�X�s���h�����SX
//	int			home_pos_t;				//�ƌ��_�ʒu
//	//
//	int			p_cut;					//�J�b�g���C���|�C���^
//	CUTLINE		cut[CUTLINE_MAX];		//�J�b�g���C������
//} WORK;
////////////////////////////