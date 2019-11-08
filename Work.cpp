//================================================================================================
//	Work.cpp		OS�ˑ��Ȃ��ɍ�肽���ł��B	���H�L�^���c��
//================================================================================================
#include	"Work.h"
#include	"commonmodule.h"

//#include	"vm_interface.h"				//VM�̋L�^�B
#include	"axis_obj.h"					//axisobj�Ǝ���
#include	"axis_record_read.h"			//


//local_axis���ӎ����邩�ǂ���
#include	"local_axis_interface.h"		//
#include	<stdlib.h>						//abs()�ł�
#include	<math.h>						//sqrt()
#include	<io.h>							//access()
//------------------------------------------------------------------------------------------------
//	defines
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
//	global
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
//	constructor
//------------------------------------------------------------------------------------------------
//�R���X�g���N�^���_�Ńt�@�C�������܂��Ă��Ȃ��Ă��ǂ��ł��B
//make()���s�������_�ŁA�t�@�C�����w������Ă���΂悢�ł��B
//�R���X�g���N�^���_�ł̓t�@�C�������Ȃ��ɂ��Ă����܂��B
work::work()	:	_line(_vm)	{	_filename[0]='\0';	}
//------------------------------------------------------------------------------------------------
//	destructor
//------------------------------------------------------------------------------------------------
work::~work()		{	;	}

//------------------------------------------------------------------------------------------------
//	class imprement
//------------------------------------------------------------------------------------------------
//	X�����쎞�ɐ؂肱��ł���΂���̓J�b�g���C���Ƃ��ċL�^����B
//		�Ăяo�����́A���Ԃ��Ƃ́A�e���̃��[�V�����i�ʒu�Ƒ��x)�𑗂��Ă��܂��B
//	�����Ƃ鎲�́AZ,W,	���܂�
//	MsPosX=CtMsPosX=NRead("MS_POSX");
//	MsPosY=CtMsPosY=NRead("ALU_CY_HI");

//	SpdlPosX=NRead("SPDL_POSX");
//	return (pos*-1)+CutParam->SpdlPosX+CutParam->WorkCenterX;	
//	toku WorkCenterX;		�́ACSP�̑����\��̏ꍇ�̃��[�N���S�ʒu

//ct_posz , blade_scope���A���Ԃɂ���Ă��ƂȂ�ꍇ�ǂ����遏
//	typedef	struct	{	int phase;	double	t;	double	pos;	double	v;	double a;	} _MOTION;		//����u�Ԃ̎��̓����ł��B

//�eZYZ���[�V��������k�؂肱�݃��[�N���S��CT�ʒu����̍��W�ϊ��������̂����C���ɏ������ޕK�v������

//�؂肱��ł�����̂��ǂ������`�F�b�N���܂��B
//	bit	0	Z1���؂肱��ł���		bit	1	Z2���؂肱��ł���B
static int __aho;	//debug
bool	work::_kirikomi(int ch ,  _MOTION z , _MOTION x ,_MOTION y)
{
	if ( z.pos < (_vm.dev.work_thick	+	_vm.dev.tape_thick) )	{	//Z�����̈ʒu�́A�؂肱��ł���ꏊ�B
#if	1	//
	__aho =1;	//toku breakp
#endif

		//	x,y	�����[�N�̓����ɂ͂����Ă��邩�𔻒肵�܂��B
		if( _vm.dev.cut_pat == _SQUARE )	{
			int size_x,size_y;	_vm.dev.size(ch,&size_x,&size_y);		//���[�N�T�C�Y���擾���܂��B
			if	(	(fabs(y.pos) <= size_y/2) && (fabs(x.pos) <= size_x/2)	) {	goto _in_work;	}
		} else {											//_ROUND
			double	r = _vm.dev.work_size_r/2;
			if	(f_eq_less(	fabs(y.pos), _vm.dev.work_size_r/2) ) {				//y�����a�ȓ��ł����
//_printf("r[%lf]  y[%lf] \n"  , r , y.pos );
//_printf("r*r=[%lf]  y*y=[%lf] ->%lf \n"  , (r*r) , (y.pos * y.pos) , (r*r) - (y.pos * y.pos) );
				if ( f_eq_less(fabs(x.pos) , sqrt((r*r) - (y.pos * y.pos))) )	{	//x�����[�N�͈͓��ł����
//_printf("x.pos:%lf  < sqrt(%lf)\r\n"  , fabs(x.pos) , sqrt((r*r) - (y.pos * y.pos)) );
					goto _in_work;			//�]�T�����܂߂邩�ǂ����B?�́A�܂�	�����Ń��[�N�͈͓��ł��B
				}
			}
		}
	}
	return false;
_in_work:
	return true;
}
//6�����[�V�������w�����Ă��������B
bool	work::kirikomi( _MOTION m[] , int axis )
{
	int ch = _line.cut_ch(m[_T]);	//toku �ƃ��[�V��������Ach�����߂�֐��́Acutline�I�u�W�F�N�g�ɂ���܂��B
	//�܂��Ƃ��J�b�g�`�����l���̂ǂ�ɂ����Ă͂܂�Ȃ��ꍇ�ɂ�false�Ƃ��܂��B
	if( ch < 0 ) return false;

	if (axis == _Z2 )	{
		return	_kirikomi( ch,m[_W] , m[_X] , m[_V] );
	}
	return	_kirikomi( ch,m[_Z] , m[_X] , m[_Y] );
}
//------------------------------------------------------------------------------------------------------------
//	���[�N�I�u�W�F�N�g�ł́A�e���̍��W���A
//	Y���ɂ��āF
//	�Y���̃��[�V�������A	�u���[�h�i�J�b�g����j�ł̊Y���ʒu�Ȃ̂��A
//							�������ł̈ʒu�i�ײ��Ă�A�������)�Ȃ̂��A
//							�킩��Ȃ��B

//						��	�P���Ɍ��������S�����������ƁA���������S�����Y���ʒu�ɂȂ�B

//							blade_scope�𑫂��ƁA�u���[�h���J�b�g���Ă���ʒu���A���������݂�ʒu�֕ϊ�����܂��B
//							
//							���@	�����A�J�[�t�`�F�b�N����Y���ʒu����Ablade_scope�������Ă��܂��ƁA
//									�Ӗ��̂Ȃ��ʒu�ɂȂ��Ă��܂��B
//									
//									�������ǁA���̍��W�́A
//									�u���܃u���[�h������ʒu�v���A���[�N���S�����_�Ƃ������W�ŕ\�����́B
//									�ƁA�Ƃ炦��ƁA�悢�B
//									�����A�J�[�t�`�F�b�N���쒆�̏ꍇ�́A
//									�J�[�t�`�F�b�N���s���Ă���Œ��́A�u���[�h�̈ʒu���g���[�X���Ă��邱�ƂɂȂ�܂��B
//									���̏ꍇ�ɂ́A���̍��W���ƍ���Ȃ��̂ŁA����Ɍ������̈ʒu�֕ϊ�����K�v������܂��B


//	���̎��_�ł́A���W�ϊ���
//		X:	���[�N���S�iSpndl_PosX�)����̑��ΓI�ȁA�u���[�h�̈ʒu�ł��B
//		Y:	���[�N���S�ʒu(Alu_Cy_Hi �)����̑��ΓI�ȁA�u���[�h�̈ʒu�ł��B
//		Z:	CT��ʂ���́A�u���[�h�̈ʒu�ł��B�i�n�C�g�ʁj
//		T�F	HOME_POST����̑��΋����Ƃ��Ă����܂��B�J�b�g���ł���Ί�{�I��0�܂���90���ƂȂ�܂��B
//	return (pos*-1)+CutParam->SpdlPosX+CutParam->WorkCenterX;

//		�؂肱��ł��锻��B
//	��	���W���A
void	work::workcenter_origin(unsigned int t , _MOTION m[] )
{
	_vm.update(t);											//VM�����̎����փZ�b�g���܂��B

	//X
	m[_X].pos	-=	_vm.mac.spdl_posx;							//CT���S����̍��W��	spndl_posxa

	//Y
	{
		m[_Y].pos	-=	_vm.mac.alu_cy_hi;							//���������S���W�B
//toku	ali_pos_y����
		m[_Y].pos	-=	_vm.mac.b_blade_scope;						//�u���[�h�X�R�[�v��
	}
	//V
	{
		m[_V].pos	=	_vm.mac.b_blade_scope_v	-	m[_V].pos;	//
		m[_V].pos	-=	_vm.mac.alu_cy_hi;							//���������S���W�B
	}
	//Z
	{
		m[_Z].pos	-=	_vm.mac.b_ct_posz;	
	m[_W].pos	-=	_vm.mac.b_ct_posw;	
		m[_Z].pos	*= -1;					m[_W].pos	*= -1;					//�n�C�g�ʂɂ���̂�-1�������܂��B
	}
	//T]
	{	m[_T].pos	-=	_vm.mac.homepos['T'];		}

	//return m;
}
//	motion_array[]	��	6��(XYVZWT)���B
//	���[�V�����z��	m[]�́Aenum	{	_X , _Y , _V , _Z , _W , _T	};�̏����ł���Ă��������B
static	int	__a=0,__b=0,__c=0;		//
extern	int	__ca,__cb,__cc,__cd;	//
extern	int	__ce,__cf,__cg,__ch;
void	_debug_reset(void)
{
	
}


bool	work::record( unsigned long tick, _MOTION m[] )
{
	//---------------------------------------------------------------------------------------------------
	//	���̑O�i�ɂāA�t�@�C���ۑ����ꂽ�L�^�f�[�^��ǂݍ��݁A
	//---------------------------------------------------------------------------------------------------
//	int i,n=n_record();		AXIS_RECORD_DATA	d;	//(12 byte)
//	for( i=0 ; i < n ; i++ ) {
//		if ( get_record(i,d) != true)	{	_msg("cut_confirm() get_record_error"); goto _error;	}
//		unsigned long t;	for( t=old_tick ; t < d.tick ; t++ )	{	//���̎��Ԃ�VM�ǂݍ��݂ł��B
//			_work.record( t ,get_multi_motion( (double)t , "XYVZWT", motion_array , 6 , true));
//
////toku XYVZWT	6�����̍�tick��motion�ł��B
//m[]
//		}
//	}
//CH�����ł��Ȃ��ꍇ�B�B�BT�����쒆�̏ꍇ�ȂǁB
//���̂Ƃ��ɁA�ǂ�CH�Ƃ��邩
	//---------------------------------------------------------------------------------------------------
	//motion _arra
	DWORD	t = GetTickCount();
	_vm.update(tick);		//VM�̏󋵂��w�����ꂽ�`�b�N�ɂ��܂��B
{	__a +=  GetTickCount() - t;	t = GetTickCount();	}

	//���[�N���S���_��Z�̓n�C�g�ɕϊ����܂��B
	workcenter_origin(tick,m);		//���W��ϊ����܂��B���[�N���S/CT���/�ƌ��_�ʒu	�����_�Ƃ����u���[�h�̈ʒu�ł��B

{	__b +=  GetTickCount() - t;	t = GetTickCount();	}

//toku ���̂Ƃ��Ƃ͌��_����
	_line.add(tick, m , kirikomi(m,_Z1) , kirikomi(m,_Z2) );

{	__c +=  GetTickCount() - t;	t = GetTickCount();	}

	return true;
}

//���[�N�I�u�W�F�N�g���������H���ǂ��������؂��܂��B
bool	work::operator==( work& a)
{	//�}�V���f�[�^�͈�v���Ă���K�v������B
	if ( (_vm.dev == a._vm.dev) && (_vm.mac==a._vm.mac))
	{
//		if(_line	==	(const cutline&)(a._line))	{	return true;	}

#if	0
		cutline l=_line;
		if( l==a._line)	return true;
#else
		if( _line == a._line ) return true;
#endif
//		if(a._line	==	_line)	{	return true;	}
		//toku ���̎��_�ŁAcutline l���A��������������Ă��܂��I�I
		//���̂��߁A
	}
	return false;
}


void	work::dump_all(int from , int to)
{
	_motion_set	s;
	const _multi_motion*p =_line.multi_adr();

	if( ! to ) {	to	=	_line.n_multi();	}
_printf("=== DUMP ALL ( [%d- %d] of  total:%d)===\n" , from ,to ,_line.n_multi());
	int i;for( i=from; i<to ; i++ )	{
		s = _line.get_motion_set(i,true);
//		_printf(" (%d) ch[%d] line[%d] Z1[%d]Z2[%d] x[%lf][%d] y[%lf][%d] z[%lf][%d] v[%lf][%d] w[%lf][%d]\n"	,
	//����Ȃ�CSV�Ƃ���
		_printf("%d,%lf,%d,%d,%d,%d,%lf,%d,%lf,%d,%lf,%d,%lf,%d,%lf,%d\n"	,
			i					,
			(p+i)->t-(p)->t		,
			(p+i)->flg.ch		,
			(p+i)->flg.line		,
			(p+i)->flg.cut_z	,
			(p+i)->flg.cut_w	,
			s.s[_X].pos	,	s.s[_X].phase	,	s.s[_Y].pos	,	s.s[_Y].phase	,	s.s[_Z].pos	,	s.s[_Z].phase	,
			s.s[_V].pos	,	s.s[_V].phase	,	s.s[_W].pos	,	s.s[_W].phase		);
		::Sleep(1);	//�_���v���S���łȂ��Ƃ�������̂�
	}
}

void	work::dump(void)
{
	_motion_set	s;	bool	cutf[2];
	int	nest;	interval	l;
	const _multi_motion*p =_line.multi_adr();

	interval	total_t,cut_t;		//�g�[�^�����Ԃƈꃉ�C�����̎���

	//�_���v���ǂ��t���Ȃ��Ƃ�������̂ŃX���[�v������Ȃ���
	for( l = _line.get_next_line(l) ; l.valid() ; l=_line.get_next_line(l) ) {
		//�ꃉ�C�����̎��Ԃł��B
		{	interval	line_t;		line_t.start  = (p+l.start)->t;	line_t.end = (p+l.end)->t;
			_printf("ch[%d] line[%d]: total(%d ms) \n" ,  (p+l.start)->flg.ch , (p+l.start)->flg.line , line_t.n() );
		}
#if	1
	{if( (p+l.start)->flg.ch == 1 ) {	if( ((p+l.start)->flg.line)==57 )	{
		__a=1;	}	
	}}
#endif
		cutf[0]=cutf[1]=false;	nest=0;	bool dumped;
		int i;	for( i = l.start ; i <= l.end ; i++ ) {
			//�J�b�g�̂��镔�������\�����Ă݂܂��B�H
			//(Z1��)
			dumped=false;
			for(int axis=0 ; axis < 2 ; axis++) {
//tok	_multi[l.start - l.end] ���Ach**,line**�̈�Q�ł���B
//		CH��LINE�́ARun()�̍ۂɊ���t���Ă���B
//		�E�V�����ԍ��̊���t���́A
//			�E�J�b�g�؂肱�݂�����
//			�ECH	:	���J�E���^�����CH�ԍ��B
//			�ELINE	:	�O��̃��C��������΁A���̃��C���Ɣ�r���āA
//
//			�؂肱��ł���Z���ɑ������Ă���	Y���ʒu	��	�����ł���Γ������C���Ɣ��f���Ă���B
//			Y���ʒu�����ƂȂ�΁A�Ⴄ�Ɣ��f���Ă���B
//			Z1��Z2���؂肱��ł��Ȃ���΁A�O���CH��LINE�Ɠ����ɑ����Ă���Ɣ��肵�Ă���B
//		�����̂��߁Acutline::get_a_line()�Ŏ擾�ł���J�b�g���C���́A
//			�ŏ��̐؂荞�݈ʒu�i���[�N�ɍ����|�������Ƃ���j	�`	�؂�I���	�`	���̃��C���̐؂荞�݈ʒu�̒��O	�܂�
//			�̃Z�b�g�ɂȂ��Ă���B
//			
//			���߂�̂Ƃ��ȂǂɁA�؂肱�݂���������ƁA�Ⴄ���C���Ƃ��Čv�Z�����B�i���̃��C���͎��ۃ��C���ԍ�������邱�ƂɂȂ�B)
//		��	���̃J�b�g����́A
//				��{�I�ɂ́A��x�t���O���o�ƁA�ǂ��炩�̃u���[�h���₦�ԂȂ����[�N�ɓ���̂ŁA
//				�J�b�g�J�n�A�J�b�g�I���A���ŏ��ɂ��āA���̌㉽���t���O�������Ȃ��Ő؂蔲���`�����C���̐؂�n�߂����Ƃɂ��������`�ɂȂ�
//
//		��	�`���b�p�[�g���o�[�X�ł́A�󔒂́i�u���[�h���؂肱��ł��Ȃ��󋵂̕������r���Ɍ����Ǝv���܂��j
//			����͖��Ȃ��B
//			
//		��	�X���[�C���A�A�E�g�ł́A�؂肱��ł���r���ŁA���x���ω�����L�^���c���Ă��邾�낤�B
//			dump()�ł́Acutf�������Ă���Acutf�������镔���܂Ŕ�΂��Ă��܂��̂ŁA
//			cutf��������̂�҂Ƃ��ɁA
//				�����e���ɑ��x�ω�������iCONST�łȂ��t�F�C�Y������Ȃ�j�A�����dump�ɂ��Ă݂邱�ƁB

//		��	�J�b�g�r����
//				�J�[�t�`�F�b�N�^�I�[�g�Z�b�g�A�b�v����E�E
//				local�œ��삳���邩�H�����삳���Ȃ��B
//				real�ł̋L�^�̏ꍇ�����삷��B���̋L�^�͂̂����Ă悢
//				�����cutconfirm�ł̓Z�b�g�A�b�v����Ƃ��A�J�[�t�`�F�b�N�ƔF���E�E
//				���̏ꍇ�A�E�E
//				
//			
//		
//		�I�[�g�Z�b�g�A�b�v����Ȃǂ��͂���ꍇ�ɁALIST�Ƌ������Ăւ�ȓ���ɂȂ邱�Ƃ��A���ɂ߂����B
//		AxisAbs���A�N���ǂ񂾂����L�^�����ق������������B(LIST�Ȃ̂��A���̃v���Z�X�Ȃ̂�)
//		

				if( !cutf[axis] && (p+i)->cut(axis) ) {							//�J�b�g���n�܂����B
					s = _line.get_motion_set(i,true);
					{	cut_t.start = (unsigned int)s.t;	if( total_t.start<0 )total_t.start = (unsigned int)s.t;}		//�J�b�g�J�n����

					{int i;for(i=0;i<nest;i++){ _printf("  ");	}}
					_printf("   <-- [Z%d] (%d) x[%lf][%lf][%d] Z1<y[%lf] z[%lf]> Z2<v[%lf] w[%lf]>\n" , axis+1 , i , s.s[_X].pos , s.s[_X].v , s.s[_X].phase , s.s[_Y].pos , s.s[_Z].pos , s.s[_V].pos , s.s[_W].pos);
					::Sleep(1);	//�_���v���S���łȂ��Ƃ�������̂�
					cutf[axis]=true;	nest++;
					dumped=true;
				} else if( cutf[axis] ) {
					s = _line.get_motion_set(i,true);	//���Ԃ������Ă��܂�����

					if( !(p+i)->cut(axis) ) {	//�J�b�g���ʂ����B
						--nest;
						{int i;for(i=0;i<nest;i++){ _printf("  ");	}}
						{	cut_t.end = total_t.end = (unsigned int)s.t;	}		//���Ԃł��B
						_printf("       [Z%d] (%d) x[%lf][%lf][%d] Z1<y[%lf] z[%lf]> Z2<v[%lf] w[%lf]> --> (%d ms) \n" , axis+1 , i , s.s[_X].pos , s.s[_X].v ,  s.s[_X].phase , s.s[_Y].pos , s.s[_Z].pos , s.s[_V].pos , s.s[_W].pos , cut_t.n());
						::Sleep(1);	//�_���v���S���łȂ��Ƃ�������̂�
						cutf[axis]=false;
						dumped=true;
					} else {					//�J�b�g���ł��B
/*
						//toku �X���[�C���B�A�E�g�̏ꍇ�ɂ́A�����ŃJ�b�g���x���ω����邱�Ƃ�����܂��B
						int a;for(a=0 ; a < _AXIS_IDX_MAX ; a++ ) {
							if( (s.s[a].phase!=PHASE_CONST) && (s.s[a].phase!=PHASE_STOP)	)	{	//���삵�Ă��鎲��
//_printf("--a=%d phase=%d\n" , a  , s.s[a].phase );
									_printf("       **acc** (%d) x[%lf][%d] y[%lf][%d] z[%lf][%d] v[%lf][%d] w[%lf][%d]\n" , i ,
									s.s[_X].pos	,	s.s[_X].phase	,	s.s[_Y].pos	,	s.s[_Y].phase	,	s.s[_Z].pos	,	s.s[_Z].phase	,
									s.s[_V].pos	,	s.s[_V].phase	,	s.s[_W].pos	,	s.s[_W].phase);
									break;
							}
						}
*/
					}
				}
//_printf("axis loop" );
			}
			//�����܂ŃJ�b�g���C���̃_���v�ł���
			if ( !dumped ) {
				int ch=(p+l.end)->flg.ch;	int	line=(p+l.end)->flg.line;
				if( (ch== 2) && (line==23 ||line==24) )	{
					s = _line.get_motion_set(i,true);
					_printf("          =[X]= (%d/%d) x[%lf][%d] y[%lf][%d] z[%lf][%d] v[%lf][%lf][%d] w[%lf][%d]\n" , i , l.end,
						s.s[_X].pos	,	s.s[_X].phase	,	
						s.s[_Y].pos	,	s.s[_Y].phase	,	
						s.s[_Z].pos	,	s.s[_Z].phase	,
						s.s[_V].pos	,	s.s[_V].v		,	s.s[_V].phase	,
						s.s[_W].pos	,	s.s[_W].phase		);
					::Sleep(2);
				}
			}
		}
#if	0
	{
		if( (p+l.end)->flg.ch == 1 && ((p+l.end)->flg.line)==57 )	{
			int i;for(i=l.end ; i < _line.n_multi() ; i++ ) {
				_motion_set s = _line.get_motion_set(i);
				_printf("[%d]/[%d] y[%lf] z[%lf] w[%lf] cut[%d][%d]\n" ,i,_line.n_multi(), s.s[_Y].pos , s.s[_Z].pos , s.s[_W].pos , s.flg.cut_z , s.flg.cut_w );
::Sleep(2);
			}
		}
	}
#endif
	}
	//�J�b�g�g�[�^������
	_printf("\n == CUT FIN (total %d ms)\n" , total_t.n());
}

//	int i;for( i=0; i < _line.n_multi()  ; i++)	{
//		const _multi_motion*p =_line.multi_adr();
//		//�J�b�g�t���O�`�J�b�g�t���O�Q��܂ł��_���v���Ă݂܂��B
//if( i==0 )_printf("p+i[%d]:[0x%x]\n" ,  i , p+i);

//	//toku �����Z1�����J�b�g���Ă���ꍇ��
//		if( !cutf && ( (p+i)->flg.cut_z==1) )	{
//			s = _line.get_motion_set(i,true);
//			_printf("[%d][%d] x[%lf][%lf] Z1<y[%lf] z[%lf]> Z2<v[%lf] w[%lf]> - " , s.flg.ch , s.flg.line , s.s[_X].pos , s.s[_X].v , s.s[_Y].pos , s.s[_Z].pos , s.s[_V].pos , s.s[_W].pos);
//			cutf = true;
//		} else if( cutf && ( (p+i)->flg.cut_z==0) )	{
//			s = _line.get_motion_set(i,true);
//			_printf("   x[%lf][%lf] Z1<y[%lf] z[%lf]> Z2<v[%lf] w[%lf] \n" , s.s[_X].pos , s.s[_X].v , s.s[_Y].pos , s.s[_Z].pos , s.s[_V].pos , s.s[_W].pos);
//			cutf = false;
//		}
//	}
//
//	//�J�b�g���C���ȊO�̎��̋O�Ղ��_���v���Ă݂�H
//	{
//		int i ;for(i= 0 ; i < _line.n_multi() ; i++ )	{
//			
//		}
//	}

//toku	�Z�[�u�ƃ��[�h�����܂��B
//
bool	work::save(void)
{
	//vm�֘A�̓t�@�C������ۑ����邾���ɂ��܂��B
	_Assert( _filename[0]!='\0' , "work::save() filename not exist(execute run()");
	HANDLE fh = open((const char*)_filename,true);
	WORK_SAVE_HEADER	h;	DWORD	w;
	//------------------------------------------------------------------------------
	//	�w�b�_�쐬�E��������
	//------------------------------------------------------------------------------
	int total_size =0;	//�g�[�^���T�C�Y���m���߂܂��B�Ō�T������ꂽ�ق�����������
	{
		h.header_size			=	sizeof(WORK_SAVE_HEADER);
		h.filename_size			=	sizeof(_filename);				//�t�@�C����(�o�b�t�@)�T�C�Y�ł��B
		h.multi_motion_size		=	sizeof(_multi_motion);			//�}���`���[�V�����\���̂̃T�C�Y�B
		h.single_motion_size	=	sizeof(_single_motion);			//�V���O�����[�V�����\���̂̃T�C�Y
		h.n_multi_motion		=	_line.n_multi();				//�ۑ�����Ă���}���`���[�V�������B
		h.n_single_motion		=	_line.n_single();				//�ۑ�����Ă���V���O�����[�V�������B
		//�w�b�_���������݂܂��B
		if(	::WriteFile( fh , (void*)&h , sizeof(WORK_SAVE_HEADER), &w , 0) != TRUE )	{	_printf("write err(header)[%s]" , LastErrorMsg()); goto _error;	}
		total_size =w;
	}
	//------------------------------------------------------------------------------
	//	�f�[�^������������
	//------------------------------------------------------------------------------
	if(	::WriteFile( fh , (void*)&_filename[0] , h.filename_size , &w , 0) != TRUE )	{	_printf("write err(fname)"); goto _error;	}
	total_size +=w;
	//�}���`���[�V�����̏������݂ł��B
	if(	::WriteFile( fh , (void*)_line.multi_adr()	,	sizeof(_multi_motion)*h.n_multi_motion	, &w , 0) != TRUE )	{	_printf("write err(multi)\n" ); goto _error;	}
	total_size +=w;
	if(	::WriteFile( fh , (void*)_line.single_adr()	,	sizeof(_single_motion)*h.n_single_motion, &w , 0) != TRUE )	{	_printf("write err(single)\n" ); goto _error;	}
	total_size +=w;
//__PsSwsw();
	if(fh>=0)	::CloseHandle(fh);
	return true;
_error:	
	if(fh>=0)	::CloseHandle(fh);
	return	false;
}
bool	work::load(const char*filename)
{
	HANDLE fh = open((const char*)filename,false);
	WORK_SAVE_HEADER	h;	DWORD	r;

//	//�܂��w�b�_��ǂݍ��݂܂��B
	if( ::ReadFile(fh,(void*)&h , sizeof(WORK_SAVE_HEADER) , &r,NULL) == 0 ) {_msg( "work::load()::readfile error: [%s]" , LastErrorMsg());	goto _error;	}
	//�w�b�_��ǂ݁A�e�T�C�Y���`�F�b�N���܂��B
		if(	h.header_size			!=	sizeof(WORK_SAVE_HEADER))	{ _msg("work::load()::_h.header_size illegal(org=%d / read=%d )"		, sizeof(WORK_SAVE_HEADER) ,h.header_size );		goto	_error;	}
		if(	h.filename_size			>	sizeof(_filename))			{ _msg("work::load()::_h.filename_size illegal(org=%d / read=%d )"		, sizeof(_filename) , h.filename_size );			goto	_error;	}
		if(	h.multi_motion_size		!=	sizeof(_multi_motion))		{ _msg("work::load()::_h.multi_motion_size illegal(org=%d / read=%d )"	, sizeof(_multi_motion), h.multi_motion_size	);	goto	_error;	}
		if(	h.single_motion_size	!=	sizeof(_single_motion))		{ _msg("work::load()::_h.single_motion_size illegal(org=%d / read=%d )"	, sizeof(_single_motion), h.single_motion_size	);	goto	_error;	}
		if(	h.n_multi_motion		>	_line.allocated(_MULTI))	{ _msg("work::load()::_h._n_multi overflow(org=%d / read=%d )"			, _line.allocated(_MULTI) , h.n_multi_motion	);	goto	_error;	}
		if(	h.n_single_motion		>	_line.allocated(_SINGL))	{ _msg("work::load()::_h._n_single overflow(org=%d / read=%d )"			, _line.allocated(_SINGL) , h.n_single_motion	);	goto	_error;	}

	//�T�C�Y���v�Ȃ̂Ńf�[�^���R�s�[���Ă����܂��B
	//�t�@�C���|�C���^�͌��݃w�b�_�̏I�_�ł��B���Ԃɓǂ�ł����܂��B
	{
		if( ::ReadFile(fh,(void*)&_filename[0]	, h.filename_size	, &r,NULL) == 0 )	{	_msg( "work::load()file read error(0): [%s]" , LastErrorMsg());	goto	_error;		}
		if( _line.load(fh,h.n_multi_motion , h.n_single_motion)!=true	)				{	_msg( "work::load()file read error(1): [%s]" , LastErrorMsg());	goto	_error;		}
	}

	//�����Ńt�@�C������߂܂����B�t�@�C�����ɉ�����VM�Ǝ��f�[�^��ǂݍ��݂܂��B
	{
_printf("file=[%s]\n" , _filename);
		if(	_vm.read(_filename)	!=	true)	{	goto _error;	}
	}
	return true;
_error:	
if(fh>=0)	::CloseHandle(fh);

	return	false;
}
#define		VOLUME		""															//�t�@�C���A�N�Z�X���邽�߂̃{�����[�����B�p�X�̑O�ɂ���K�v������܂�
#define		EXT			".work"																	//KcMap�t�@�C���̊g���q
#define		_FILE_DIR	"C:\\Disco\\6000_toku\\V250_CUT_CONFIRM\\soft\\NT\\cut_confirm\\"		//�ۑ���f�B���N�g���ł��B
static	void	_make_path(char*buff , const char*fname ,unsigned int max )
{
	_Assert( (strlen(VOLUME) + strlen(_FILE_DIR) , strlen(fname) + strlen(EXT)) < max , "[axis_record:_make_path] buff not enough file=%s max=%d" , fname , max );
	sprintf(buff,"%s%s%s%s" ,VOLUME , _FILE_DIR , fname , EXT);
}
HANDLE	work::open(const char *filename , bool write)
{
	char path[512];	_make_path(path,filename,sizeof(path));		//filename�́A�t�@�C�����������w�����܂��B�K���Ȋg���q�����ĊJ���܂��B
	unsigned long r;	//ReadFile�̓ǂݍ��݃o�C�g��
	//���Ƀt�@�C�������݂��Ă���ꍇ�͂��̂܂ܕԂ��܂�
//	if( _access( filename , 0 ) == 0)	return true;
	//�t�@�C���I�[�v��
	//
	DWORD	dwCreationDisposition;	//�I�[�v������Ƃ��̃t�@�C�����݂��邩�ǂ�����
	if( write ){
		if( _access( path , 0 ) == 0 ) {
				dwCreationDisposition=	TRUNCATE_EXISTING;
		} else {	dwCreationDisposition=	CREATE_ALWAYS;	}
	}else {		//�ǂݍ��݂̏ꍇ�ɂ͂��Ȃ炸�t�@�C�������݂���O��Ƃ��܂��B
		dwCreationDisposition=OPEN_EXISTING;
	}

	HANDLE	fh = ::CreateFile(
				path									,		//�t�@�C����
				write ? GENERIC_WRITE: GENERIC_READ		,		//�ǂݍ��݃I�[�v��
				(write ? (FILE_SHARE_WRITE):0) | (FILE_SHARE_READ)	,		//���L���[�h�F�ǂݍ��݂͋���
				NULL					,		//�q�v���Z�X�Ƀn���h���̌p�����������iNULL:�����Ȃ��j
				dwCreationDisposition	,		//
				FILE_ATTRIBUTE_NORMAL	,		//�t�@�C�������F�����Ȃ�
				NULL );
	//�I�[�v�����s
	_Assert( fh != INVALID_HANDLE_VALUE , "[work::fileopen] FileOpen error %s [%s]\n" , filename , LastErrorMsg()	);
	return	fh;
}

//�w��
bool	work::run(const char*filename )
{
	_printf("_MOTION=%d / single_motion=%d / multi_motoino=%d / motion_set=%d\n" , sizeof(_MOTION), sizeof(_single_motion) , sizeof(_multi_motion) , sizeof(_motion_set));
	//�e
	if(	_vm.read(filename)	!=	true)	{	goto _error;	}//�f�o�C�X�f�[�^�E�}�V���f�[�^��ǂݍ��݂܂��B

	//������L�^��ǂݍ��݂܂��B
	if( axis_file_read(filename)!= true	)	{	goto _error;	}

	//�܂�VM����A�e���_�����߂܂��B
	//�ϐ��́A�����ɂ���ĕω������肵�܂��B
	//���������菑���Ă݂܂��B
	//���[�J������p�ӂ��āA�L�^�ʂ�ɓ��삳���Ă݂܂��B
	{	local_axis_init(&_vm);		}	//����Ń��[�J�������ł��܂����B

	//��������`�F�b�N���Ă����܂��B
	{
		unsigned long prev_tick=0;						//�ЂƂO��
		int i,n=n_record();		AXIS_RECORD_DATA	d;	//(12 byte)
#if 0//
		for( i=0 ;  i < 50 ; i++ )	{
#else
		for( i=0 ;  i < n ; i++ )	{
#endif
			//	�i�����얽�߂̋L�^�j	����擾���܂��B
			if ( get_record(i,d) != true)	{	_msg("cut_confirm() get_record_error"); goto _error;	}

			//�O��̎w������A����̎w���܂ł̃��[�V���������[�N�I�u�W�F�N�g�ɋ����܂��B
			if( prev_tick ) {
				_MOTION motion_array[6];	//6�����̃��[�V�����ł��B
DWORD _t = GetTickCount();
//�����ɉ��b���������Ă��܂��悤�ł��B
				unsigned long t;	for( t=prev_tick ; t < d.tick ; t++ )	{	//���̎��Ԃ�VM�ǂݍ��݂ł��B
					record( t ,get_multi_motion( (double)t , "XYVZWT", motion_array , 6 , true));
				}

_printf("n(%d) ->  t[%d]ms  __a[%d] __b[%d] __c[%d] __ca[%d] __cb[%d] __cc[%d] __cd[%d]\n" ,	d.tick - prev_tick , GetTickCount() - _t  , __a ,__b ,__c,__ca,__cb,__cc,__cd);
_printf("  __ce[%d] __cf[%d] __cg[%d] __ch[%d]\n" , __ce=0,__cf=0,__cg=0,__ch=0);
			}
			//�擾�����f�[�^����A���[�J���̎��𓮍삳���܂��B
			local_axis_call(d.tick , d.r4_axis, d.r0_cmd , d.r5 , d.r6 , d.r7 );
			prev_tick = d.tick;		//�O��̃`�b�N��ۑ����܂�
		}
	}
	//�����łł��܂����B���[�N�I�u�W�F�N�g�̃t�@�C�����������Ŋm��Ƃ��܂��B
	strcpy_lim(	_filename , sizeof(_filename) , (char*)filename );
	return	true;
_error:
	_filename[0]='\0';	//���s�����ꍇ�ɂ̓t�@�C�����𖳌��ɂ��܂��B
	return	false;
}


//
//	//�T�C�Y���`�F�b�N���܂��B
//	{
//		//�w�b�_�̃t�H�[�}�b�g�������Ă��邩�ǂ���
//		if(	_h.header_size		!=	sizeof(VM_SAVE_HEADER))		{	_msg("work::load()a::_h.header_size illegal(org=%d / read//=%d )"		, sizeof(VM_SAVE_HEADER) , _h.header_size );		goto	_error;	}
//		if(	_h.vm_entry_size	!=	sizeof(VM_ENTRY))			{	_msg("vm_read_data::_h.vm_entry_size illegal(org=%d / re//ad=%d )"	, sizeof(VM_ENTRY) , _h.vm_entry_size );			goto	_error;	}
//		if( _h.N_entry_size		>	sizeof(NEntry) 	)			{	_msg("vm_read_data::N entry size illegal(org=%d / read=%//d )"		, sizeof(NEntry)	, _h.N_entry_size	);			goto	_error;	}
//		if( _h.S_entry_size		>	sizeof(SEntry)	)			{	_msg("vm_read_data::S entry size illegal(org=%d / read=%//d )"		, sizeof(SEntry)	, _h.S_entry_size	);			goto	_error;	}
//		if( _h.B_entry_size		>	sizeof(BEntry) 	)			{	_msg("vm_read_data::B entry size illegal(org=%d / read=%//d )"		, sizeof(BEntry)	, _h.B_entry_size	);			goto	_error;	}
//		if( _h.vm_mmtbl_size	>	sizeof(VM_MMTBL))			{	_msg("vm_read_data::MMTBL size illegal(org=%d / read=%d //)"			, sizeof(VM_MMTBL)	, _h.vm_mmtbl_size	);			goto	_error;	}
//		if( _h.vmdata_size		>	sizeof(VMDATA)	)			{	_msg("vm_read_data::VMDATA size illegal(org=%d / read=%d// )"			, sizeof(VMDATA)	, _h.vmdata_size	);			goto	_error;	}
//		if( _h.vm_hash_tbl_size		> (VM_HASH_TBL_SIZE*sizeof(unsigned int))	)			{	_msg("vm_read_data::hash_tbl// size illegal(org=%d / read=%d )"		, VM_HASH_TBL_SIZE	, _h.vm_hash_tbl_size);			goto	_error;	}
//		if( _h.vmtrace_record_size	> sizeof(_trace_record))	{	_msg("vm_read_data::vmtrace_record size illegal(org=%d /// read=%d )"	, sizeof(_trace_record)	, _h.vmtrace_record_size);	goto	_error;	}
//		if( _h.vmtrace_pool_size	> sizeof(_trace_pool))		{	_msg("vm_read_data::vmtrace_pool size illegal(org=%d / r//ead=%d )"	, sizeof(_trace_pool)	, _h.vmtrace_pool_size);	goto	_error;	}
//	}
//	//�T�C�Y���v�Ȃ̂Ńf�[�^���R�s�[���Ă����܂��B
//	//�t�@�C���|�C���^�͌��݃w�b�_�̏I�_�ł��B���Ԃɓǂ�ł����܂��B
//	{
//		//�e�� VMENTRY
//		if( ::ReadFile(fh,(void*)&NEntry	, _h.N_entry_size		, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(0): [%s]" , LastErrorMsg());	goto	_error;		}
//		if( ::ReadFile(fh,(void*)&SEntry	, _h.S_entry_size		, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(1): [%s]" , LastErrorMsg());	goto	_error;		}
//		if( ::ReadFile(fh,(void*)&BEntry	, _h.B_entry_size		, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(2): [%s]" , LastErrorMsg());	goto	_error;		}
//		if( ::ReadFile(fh,(void*)&VM_MMTBL	, _h.vm_mmtbl_size		, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(3): [%s]" , LastErrorMsg());	goto	_error;		}
//		if( ::ReadFile(fh,(void*)&VMDATA	, _h.vmdata_size		, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(4): [%s]" , LastErrorMsg());	goto	_error;		}
//		//�n�b�V���e�[�u��
//		if( ::ReadFile(fh,(void*)&NHashTbl[0], _h.vm_hash_tbl_size	, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(5): [%s]" , LastErrorMsg());	goto	_error;		}
//		if( ::ReadFile(fh,(void*)&SHashTbl[0], _h.vm_hash_tbl_size	, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(6): [%s]" , LastErrorMsg());	goto	_error;		}
//		if( ::ReadFile(fh,(void*)&BHashTbl[0], _h.vm_hash_tbl_size	, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(7): [%s]" , LastErrorMsg());	goto	_error;		}
//		//VM�g���[�X
//		if( ::ReadFile(fh,(void*)&_trace_record[0], _h.vmtrace_record_size,&r,NULL)==0)	{	_msg( "vm_read_data::file read e//rror(7): [%s]" , LastErrorMsg());	goto	_error;		}
//		if( ::ReadFile(fh,(void*)&_trace_pool[0], _h.vmtrace_pool_size,&r,NULL)==0)		{	_msg( "vm_read_data::file read e//rror(7): [%s]" , LastErrorMsg());	goto	_error;		}
//	}
//
//	return true;
//	//����ŃR�s�[�͂����܂��ł��B
//	//�A�h���X��ϊ����܂���?
//	//�t�@�C���쐬�ł����瑦�N���[�Y���܂�
//	//�����Ƃ��ăt�@�C������
//	strcpy_lim(	_filename , sizeof(_filename) , (char*)filename );
//	::CloseHandle(fh);
//	return true;
//_error:
//	::CloseHandle(fh);
//	_filename[0]=0;	//���s�̏ꍇ�́A���������Ƃ��Ă����܂��B
//////////////////////

