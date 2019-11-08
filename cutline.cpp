//------------------------------------------------------------------------------------------------------------------
//	motion_pool.cpp
//	���[�V�����E�v�[��	��ʂ̎����[�V�������L�^���邽�߂̏ꏊ�ł��B
//	���͓�̃��[�V�����v�[�����r���邽�߂ɁA����̃v�[�����K�v�ł��B
//	���I�ɏK������K�v������H�H
//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
//	include
//------------------------------------------------------------------------------------------------------------------
#include	"cutline.h"
#include	"commonmodule.h"
#include	"local_axis_interface.h"		//���[�J�����ɕ�������

#include	<stdlib.h>

//------------------------------------------------------------------------------------------------------------------
//	defines
//------------------------------------------------------------------------------------------------------------------
//�������̃g���[�X���s�����߂�

#define		_DEFAULT_SINGL_MOTION_SIZE	(8*1024*1024)							//8MByte�Ƃ�܂�	(419430 ��)	�S	419sec(�P��������)4������100sec���x
#define		_DEFAULT_MULTI_MOTION_SIZE	(2*1024*1024)							//2MByte�Ƃ�܂�	(174762	��)	�S 174sec��

//------------------------------------------------------------------------------------------------------------------
//	global
//------------------------------------------------------------------------------------------------------------------
//tokutokutoku
int		__ca=0,__cb=0,__cc=0,__cd=0;
int		__ce=0,__cf=0,__cg=0,__ch=0;
//
int		__da=0,__db=0,__dc=0,__dd=0,__de=0,__df=0,__dg=0,__dh;
static int __aho;	//debug


//------------------------------------------------------------------------------------------------------------------
//	class	implement
//------------------------------------------------------------------------------------------------------------------
//�f�t�H���g�R���X�g���N�^�������Ȃ��悤�ɂ��܂��B
static	vm_database	_vm_invalid;;	//�f�t�H���g�R���X�g���N�^�̃f�[�^�Q�Ƃł��B
cutline::cutline()	:	_vm(_vm_invalid)	{	_Assert(0,"cutline::cutline() default constructor not allowed");	}
cutline::cutline(vm_database &__vm) : _vm(__vm)
{
	_n_single = _n_multi = 0;		//�e���ޯ��
	//���������m�ۂ��Ă����܂��B
	HANDLE hheap = ::GetProcessHeap();	//�v���Z�X�q�[�v���擾����
	{
		_multi		=	(_multi_motion*) ::HeapAlloc(hheap ,HEAP_ZERO_MEMORY,_DEFAULT_MULTI_MOTION_SIZE);	//�v�[���̈���m�ۂ��܂��B
		_single		=	(_single_motion*)::HeapAlloc(hheap ,HEAP_ZERO_MEMORY,_DEFAULT_SINGL_MOTION_SIZE);	//�v�[���̈���m�ۂ��܂��B
		_Assert(_single	!=	NULL , "singl motion alloc failed(%s)" , LastErrorMsg() );
		_Assert(_multi	!=	NULL , "multi motion alloc failed(%s)" , LastErrorMsg() );
	}
	_n_allocated[_SINGL]	=	::HeapSize(hheap , 0, _single );	//
	_n_allocated[_MULTI]	=	::HeapSize(hheap , 0 , _multi );	//
	_n_single = _n_multi = 0;										//���݂̏������݃C���f�b�N�X�ł��B

	::InitializeCriticalSection(&_critsec);		//�����p�N���e�B�J���Z�N�V����
}
cutline::cutline( const cutline &a)	:_vm(_vm_invalid)
{
	_Assert(0,"cutline::cutline(cutline &a) : copy constructor not allowed(allocated memory transfer unsupported yet)");
	//toku �����Ή�����Ȃ�A���߂ă��������m�ۂ��āA���e���R�s�[���鏈���������Ȃ��ƂȂ�܂���B
	//		operator=�@���A���l�̗��R�ɂċ����Ȃ��悤�ɂ��܂��B
}
cutline::~cutline()
{	//�m�ۂ�����������������܂��B
	HANDLE hheap = ::GetProcessHeap();			//�v���Z�X�q�[�v���擾����
_printf("cutline: destruct: memoru free\n");
	::HeapFree(hheap ,0 , _single);	::HeapFree(hheap ,0 , _multi);
}
//--------------------------------------------------------------------------------------------------------------
//	���낢��Ȍ����L�[����A�}���`���[�V�����̓o�^�C���f�b�N�X��T���B
//--------------------------------------------------------------------------------------------------------------
//�w�肳�ꂽ�}���`���[�V�����̲��ޯ����Ԃ��܂��B
int	cutline::_index(const _multi_motion &m)
{
	int i;for(i=0 ; i < _n_multi ; i++ ) { if( m == _multi[i])	return i;	}
	_Assert(0 , "cutline::index() : index not found" );
	return -1;	//
}

//tick�ɂ�����A_multi[]�ɋL�^����Ă��铮���
int	cutline::_index(double tick)
{

#if	1	
	DWORD	__t = ::GetTickCount();
#endif
	//���Ȃ炸tick�͂͂��߂ɋL�^���ꂽ����������ɂȂ��ƂȂ�܂���B
	_Assert( f_eq_less( _multi[0].t, tick) , "cutline::_index() : tick is before start(_multi[0] %lf), (tick:%lf)\n" , _multi[0].t , tick );
	_Assert( _n_multi > 0 , "cutline::_index() : no registration (_multi)");	//�o�^���Ȃ��ƂȂ�܂���B

	//�����o�^����Ă�����̂��ׂĂ������Ƃ�tick���w�����ꂽ�ꍇ�́A�Ō�̃C���f�b�N�X��Ԃ��܂��B
	if( tick > _multi[_n_multi-1].t) return _n_multi-1;

	//����ȊO�͓o�^�͈̔͂̒��ɂ���܂��̂ŒT���܂��B
//�����͈͂����߂����ȑO�Ɏw���̂������`�b�N�ƁA�C���f�b�N�X�̑Ή�������΂������J�n�n�_��
	//�����L���b�V������A�`�b�N�ɂ����Ƃ��߂�
	int search_start = _cache.least(tick);	//

	//
	int i;for(i=search_start ; i < _n_multi ; i++ ) {
		if( f_eq(_multi[i].t , tick) ==true)	{

#if	1	//
	{	__dg +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif
				return i;		
			}	//���S�Ɉ�v����ꏊ������΁A�����ł��B
		else if( tick < _multi[i].t)			{
#if	1	//
	{	__dh +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif
				return 	i-1;	}	//���S�Ɉ�v���Ȃ��Ƃ��Atick�𒴂����ꍇ�ɂ͂ЂƂO��Ԃ��܂��B
	}

	_Assert(0 , "cutline::index(doublt tick) : index not found" );
	return -1;	//
}
//--------------------------------------------------------------------------------------------------------------
//	���[�V�����Z�b�g�i�������j	���[�V�����̎擾
//--------------------------------------------------------------------------------------------------------------
//�w�����ꂽ�C���f�b�N�X�ł̃��[�V�����Z�b�g�����܂��B
_motion_set	&cutline::get_motion_set(_motion_set &set,int idx,bool complete_single_motion  /* = true */)
{
#if	1	
	DWORD	__t = ::GetTickCount();
#endif
	_Assert( idx <  _n_multi , "cutline;::get() illegal indx(%d/%d)" , idx , _n_multi );
	::EnterCriticalSection(&_critsec);
		set=_multi[idx];			//�w�����ꂽidx�̃}���`���[�V���������ɍ���Ă����܂��B
		//�V���O�����[�V�������R�s�[���Ă����܂��B
#if	1	//
	{	__dd +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif
		int i;for(i = 0 ;i < (int)set.flg.n_axis ; i++ ) {			//�o�^���ꂽ�������V���O�����[�V������o�^���܂�
			_single_motion s	=	_single[ set.index + i];		//
_Assert(s.axis >= 0  ,"get_motion_set(): no data (s.axis illegal:%d)" , s.axis);
			set.s[s.axis]	=	s;									//�V���O�����[�V�����z��̓Y�����͎��C���f�b�N�X�ł�
		}

#if	1	//
	{	__de +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif

//tokutoku
		if( complete_single_motion	)	_complete(set,set);		//�Ȃ��V���O�����[�V������
#if	1	//
	{	__df +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif

	::LeaveCriticalSection(&_critsec);
	return	set;
}
//toku ����͎��Ԃ��������Ⴄ
_motion_set	cutline::get_motion_set(int idx,bool complete_single_motion  /* = true */)
{
	_motion_set set;	return	get_motion_set(set,idx,complete_single_motion);
}
//toku �����
//�`�b�N���w�����āA���̂Ƃ��̃��[�V�����Z�b�g��Ԃ��܂��B
_motion_set	&	cutline::get_motion_set(_motion_set &set ,double tick )
{
	//toku tick��double�ł����A


	//	int idx	= _index(tick);	//toku ����͎w��tick�̒��O��index���A���Ă��܂��B
	//	��F	idx		  5				  6				  7				  8
	//			tick	10.0			20.0			30.0			40.0
	//											23.5
	//											�� tick
	//									  �� _index() return 6
//_motion_set	&cutline::get_motion_set(_motion_set &set,int idx,bool complete_single_motion  /* = true */)

//toku �J�b�g���̃`�b�N���i��ł��A�J�b�g�J�n���_�ł̃}���`���[�V�������A��܂��B
//�J�b�g�t���O��
#if	1	
	DWORD	__t = ::GetTickCount();
#endif
//tokutoku
if( f_eq(tick , 81925.0)==true)	{
	_printf("tick! %lf -> %lf\n" , _multi[0].t , tick);	
}
//toku ���������Ԃ����B�@
	get_motion_set(set , _index(tick),true);	//���g�����ׂĕ⊮���܂�
	//����ŁAtick�͒��߂��炸��Ă���\��������܂��B

#if	1	//
	{	__db +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif

	//tick�ɐi�߂�̂ł��B������tick���i�߂��Ƃ��ɁA�L�^�������̉��������������Assert�ɂȂ�܂��B
	//�����̓f�[�^�̋󔒕������A�����^���܂���stop�̕����������Ƃ΂���Ă��āA

	//���Ԃ�u���ς��ď������܂��B
	if( set.t < tick ) {	//����́A�w�����ꂽ�`�b�N���A�o�^�C���f�b�N�X�ƈقȂ��Ă���ꍇ�ɁA���̓����i�߂鏈���ł��B
		//�����^���̂��̂������āA��ɐi�߂܂��B
		set.t = tick;
		int i;for(i=_X ; i < _AXIS_IDX_MAX ; i++ ) {
			_Assert( ((set.s[i].phase==PHASE_CONST) || (set.s[i].phase==PHASE_STOP)),"get_motion_set(double tick): illegal phase(%d,%d)" , i, set.s[i].phase);
			if( set.s[i].phase == PHASE_CONST )	{		//�����^���̏ꍇ��
//toku ����ɂ����̎��ԐH���B�A�B
				calc_single_motion( set.s[i],i,tick);
			}
		}
	}

#if	1	//
	{	__dc +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif
	return set;
}


//���߂̃}���`���[�V������Ԃ��܂��B�i�߂�l�́A�V���O�����[�V�������܂ށA���[�V�����E�Z�b�g��Ԃ��܂�)
_motion_set&	cutline::get_last_motion_set(_motion_set &m , bool complete_single_motion/*=true*/ )
{
	if( _n_multi > 0 )	{		get_motion_set(m,_n_multi-1 ,complete_single_motion);	}	//�f�[�^������ꍇ�ɂ͕Ԃ��܂��B
	else m.invalid();	//�f�[�^���Ȃ��ꍇ�̓f�t�H���g�̖����ȃI�u�W�F�N�g��Ԃ��܂��B
	return m;
}
_motion_set	cutline::get_last_motion_set(void)	{	_motion_set set;	return	get_last_motion_set( set , true);	}
//���߂̃J�b�g���Ă������[�V������T���ĕԂ��܂��B
//�����F	
//		unsigned int	bit_assinged_z		:	�ޯĊ���t����	bit0:Z1 bit1:Z2
//		_motion_set		&m					:	
//bool	cutline::get_last_cut_motion_set(_motion_set &m , unsigned int bit_assinged_z /*=0x3*/ )
_motion_set	cutline::get_last_cut_motion_set(unsigned int bit_assigned_z/*=0x3**/)
{
	int i;for( i = _n_multi-1 ; i >= 0 ; i-- )	{	//
		if	( 	bit_assigned_z & 0x1	)	{	if ( _multi[i].flg.cut_z )	goto found;		}
		if	(	bit_assigned_z & 0x2	)	{	if ( _multi[i].flg.cut_w )	goto found;		}
	}
	return	_motion_set();	//�f�t�H���g�̖����ȃI�u�W�F�N�g��Ԃ��܂��B
found:
	return	get_motion_set(i);
//	m = get_motion_set(i);	return true;
}


//interval�\���̂�Ԃ��܂��B������Ȃ��ꍇ�ɂ́A������interval��Ԃ��܂��B
//�J�b�g�t���O�́A���x���������肷�邱�Ƃ����邩������܂���B
//
//	interval
//	---------+		 +-------+		 +------+	 +-------------
//			 |_______|		 |_______|		|____|			   
//	��		 												  ��
// start   								  					  end
//toku ����͎w�����ꂽch��line�������C����Ԃ�Ԃ��܂��B��������͎g���Ȃ��Ȃ�܂����B
//interval	cutline::get_a_line(int ch , int line , int search_start_idx/*=0*/)	const
cut_interval	cutline::get_a_line(int ch , int line , int search_start_idx/*=0*/)	const
{
	_Assert( search_start_idx < _n_multi , "get_a_line :: multi_" );
	cut_interval	r(ch ,line);		//interval��ch��line�͎w���ʂ�̂��̂ł��B

	int i;
//_search_start:-----------------------------------------------------------
	for( i=search_start_idx ; i< _n_multi ; i++ )	{	//���C���J�n�ʒu�������܂��B
		if(	((int)_multi[i].flg.ch == ch ) && ((int)_multi[i].flg.line == line)) {	r.start = i;goto _search_end;}
	}
	goto	_not_found;		//������܂���B
_search_end:
	for( 					; i< _n_multi ; i++ )	{	//���C���G���h�ʒu��T���܂��B
		if(	((int)_multi[i].flg.ch != ch) ||	((int)_multi[i].flg.line != line))	{
			r.end=i-1;	break;	}
	}	//toku �Ō�܂ŗ��Ă��܂����ꍇ�����C���G���h�Ƃ��܂��B
	{	if(r.end<0) r.end = _n_multi-1;	}	//�Ō�܂ŗ��Ă��܂����ꍇ�ɂ́A�Ō��o�^���܂��B
	return r;
_not_found:
	return cut_interval();
}
//�w�����ꂽ�J�b�g��Ԃ́A���̃J�b�g��Ԃ�Ԃ��܂��B
cut_interval	cutline::get_next_line(const cut_interval &prev_line)	const
{
	int idx;	if( (idx = (prev_line.valid() ? prev_line.end + 1 : 0)) < _n_multi ) 	{
		return	get_a_line(	(int)_multi[idx].flg.ch	, (int)_multi[idx].flg.line ,  idx);
	}
	//�����J�n�C���f�b�N�X�����߂܂��B�O��̃��C���̍Ō�̎�����J�n�ł��B
_error:
	return	cut_interval();
}

//�w�����ꂽ�͈͂ŁA�w�����ꂽ���̃J�b�g�͈͂������ĕԂ��܂��B
cut_interval	cutline::get_cut_part(const cut_interval &range , int z )	const
{
	//�^����ꂽ�J�b�g���C���̂����An�Ԗڂ̃J�b�g������Ԃ��܂��B
	cut_interval	r(range.ch,range.line);
	int i;bool cutting=false;
	for( 	i=range.start	;	i <= range.end ; i++ )	{
		if( !cutting )	{	if(	_multi[i].cut(z) == true )	{
//toku �����ŉ�������̎����؂肩�������B
			r.start = i;	cutting=true;		}}
		else			{	if( _multi[i].cut(z) == false)	{
//toku �����ŉ�������̎���
			r.end	= i;	goto _found;		}}
	}
	//�����܂ł��Ă��܂����猩�����Ă��܂���B
	return	cut_interval();
_found:
	return	r;
}
//n�Ԗڂ̐������擾���܂��B
cut_interval	cutline::get_cut_part(const cut_interval&whole_line , int z , int n)	const
{
	cut_interval	src = whole_line,r;
	int i;for( i =0 ; i<=n ; i++ , src.start = r.end ) {
		if( (r = get_cut_part(src,z)).valid() != true) {
			goto	_error;
		}
	}
	return r;
_error:
	return	cut_interval();	//�Ȃ��ꍇ�ɂ͖����ȋ�Ԃ�Ԃ��܂��B
}
//�J�b�g���C�����������ǂ����̔�������郂�W���[��
bool	cutline::operator==(cutline &lb)
{

#if	1	
	DWORD	__t;
#endif
//	int ch,line;
	cut_interval	line_a,line_b;	int idx;
	for( ; ; )	{
		line_a = get_next_line(line_a);		line_b = lb.get_next_line(line_b);
//		line_a = get_a_line(ch,line,(line_a.valid()?line_a.end:0));	line_b = b.get_a_line(ch,line,(line_b.valid()?line_b.end:0));
		//�����ȃ��C��������ꍇ�B
		if ( !line_a.valid() || !line_a.valid() )	{	if( ! line_a.valid() && !line_b.valid() )	goto _fin;	//	�����Ȃ��ꍇ�͏I���ł��B		
		//����́A�܂��Е��ɂ̓J�b�g���C�������݂���
			set_lasterr("(ch:%d,line:%d cut line invalid (%d,%d)", line_a.ch, line_a.line , line_a.valid() ,line_b.valid());	
			_printf("ch:%d,line:%d cut line invalid (%d,%d)", line_a.ch, line_a.line , line_a.valid() ,line_b.valid());	
			goto _error;
		}
		//�J�b�g���Ă���ch�Ƃ����C�����Ⴄ�ꍇ���`�F�b�N���܂�
		if( (line_a.ch != line_b.ch) || (line_a.line != line_b.line) )	{
			set_lasterr("ch:%d,line:%d/ ch /line is differrent  (b.ch: %d , b.line: %d)", line_a.ch, line_a.line , line_b.ch ,line_b.line);	
			_printf("ch:%d,line:%d/ ch /line is differrent  (b.ch: %d , b.line: %d)", line_a.ch, line_a.line , line_b.ch ,line_b.line);
			goto _error;
		}
//toku �`�����l���̔�r�J�n�Ƃ��ă_���v���܂��B
_printf("[ch:%d][line:%d]  A: [%d t(%d)]-[%d t(%d)]    B:[%d t(%d)] [%d t(%d)]\n ", line_a.ch, line_a.line 
,	line_a.start	,	(int)_multi[line_a.start].t		,	line_a.end		,	 (int)_multi[line_a.end].t	,
	line_b.start	,	(int)lb._multi[line_b.start].t	,	line_b.end		,	(int)lb._multi[line_b.end].t );

#if	1	
	__da=__db=__dc=__dd=__de=__df=__dh=__dg=0;	__t = ::GetTickCount();
#endif

//toku 1���C�����ɂ�
		//����Ƀ��C���̒��̐��������o���Ĕ�r���Ă����܂��B
		{
			bool	fin[2];	int n;for( n=0 , fin[0]=fin[1]=false ; ( (fin[0] !=true) || (fin[1]!=true) ) ; n++ )	{	//1���C�����̃J�b�g��Ԃ���Â]�����Ă����܂��B
//toku �f���A���J�b�g�̏ꍇ�ɂ́AZ1Z2�Ɨ����Ă݂Ă����܂��B
				int	z;for( z=0 ; z<2 ; z++)	{
					if (fin[z])		continue;	//�I����Ă��鎲�̕]���͂��Ȃ��B
					cut_interval a	= get_cut_part(line_a ,z , n );	cut_interval b	= lb.get_cut_part(line_b , z , n );

					//�����Ȑ���������ꍇ�B
					if( !a.valid() || !b.valid() ) {
						if( !a.valid() && !b.valid() )	{		fin[z] = true;continue;		}	//�����Ƃ��J�b�g���C����������܂���̂ŏI���ł��B
//						set_lasterr("(ch:%d,line:%d cut part invalid (%d,%d)", line_a.ch,line_a.line,a.valid() , b.valid());
						//toku �`�b�N��1ms�̊ԂɁA���[�N���肬��̏ꍇ�ɐ؂肱�݂��������Ȃ��ꍇ������B�i������Ƃ����؂�Ȃ��ꍇ�j
						//���́A�{���͂�����Ƃ����؂�Ȃ����A�{���͐؂�Ă��镔���������Ȃ��ƂȂ�܂���B
						//				��	add()�̂Ƃ��ɁA1ms���Ƃɑ������Ă��������ɃJ�b�g���C�������������Ȃ��B
						
						//�Ƃ肠�����A�Е��ɂ����Ȃ����C���ɂ��āA���̂悤�ɕ\������悤�ɂ��܂��B
//						_printf("(ch:%d,line:%d cut part invalid (%d,%d)", line_a.ch,line_a.line,a.valid() , b.valid());
//						goto _error;
					}

					//�`�b�N�̐����r���܂��B�������[�V�������L�^����Ă���Ȃ�A1�`�b�N��菬�����덷�ɂȂ��Ă���
					//�`�b�N���͖��Ȃ��Aend�̈ʒu��
//toku �����𖳎����Ă݂�B
//					if ( abs( a.n() - b.n() ) > 1 )	{	set_lasterr("ch:%d,line:%d cut part n() not same(%d,%d) ",line_a.ch, line_a.line , a.n() , b.n() );	goto _error;	}

//toku ������

					//�J�b�g�͈͓��̃`�b�N�̐��́A
					//�J�b�g��Ԓ��́A�܂������������[�V�����ł��邱�Ƃ��m�F���܂�
//�J�b�g��Ԃɂ����āA�`�b�N���Ƃɔ�r���Ă����܂��B
//_motion_set	&	cutline::get_motion_set(_motion_set &set ,double tick )

//�Е���i�L���łȂ��ꍇ�������܂��B
//					_interval<double>ta	{	( a.valid() ? (_multi[a.start].t	,	_multi[a.end].t)	,	(0,0)	};
//					interval<unsigned int>tb	{	( b.valid()	? (lb._multi[b.start].t	,	lb._multi[b.end].t)	,	(0,0)	};
//					unsigned int	tick_a		=	a.valid()?_multi[a.start].t	:0	,	tick_b		=	b.valid()?lb._multi[b.start].t	:0;
//					unsigned int	tick_a_end	=	a.valid()?_multi[a.end].t	:0	,	tick_b_end	=	b.valid()?lb._multi[b.end].t	:0;
					_interval<unsigned int>tick_a=t(a)	,	tick_b=t(b);
					unsigned int tick_a=(unsigned int)t(a).start , tick_b = (unsigned int)t(b).start;
					for( ;	(tick_a < (unsigned int)t(a).end) || (tick_b<(unsigned int)t(b).end ) ; )	{

//if( (line_a.ch==2) && (line_a.line ==5) ){	_printf("[%d][%d] [%d][%d]\n" , tick_a , tick_a_end , tick_b ,tick_b_end);}

						//�A���C�����g�ʒu���_�Ŕ�r���Ă����܂��Btoku ���̎��_�ŁA�����[�V�����̕⊮�����Ă͂����Ȃ��ł��B
						//�Ƃ����̂́AVM�i�}�V���f�[�^�A�f�o�C�X�f�[�^�j�́Athis->_line �ƁA b._line	���Q�Ƃ��郏�[�N�I�u�W�F�N�g��
						//VM�t�@�C�����Ⴄ�B���̂��߁A�����VM�̏󋵂������Ă��邩�ǂ������A�킩��Ȃ��̂ŁA
						//get_motion_set�Ƃ��Aalipos_orogin�Ȃ�_mac��_vev���g���悤�Ȋ֐��ł�
						//
						//��������
		//toku ���߂��B������VM���g���Ȃ��Balipos_origin���Ăׂ܂���B
		//����Ăׂ�B	vmdatabase::update()	�ɂāA�t�@�C�����X�V���邩��A���v���Ǝv���܂��B

		//toku �e���[�����B�̔�r�ł��B
		//		�Е��́AZ2���؂肱�ނ̂ł����͂��B

						_motion_set ma,mb;
						//toku �����A�����łǂ��炩�Е����I����Ă���ꍇ�ɁA�Е��ɂ����Ȃ����Ƃ��L�^����悤�ɂ��܂��B
						if ( tick_a == tick_a_end)	{	//A���I����Ă���B
if( (line_a.ch==2) && (line_a.line ==5) ){	_printf("only B[%d][%lf]\n" , tick_b , (double)tick_b );	}
							mb= lb.alipos_origin(lb.get_motion_set(mb , (double)tick_b));
							_printf("    -- [B only:end] tick[--][%d] idx[--][%d] [%c]                            /  pos[%8.3lf]spd[%8.3lf][%d] \n",tick_b ,lb._index(tick_b) , 'X' , mb.s[_X].pos , mb.s[_X].v , mb.s[_X].phase );
							++tick_b;continue;
						}
						if ( tick_b == tick_b_end)	{	//B���I����Ă���B
if( (line_a.ch==2) && (line_a.line ==5) ){	_printf("only A\n" );	}
							ma = alipos_origin(get_motion_set( ma , (double)tick_a));
							_printf("    -- [A only:end] tick[%d][--] idx[%d][--] [%c]                            /  pos[%8.3lf]spd[%8.3lf][%d] \n",tick_a  , 'X' , mb.s[_X].pos , mb.s[_X].v , mb.s[_X].phase );
							++tick_a;continue;
						}

#if	1	//
	{	__t = GetTickCount();	}
						get_motion_set( ma , (double)tick_a);		alipos_origin( ma);
//						{	ma = alipos_origin(get_motion_set( ma , (double)tick_a));
						lb.get_motion_set(mb , (double)tick_b);		lb.alipos_origin(mb);
//						mb= lb.alipos_origin(lb.get_motion_set(mb , (double)tick_b));
	{	__da +=  GetTickCount() - __t;	__t = GetTickCount();	}

							_Assert(ma.valid() && mb.valid() , "alipos_origin error(ch:%d / line:%d n:%d / %d ,%d)" , line_a.ch ,line_a.line , n , ma.valid() , mb.valid());
//						}
#else
						{	ma	= alipos_origin(get_motion_set(i_a,false));	mb= alipos_origin(lb.get_motion_set(i_b,false));
							_Assert(ma.valid() && mb.valid() , "alipos_origin error(ch:%d / line:%d n:%d / %d ,%d)" , line_a.ch ,line_a.line , n , ma.valid() , mb.valid());
						}
#endif
						//toku�������������[�V�������ǂ����̔���ł��B�덷1ms�i�`�b�N�̂��ꕪ�j�Ŕ��肵�܂��B
						//�J�b�g�����Ă��Ȃ����ɂ��Ă͔�������Ȃ��悤�ɂ��܂��B
						//�Ȃ߂炩�ƁA�ʏ�̏ꍇ�ŁA�����ʂ����ƂȂ邽�߁Ais_same��false��Ԃ��Ă��܂��B
						//������̓��삪�������ǂ����𔻒肵�Ă����܂��B
						//�����Ń��[�V�������A�قȂ�ꍇ�ɁAX���̐i�s�����ɉ����āA�ǂ��炩��҂悤�ɂ��܂��H

						//X���̈ʒu���r���āA�Ⴄ�ꍇ�ɂ́A�x���ق����A�Ǝ���
						{
							if( ma.s[_X].is_same_pos(mb.s[_X],1.0)!=true) {	//X���̈ʒu�����ƂȂ�ꍇ�A��Ɏ��Ԃ�i�߂܂��B�i�܂�)
								if(		((ma.s[_X].v >= 0 )&& (ma.s[_X].pos > mb.s[_X].pos))	
									||	((ma.s[_X].v < 0 ) && (ma.s[_X].pos < mb.s[_X].pos))	){		//a���i��ł���ꍇ
									//b�����i�߂܂��B���������߂Ȃ��ꍇ�́A�B�B�B	B��A�ɒǂ����ĂȂ��̂ŋL�^���܂�
									_printf("    -- [B only slow] tick[%d][%d] idx[%d][%d] [%c] pos[%8.3lf]spd[%8.3lf][%d] /  pos[%8.3lf]spd[%8.3lf][%d] \n",tick_a,tick_b, _index(tick_a) ,_index(tick_b) ,'X' ,ma.s[_X].pos , ma.s[_X].v , ma.s[_X].phase , mb.s[_X].pos , mb.s[_X].v , mb.s[_X].phase );
									if( ++tick_b > tick_b_end ){	tick_b=tick_b_end;	_printf("    -- [error] B do not reach to A \n");		}//(X�������B���Ȃ�)
									continue;
								} else {
									_printf("    -- [A only slow] tick[%d][%d] idx[%d][%d] [%c] pos[%8.3lf]spd[%8.3lf][%d] /  pos[%8.3lf]spd[%8.3lf][%d] \n",tick_a,tick_b, _index(tick_a) ,_index(tick_b) ,'X' ,ma.s[_X].pos , ma.s[_X].v , ma.s[_X].phase , mb.s[_X].pos , mb.s[_X].v , mb.s[_X].phase );
									if( ++tick_a > tick_a_end ){	tick_a=tick_a_end;	_printf("    -- [error] A do not reach to B \n");		}//(X�������B���Ȃ�)
									continue;
								}
							}
						}

		//���C���ɂ́A���葤�Ɣ�����������B
		//�ʏ��
		//�܂��A���[�V�����̋L�^�̃C���f�b�N�X�ł͈ʒu��
						//_X�ʒu�͓����ł����A���x���Ⴄ��������Ȃ�����
						{
							const char _axis[_AXIS_IDX_MAX] = {'X','Y','V','Z','W','T'	};	_Assert( sizeof(_axis)==_AXIS_IDX_MAX, "axis define illegal\n");
							int i;for(i=_X ; i < _AXIS_IDX_MAX ; i++ ) {
								if( (z==0)	&&	((i==_V) || (i==_W))	)	{	continue;	}	//Z1�̕]���̏ꍇ�ɂ�Z2���͕]�����Ȃ�
								if( (z==1)	&&	((i==_Y) || (i==_Z))	)	{	continue;	}
								if( ma.s[i].is_same_pos(mb.s[i],1.0)	!=	true	)	{	//�ʒu�̕]���ł��B
										_printf("    -- [pos        ] tick[%d][%d]idx[%d][%d] [%c] pos[%8.3lf]spd[%8.3lf][%d] /  pos[%8.3lf]spd[%8.3lf][%d] \n",tick_a,tick_b,_index(tick_a) ,_index(tick_b) ,_axis[i],ma.s[i].pos , ma.s[i].v , ma.s[i].phase ,mb.s[i].pos , mb.s[i].v , mb.s[i].phase );
								}
								if( ma.s[i].is_same_spd(mb.s[i],1.0)	!=	true	)	{	//���x�̕]���ł��B�����Ȃǂ̃t�F�[�Y�Ⴂ�����Ă���������
										_printf("    -- [spd        ] tick[%d][%d]idx[%d][%d] [%c] pos[%8.3lf]spd[%8.3lf][%d] /  pos[%8.3lf]spd[%8.3lf][%d] \n",tick_a,tick_b,_index(tick_a) ,_index(tick_b) ,_axis[i],ma.s[i].pos , ma.s[i].v , ma.s[i].phase , mb.s[i].pos , mb.s[i].v , mb.s[i].phase );
								}
							}
						}
		//				if( ma.is_same(mb , 1 ) !=true)	{	set_lasterr("ch:%d,line:%d motion not same", line_a.ch, line_a.line);	goto _error;	}
						//toku	�����܂ł����ꍇ�ɂ́A�����̃`�b�N���C���N�������g���܂��B
						//b�����i�߂܂��B���������߂Ȃ��ꍇ�́A�B�B�B
						if( ++tick_b > tick_b_end )	tick_b=tick_b_end;
						if( ++tick_a > tick_a_end )	tick_a=tick_a_end;
					}
				}
			}
		}
_line_fin:
_printf("ch:%d line[%d] ok __da[%d])(total) __db(motion_set(sub)[%d] __dc(motion_set other)[%d] __dd[%d] __de[%d] __df[%d] __dg[%d] __%h[%d]\n" , line_a.ch , line_a.line , __da , __db , __dc , __dd , __de ,__df, __dg ,__dh );
}
_fin:
	return	true;
_error:	//��v���Ȃ�
	//��v���Ȃ��ꍇ�A�ǂ̃��C���Ń_�������ڍׂɂ���K�v������
	return false;
}
//--------------------------------------------------------------------------------------------------------------
//	�V���O���i�P���j���[�V�����̎擾
//--------------------------------------------------------------------------------------------------------------
//�w�����ꂽ�C���f�b�N�X�ŁA�w�����ꂽ���̃V���O�����[�V������Ԃ��܂��A�Ȃ��ꍇ��false�ł��B
//toku ����́A�w�����ꂽ�C���f�b�N�X�ł̃V���O�����[�V������Ԃ����̂ł��B
//�ߋ��̃��[�V�����̎Q�Ƃ�Ԃ��Ă������̂ł�

//�C���f�b�N�X��Ԃ��o�[�W�����B	������Ȃ��ꍇ�ɂ�-1��Ԃ��܂��B
int	cutline::search_single_motion(int axis,int idx)
{
	_Assert( idx <  _n_multi		,	"cutline;::get_single_motion() illegal indx(%d/%d)" , idx , _n_multi );
	_Assert ( axis < _AXIS_IDX_MAX	,	"cutline::get_single_motion() axis illegal(%d)" , axis);
	::EnterCriticalSection(&_critsec);
	{
		int i;for(i = 0 ;i < (int)_multi[idx].flg.n_axis ; i++ )	{	//�o�^���ꂽ�������V���O�����[�V������o�^���܂�
			if( _single[ _multi[idx].index + i].axis == axis)	{
				::LeaveCriticalSection(&_critsec);
				return	_multi[idx].index + i;
			}
		}
	}
	::LeaveCriticalSection(&_critsec);
	return -1;	//������Ȃ��ꍇ�ɂ�-1�ł��B
}
//�I�u�W�F�N�g�Q�Ƃ��󂯂Ă���ɃR�s�[���ĕԂ��o�[�W�����B���ԓI�ɂ͕ς��Ȃ������B
_single_motion&	cutline::get_single_motion(_single_motion &s , int axis, int idx )
{
	int s_idx;	if ( (s_idx = search_single_motion(axis,idx)) < 0 ) {	return	(s=_single_motion());	}	return	s=_single[s_idx];
}
_single_motion	cutline::get_single_motion(int axis, int idx )	{	_single_motion s;	get_single_motion(s,axis,idx);	return s;	}
//�w�����ꂽ���ŁA�w�����ꂽ�`�b�N���炳���̂ڂ���
//�݂��������[�V�����̎�����found_tick�֋L�^���܂��B
int	cutline::search_last_single_motion(int axis,double tick , double *found_tick/*=0*/)
{
	int idx = _index(tick);						//�w�����ꂽ�`�b�N�̃C���f�b�N�X�ł��B
	int i;for( i = idx ; i >= 0 ; --i )	{		//�����̂ڂ��āA�V���O�����[�V�����̂���ꏊ��T���܂��B
		int s_idx;	if( (s_idx = search_single_motion(axis,i)) >= 0 ) {	//�݂�����
			if( found_tick )	{
				*found_tick = _multi[i].t;					//���������`�b�N�������߂��܂��B
				_Assert( found_tick >= 0 , "search_last_single_motion() found_tick is illegal");//�����͐̂łȂ��ƂȂ�܂���
			}
			return s_idx;
		}
	}
	//�����݂���Ȃ��ꍇ�́A�A�T�[�Ƃł��B���̃f�[�^�x�[�X�́A�K����ԏ��߂ɂ��ׂĂ̎��̃��[�V�����������悤�ɂ��܂��B
	//���̂��߁A�V���O�����[�V�����͕K��������O��Ƃ��܂��B
	_Assert( 0 , "cutline::search_last_single_motion() not found axis=%d" ,axis);
	return	-1;	//�����ȃC���f�b�N�X��
}

_single_motion	&cutline::	get_last_single_motion(_single_motion &s,int axis, double tick ,double *found_tick/*=0*/ )
{
	int idx = _index(tick);						//�w�����ꂽ�`�b�N�̃C���f�b�N�X�ł��B
	int i;for( i = idx ; i >= 0 ; --i )	{		//�����̂ڂ��āA�V���O�����[�V�����̂���ꏊ��T���܂��B
		//�C���f�b�N�X��������o�[�W�����ɂ��Ă݂܂��B
		if ( get_single_motion(s,axis,i).valid() == true)	{
			if( found_tick )	{
				*found_tick = _multi[i].t;					//���������`�b�N�������߂��܂��B
				_Assert( found_tick >= 0 , "get_last_single_motion() found_tick is illegal");//�����͐̂łȂ��ƂȂ�܂���
			}
			return s;
		}
	}
	//�����݂���Ȃ��ꍇ�́A�A�T�[�Ƃł��B���̃f�[�^�x�[�X�́A�K����ԏ��߂ɂ��ׂĂ̎��̃��[�V�����������悤�ɂ��܂��B
	//���̂��߁A�V���O�����[�V�����͕K��������O��Ƃ��܂��B
	_Assert( 0 , "cutline::get_last_single_motion() not found axis=%d" ,axis);
	return s=_single_motion();	//�����ȃV���O�����[�V������Ԃ��܂��B
}
_single_motion	cutline::get_last_single_motion(int axis, double tick ,double *found_tick/*=0*/ )
{
	_single_motion	s;	get_last_single_motion( s , axis ,tick ,found_tick);	return s;
}

//--------------------------------------------------------------------------------------------------------------
//	�V���O���i�P���j���[�V�����̕⊮	�i�o�^�̂Ȃ������̃V���O�����[�V�������v�Z�A�쐬����j
//--------------------------------------------------------------------------------------------------------------
//����`�b�N�ł�(�o�^�̂Ȃ�)�V���O�����[�V����	�i�P���̋���)	���v�Z���܂��B
_single_motion	&cutline::calc_single_motion(_single_motion &s ,int axis , double tick)
{
	double	found_tick;	//���߂ɋL�^���ꂽ���[�V�����̎����ł��B
#if	1
	s = _single[search_last_single_motion(axis,tick,&found_tick)];
#else
	get_last_single_motion( s , axis , tick , &found_tick);		//tick���炳���̂ڂ��čŌ�ɋL�^���ꂽ����
#endif

	_Assert( s.valid()== true , "calc_single_motion() single motion not found");//�Ȃ��ƃA�T�[�g���܂��B
	//�V���O�����[�V����������܂����B���߂̃V���O�����[�V�������󋵂𒲂ׂ܂��B
	if	( s.phase == PHASE_CONST )	{			//��葬�x�œ��삵�Ă���ꍇ�ɂ́A���̎����ł̃��[�V�������v�Z���܂��B
		s.pos	+=	s.v*(tick - found_tick);		//���܌��݂̈ʒu���v�Z���܂��B
		//toku ����́A��������ɂ���ĕ����������͂�
	}
	else if( s.phase==PHASE_STOP)	{;}	//�Ƃ܂��Ă���ꍇ�B�V���O�����[�V�����͕ω����Ă��܂���̂ł��̂܂܂�Ԃ��܂��B
	else	{	_Assert(0,"calc_single_motion:: illegal phase(%d)" , s.phase);	}
//����ȊO�̃��[�V�����͂��܂̂Ƃ���A�����Ȃ��悤�ɂ��܂��B
//�e���[�V�����́A�����E�����̏I���ɂ́A���Ȃ炸STOP��CONST����x�L�^�����悤�ɂ��܂��B
	return	s;	
}
_single_motion	cutline::calc_single_motion(int axis , double tick)
{
	_single_motion s;	calc_single_motion( s , axis , tick );	return s;
}
//���[�V�����Z�b�g�ɖ��o�^�̃V���O�����[�V�������A�⊮���܂��B
_motion_set	&cutline::_complete( const _motion_set &org , _motion_set &dst )
{
//���ǂ����ŃR�s�[������܂��邪�B�E�E
//toku ������copy���Ȃ���
	//src��dst�������Ȃ�
	if(	&org	!=	&org) {
		dst = org;	//�܂��R�s�[
	}
//_single_motion	&cutline::calc_single_motion(_single_motion &s ,int axis , double tick)

	int i;for( i=0; i < _AXIS_IDX_MAX ; i++ ) {
//		if( org.s[i].axis == -1 )	{	dst.s[i] = calc_single_motion(i , org.t);		}	//�o�^�Ȃ��̏ꍇ�v�Z����B
//�Q�Ƃ�n���悤��
		if( org.s[i].axis == -1 )	{	calc_single_motion(dst.s[i] , i , org.t);		}	//�o�^�Ȃ��̏ꍇ�v�Z����B
//		else						{	dst.s[i] = org.s[i];							}	//�R�s�[
	}
	dst.flg.n_axis = _AXIS_IDX_MAX;	//�O�����̃f�[�^������܂��B
	return dst;
}
//�R�s�[���Ȃ��i�������g������������

//toku ����͎��Ԃ�������̂�
_motion_set	cutline::_complete( const _motion_set &org)
{
	_motion_set s;	_complete( org , s );	return s;
/*
	_motion_set	set = org;	//�܂��R�s�[
	int i;for( i=0; i < _AXIS_IDX_MAX ; i++ ) {
		if( set.s[i].axis == -1 )	{	set.s[i] = calc_single_motion(i , org.t);		}	//�o�^�Ȃ��̏ꍇ�v�Z����B
	}
	set.flg.n_axis = _AXIS_IDX_MAX;	//�O�����̃f�[�^������܂��B
	return set;
*/
}
//--------------------------------------------------------------------------------------------------------------
//	�V���O�����[�V�����̓o�^
//--------------------------------------------------------------------------------------------------------------
//�⊮���ꂽ�Ƃ��āA���̃��[�V�������Ach�Ȃɂ́A�����C���ځA�Ƃ����̂�
//����CH�́A������������x��������B
//���C���́A
//	Y��	�A

//�}���`�A���[�V�����ɂāA
//	X����������J�n���āA��x�Ƃ܂�܂ŁB���ꂪ�ꃉ�C���ł��B
//	�t���[����щz���ȂǂŁA��x�Ƃ܂�B
//	Z���グ�āA�ēxX�����삵�AZ�������āAX�����삷��B
//	���̂Ƃ��ɁA�J�b�g�t���O���ēx�o���܂��B
//	���̂����ɁA�������C�����ǂ����𔻒肷��̂ɁA�O�ɃJ�b�g�t���O�������Ă����}���`���[�V������T���A
//	���̎��́AYV���ʒu�A

//	Y���AV���̈ʒu���ς�炸�AX���̕������ꏏ�ł���΁A�����C���Ƃ݂Ȃ��B
//	�����C���ɂāA
//	

//	��	�����B�A�B�B�؂肱��ł���̂ɁAY���ɉ�����A�����A�Ȃǈʒu���ς��悤�Ȃ��Ƃ�����ꍇ
//		������ǂ��Ƃ炦�邩�A����̓G���[
//
//		�J�b�g�t���O���o�����ςȂ��̂Ƃ���
//		YV�𓮂������Ƃ������Ă��A����́A�������C���ł��B�i�G���[�����o���邩�H�j
//		
//		�������C�����ǂ����̔���́A
//


//���[�V�����Z�b�g���f�[�^�x�[�X�֓o�^���܂��B
void	cutline::add( const _motion_set &set)
{
	_Assert( _n_multi	< allocated(_MULTI)	,	"cutline::add(motion_set) _n_multi overflow");	//
	_Assert( _n_single	< allocated(_SINGL)	,	"cutline::add(motion_set) _n_single overflow");	//

	::EnterCriticalSection(&_critsec);		//���[�V�����E�Z�b�g��o�^���܂��B
		_multi[_n_multi++] = (_multi_motion&)set;												//multi
//toku set.s[xx]�́A�ƂтƂтœ����Ă��܂��B���Œ�ɂē����Ă��܂��B������̂ق��������₷���Ǝv���̂�
		int	i_s, i;for(i_s=0,i=0 ; i <(int)set.flg.n_axis ; i++ ) {
			//�ۑ�����V���O�����[�V������T���܂��B
			for( ; i_s < MOTION_SET_N_SINGLE ; i_s++) {
				if( set.s[i_s].axis != _INVALID_AXIS ) { _single[_n_single++] = set.s[i_s];	i_s++;goto _next_axis; }
			}
			_Assert( 0 , "cutline::add(motion set): single motion not enough(flg.n_axis=%d)" , set.flg.n_axis);
_next_axis:;
		}	//single
	::LeaveCriticalSection(&_critsec);
}
//���鎞���̃}���`���[�V�������L�^���܂��B
//���̎��_�ł͂U�����f�[�^
bool	cutline::add( double t , _MOTION org[] , bool cut_z  , bool cut_w )
{
#if	1	
DWORD	__t;
#endif

	::EnterCriticalSection(&_critsec);

	//����̃}���`���[�V�������샊�܂��B
	_motion_set	now(t);		//toku �����

#if 1//
	{if(cut_z || cut_w ) {
		__aho = 1;
	}
	if(cut_z){
		__aho=2;
	}
	}
#endif

	//����؂��Ă��邩�ǂ���
	{
		now.flg.cut_z	=	cut_z;		//�w�肳�ꂽ�J�b�g�t���O
		now.flg.cut_w	=	cut_w;		//�w�肳�ꂽ�J�b�g�t���O
		//�L�^���鎲�����߂܂��B	�������ŁA�L�^���邽�߂ɁA
		//�L�^���̏����F
		//		�@	�}���`���[�V�������͂��߂ċL�^�����ꍇ�B
		//		�A	�L�^��2�x�ڈȍ~�ɂāA���������s���Ă���ꍇ�B(�����E�����̍Ō�ɁA���I�ȓ���ɓ���A���̈�x�ڂ�����o�^����)
#if	1	
__t = ::GetTickCount();
//toku �����́A��葬�x��~�܂��Ă��郂�[�V���������I������A���[�V�����������Ă���ꍇ�͍Ō�̃��[�V�������A
//���Ԃ̂����鏈���Ƃ���΍Ō�̃V���O�����[�V�����̌���
#endif
		int axis;for(axis = 0 ; axis < _AXIS_IDX_MAX ; axis++)	{
			if( _n_multi > 0 ) {	//�ŏ��̓o�^�ł͂Ȃ��ꍇ�A���I�ȓ���͏d�����ċL�^���Ȃ��悤�ɂ��܂��B
				if( ( org[axis].phase == PHASE_STOP) || ( org[axis].phase == PHASE_CONST) )	{
#if	1				//�ȉ��ɂ���Α���
					_single_motion	&s = _single[search_last_single_motion(axis,t)];
#else
					_single_motion	s = get_last_single_motion(axis,t);	//�Y�����ŁA�Ō�ɓo�^���ꂽ���[�V������T���܂��B
#endif

					if( (s.phase==PHASE_STOP) || (s.phase ==PHASE_CONST) )	{continue;}	//�����ꂪ�ASTOP�ł���΁A�d�����ēo�^���Ȃ��B
				}
			}
			//�����I�_�A�����I�_�ł�PHASE_STOP�APHASE_CONST�͓��ꂽ�ق���
//toku �����Ŏ�������t����K�v�����邩�B������̂ق����킩��₷�������B�E�E�E
			now.s[axis]	=	_single_motion( (_AXIS_IDX)axis , org[axis] );
			now.flg.n_axis++;
		}


		//��������؂肱�݂�����ꍇ�́A�J�b�g���C������肵�܂��B
		//�J�b�g���C�������낤���A�Ȃ��낤���A��������̃J�b�g���C���ɑ����܂����H
		//����I���`�߂�`���̃��C���̎n�܂�܂ł́A�O��̃��C���ɑ����邱�ƂɂȂ�܂��B
//toku �����������Ȍ��ɂȂ�Ȃ���
//toku is_same_ch/is_same_line���������ƁB

		//�؂肱�݁i�t���O�j������ꍇ�ACH��LINE����肵�܂��B�؂肱�݂��Ȃ��ꍇ�́A���߂̃��[�V�����Ɠ����ɂ��܂��B
//���̃��[�V�����ł�ch�́A0���琔�����A�����ƂȂ�܂��B
//���ۂ�CH�́AAB���[�h�̏ꍇ 1, 2 , ��޲��ޯ���̏ꍇ�ACUT_CH
#if	1	//now.s[]�ŏ������ނׂ����i�������j�݂̂�
{	__ca +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif
		{
			_motion_set last;						//���߂̃J�b�g���Ă��郂�[�V�����ł��B
			_motion_set _c_now	= _complete(now);	//�v�Z�̂��߁A�S�����⊮����Ă��錻�݂̃��[�V�����Z�b�g��p�ӂ��܂��B
			if (_c_now.flg.cut_z || _c_now.flg.cut_w) {						//����Ȃɂ�����̐؂荞�݂�����ꍇ
				//�J�b�g���Ă���ꍇ�ɂ́Ach������ł���󋵂ɂ���͂��ł��i�̂Ō�����Ȃ��ꍇ�ɃA�T�[�g�Ƃ��܂�)
				int ch = cut_ch(_c_now.s[_T]);	_Assert(ch >= 0 , "cutline::add() cut_ch not found T_cnt=%lf", _c_now.s[_T].pos );
				if(	(last=get_last_cut_motion_set()).valid() == true ) {	//���߂̃J�b�g���C�����擾�ł�����
					if( _c_now.is_same_ch(last) == true ) {
						now.set(	ch	,	last.flg.line + (_c_now.is_same_line(last)?0:1)	);	//�������C���łȂ��ꍇ�̓C���N�������g�ł�
					} else {	//�ႤCH�ɂȂ����悤�ł��B
#if	1	//	
		__aho=0;
#endif
						now.set( ch, 0);		//����CH��
					}
				}	else{	//���߂̃��C�����݂���Ȃ��ꍇ�ɂ�CH�̈�ԏ��߂Ƃ��܂��B
					now.set( ch, 0);
				}
#if	1	//�����؂肱��ł���ꍇ��ch�ƃ��C������肷��B�Ƃ���A�����͎��Ԃ�����Ȃ��B
{	__cb +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif
			} else { //�J�b�g���C�����Ȃ��ꍇ�́A���߂̃��[�V�����Ɠ����ɂ��܂��B�H
#if	1	
__t = ::GetTickCount();
//toku �����́A��葬�x��~�܂��Ă��郂�[�V���������I������A���[�V�����������Ă���ꍇ�͍Ō�̃��[�V�������A
//���Ԃ̂����鏈���Ƃ���΍Ō�̃V���O�����[�V�����̌���
#endif
//				last=get_last_motion_set();			//�����񂩂���		total -> t[891998]ms 	complete����
													//					total -> t[919267]ms 	
													//		_compare���Q�Ɣł�	��		[872201]ms 

//				get_last_motion_set(last);			//�����񂩂����	total -> 		t[476785]ms 	complete����
													//		_complete���Q�Ɣł�	��		t[472168]ms		�������ĕς��Ȃ��B
													//		_complete���C���f�b�N�X��	t[579013]ms 	�悯���Ɏ��Ԃ�����E�E�H

#if	0			//run() total -> t[58501]ms t[71106]ms 
				if( get_last_motion_set(last,false).valid() == true) {	now.set( last.flg.ch , last.flg.line );	}
				else	now.set(0,0);		//���߂̃��[�V�������݂���Ȃ��ꍇ�ɂ�
#else
				//�ȉ��̃R�[�h��	t[61371]ms 		
				if(_n_multi > 0 ) {
					now.set( _multi[_n_multi-1].flg.ch	,	_multi[_n_multi-1].flg.line	);
				}else{		now.set(0,0);	}		//���߂̃��[�V�������݂���Ȃ��ꍇ�ɂ�
#endif

#if	1	
{	__cc +=  GetTickCount() - __t;	__t = GetTickCount();	}	//toku �������x�z�I�̂悤
#endif
			}
		}
	}

//toku �����܂ł����Ԃ�����-----------------------

	//toku ���̎��_�ŁA���삵�Ă���B�E���ĂȂ��Ɋ֌W�Ȃ�6���S���̃��[�V�������͂����Ă��܂��B
	//toku �����Œ��߂̓��삪set�ɓ����Ă��܂��B
	//�������݂����邩�ǂ����𔻒f���܂��B
	{
		//���߂Ă̏������݂̏ꍇ�ɂ́A�����܂��B
		if( _n_multi == 0 ) 										goto _write;

		//�O�񂩂�J�b�g�t���O���ς���Ă��邩�ǂ����B
		if( _multi[_n_multi-1].flg.cut_z != now.flg.cut_z)	{
//�J�b�g���C���̂͂���E�o���ł́A���[�V���������̂��߂ɁA���̋L�^������Ă��Ȃ��ꍇ������܂��B
_printf("ch�F%dline:%d (%d->%d)\n" , now.flg.ch, now.flg.line ,_multi[_n_multi-1].flg.cut_z ,  now.flg.cut_z);
			now	= _complete(now);	//�S����⊮���܂��B
			goto _write;
		}
		if( _multi[_n_multi-1].flg.cut_w != now.flg.cut_w)	{
			now	= _complete(now);	//�S����⊮���܂��B
			goto _write;
		}

//�t���O�̈Ⴂ�ŏ����ꍇ�A������������A�V���O�����[�V�����Ƃ��ĂȂɂ��o�^���Ȃ��ꍇ�����邩������܂���B
		//���̂ق��A�L�^����ׂ���������ꍇ�ł��B
		//���łɃV���O���m�[�h������Ă���ꍇ�ɂ́A
		if(	now.flg.n_axis )										goto _write;
	}
	::LeaveCriticalSection(&_critsec);
	return	false;
_write:

	//����̏������݃C���f�b�N�X��	_n_multi/ _n_singl�ł��B
	//���̎��_�ŁAmotion_set�̃V���O�����[�V�����ւ̐擪�C���f�b�N�X�����܂�܂��B
	now.index =	(now.flg.n_axis>0)?_n_single:0xffffffff;	//�V���O���̓o�^���Ȃ��ꍇ��INVALID�Ƃ��Ă����܂��B
	add(now);
	::LeaveCriticalSection(&_critsec);


	return true;
}
//���[�V�����Z�b�g���A�A���C�����g�ʒu����̑��΂Ƃ��܂�
//T�̃J�E���^����A�J�b�g�`�����l�����v�Z���ĕԂ��܂��B
//�e�`�����l���̃��̃J�E���^�l��Ԃ��܂��B
int	cutline::ch_theta_count(int ch , bool abs )
{
	_Assert( (ch>0) && (ch < MAX_CH) ,"ch_theta_count ch illegal(%d) " , ch);
	if (_vm.dev.sub_index() )	
	{	return	(abs?_vm.mac.homepos['T']:0)	+ _vm.dev.post_ch[ch] +	_vm.mac.ali_post[ch];	}
	return	(abs?_vm.mac.homepos['T']:0) + ( (ch==2) ? 90000000 : 0 ) + _vm.mac.ali_post[ch];
}
//�Ƃ̃J�E���^�l����Ach��Ԃ��܂��B
//�ǂ̃`�����l���ɂ������Ȃ��ꍇ�́A-1��Ԃ��B
int cutline::cut_ch(_MOTION t)
{	//�t�F�C�Y���A������~��ԂłȂ��ꍇ�ACH�͂킩��܂���B�i�ߓn���)
	if(	t.phase!=PHASE_STOP	)	goto	_not_found;
	//�J�E���^�l��	1���ȓ��Ȃ�΁A����ch�Ƃ��܂��B
	int ch;for(ch=1 ; ch < MAX_CH ; ch++ ) 	{
		if( f_eq((double)ch_theta_count(ch),t.pos  , 1000000)==true) {	return ch;	}
	}
_not_found:
	return -1;
}
//���[�N�Z���^�[�I���W�����A���C�����g�ʒu���motion_set�ɕύX���܂��B
_motion_set&	cutline::alipos_origin(_motion_set &m)
{
	int ch,ali_posy;
//#if	1
//	DWORD	__t = ::GetTickCount();
//#endif
	_vm.update((unsigned int)m.t);	//�}�V���f�[�^�����̃`�b�N�̂��̂ɂ��܂��B
//#if	1
//	{	__dc +=  GetTickCount() - __t;	__t = GetTickCount();	}
//#endif

	if( (ch = cut_ch(m.s[_T])) < 0 )	{	//CH��������Ȃ�
		//�����ŁA�킩��Ȃ��ꍇ�ɂ́A�\�z����CH��Ԃ��Ă�����
		goto _error;
	}
	//toku (�b��)ali_post[ch]��0�̏ꍇ�́A���[�N�Z���^�[�Ƃ��܂��B(AlI_POSY�́A��Έʒu�ł��B0�̏ꍇ�ɂ́ACUT�v���O�����ɂ�ALU_CY_HI�ɕ⊮���邵���݁j
	ali_posy = (_vm.mac.ali_posy[ch]==0) ? _vm.mac.alu_cy_hi : _vm.mac.ali_posy[ch];

	m.s[_Y].pos	+=	_vm.mac.alu_cy_hi - ali_posy;		//
	m.s[_V].pos	+=	_vm.mac.alu_cy_hi - ali_posy;		//

//	m.s[_Y].pos	+=	_vm.mac.alu_cy_hi - _vm.mac.ali_posy[ch];		//
//	m.s[_V].pos	+=	_vm.mac.alu_cy_hi - _vm.mac.ali_posy[ch];		//

//#if	1
//	{	__dd +=  GetTickCount() - __t;	__t = GetTickCount();	}
//#endif

	return	m;
_error:
	return	_motion_set();
}
//---------------------------------------------------------------------------------------------------
//		class/struct	implements
//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------
//	_single_motion
//---------------------------------------------------------------------------------------------------
//t_ms ms��̃��[�V������\�����܂��B�i���m�ł͂���܂���)����덷�̌v�Z�̂��߂Ɏg���܂��B
_single_motion	_single_motion::predict(double t)	const
{
	_single_motion s = *this;	//�܂����g���R�s�[
	//	int		phase;	double	pos;	double	v;	double a;
	//	toku �P���ɁA���xV�A�����xA�ɂāA�����I�ɉ��������ꍇ�́A���x�ƕψʂł��B
	if( t <	0 ) {		//�}�C�i�X�̏ꍇ���Ԃ������̂ڂ�
		t*=-1;			//���������ɖ߂��Čv�Z���܂��B
		s.v			= 	v	-	a*t;
		s.pos		=	pos	-	(s.v*t	+	(a*t*t/2));		//�����x�́A�ߋ���
	} else {				//�v���X�̏ꍇ�B
		s.v			=	v	+	(a*t);
		s.pos		=	pos +	(v*t	+	(a*t*t/2));		//
	}
	//��toku �ڕW�ʒu�𒴂��āAphase���{���͂Ƃ܂�ꍇ�����邩������Ȃ��B
	return s;		//
}

//
int		_single_motion::is_same_pos(const _single_motion &a,int gosa_ms) const
{
	_single_motion	s;	double max,min;

	//	============================	�ʒu	=================================
	{	s= predict(gosa_ms);	max = __max(pos , s.pos);	min = __min( pos , s.pos);	}	//gosa_ms���i�߂܂��B
	//���̒��ɁA�����Ă̈ʒu�����݂���΁A����͓����ʒu�Ƃ݂Ȃ��܂��B
	if( f_eq_less(a.pos,max,1.0) && f_eq_less(min,a.pos,1.0)	)	goto _in_range;
	//�������́A����������̌덷�͈͓��ɂ���ꍇ�͓����ʒu�Ƃ݂Ȃ��܂��B
	{	s= a.predict(gosa_ms);	max = __max(a.pos , s.pos);	min = __min( a.pos , s.pos);	}	//gosa_ms���i�߂܂��B
	if( f_eq_less(pos,max,1.0) && f_eq_less(min,pos,1.0))			goto _in_range;
	return	false;
_in_range:
	return	true;
}
int		_single_motion::is_same_spd(const _single_motion &a , int gosa_ms) const
{
	_single_motion	s;	double max,min;
	//	============================	���x	=================================
	{	s= predict(gosa_ms);	max = __max( v , s.v);	min = __min( v , s.v);	}	//gosa_ms���i�߂܂��B
	//���̒��ɁA�����Ă̈ʒu�����݂���΁A����͓����ʒu�Ƃ݂Ȃ��܂��B
	if( f_eq_less(a.v , max ,1.0) && f_eq_less(min,a.v,1.0))	goto _in_range;
	//�������́A����������̌덷�͈͓��ɂ���ꍇ�͓����ʒu�Ƃ݂Ȃ��܂��B
	{	s= a.predict(gosa_ms);	max = __max(a.v , s.v);	min = __min( a.v , s.v);	}	//gosa_ms���i�߂܂��B
	if( f_eq_less(v,max,1.0) && f_eq_less(min,v,1.0))			goto _in_range;
	return false;
_in_range:
	return true;
}
//
bool	_single_motion::is_same(const _single_motion &a,int gosa_ms) const
{
	//�덷 gosa_ms�ȓ��œ������W�ɂ��邩�ǂ�����]�����܂��B
	//����
	if(is_same_pos(a,gosa_ms)==true) {
		if(is_same_spd(a,gosa_ms)==true){	goto _in_range;	}
	}
	return false;
_in_range:
	return true;
}

//---------------------------------------------------------------------------------------------------
//		_multi_motion	(struct)	imprement
//---------------------------------------------------------------------------------------------------
//�f�t�H���g�R���X�g���N�^�Ƃ��Ė����ȃI�u�W�F�N�g�����܂��B
_multi_motion::_multi_motion()				{	set( 0 , -1 ,0,false,false,0,0);		}
_multi_motion::_multi_motion( double _t)	{	set(_t	, -1 ,0,false,false,0,0);		}
void	_multi_motion::set( double _t, unsigned int _index,int _n_axis , bool _cut_z , bool _cut_w,int ch,int line)
{
	t=_t;	index = _index;		flg.n_axis=_n_axis;	flg.cut_z = _cut_z;	flg.cut_w = _cut_w;	set(ch,line);
}
//ch��line�̃Z�b�g�ł��B
void	_multi_motion::set(int ch, int line)	{	flg.ch=ch;flg.line=line;	}
bool	_multi_motion::operator==(const _multi_motion &a)	const
{
	if(	! f_eq(t,a.t) )	return	false;		//�`�b�N����v
	if(	a.index			!=	index			)	return	false;		//�V���O�����[�V�����v�[���ւ̃|�C���^����v
	if( a.flg.n_axis	!=	flg.n_axis		)	return	false;		//
	if(	a.flg.cut_z		!=	flg.cut_z		)	return	false;
	if( a.flg.cut_w		!=	flg.cut_w		)	return	false;
	if( a.flg.ch		!=	flg.ch			)	return	false;
	if(	a.flg.line		!=	flg.line		)	return	false;
	return true;
}
bool	_multi_motion::valid(void)const
{
	//�V���O���C���f�b�N�X�̓o�^���Ȃ��E//�J�b�g�t���O�̓o�^���Ȃ��Ɩ����ł��B
	if(	(index==-1)	&& (flg.n_axis==0) )	{if(( flg.cut_z==0) && ( flg.cut_w==0))	{return false;}}
	return	true;	//����ȊO�͗L���ł��B
}
_multi_motion&	_multi_motion::invalid(void)	{	set(0, -1 ,0,false,false,0,0);return *this;	}
//---------------------------------------------------------------------------------------------------
//		_motion_set	(struct)	imprement
//---------------------------------------------------------------------------------------------------
//�w�����ꂽmulti_option����ɂ����R���X�g���N�^
//toku ���̕��@�ł悢�̂��ǂ����悭�킩��Ȃ��B
_motion_set::_motion_set(_multi_motion &m)
{	set(m.t , m.index , m.flg.n_axis , m.flg.cut_z , m.flg.cut_w , m.flg.ch , m.flg.line );	}
//���̃��[�V�����Z�b�g�ƁA����CH���ǂ����̔���ł��B	�Ǝ��̃��[�V�������݂܂��B
//��
bool	_motion_set::is_same_ch(const _motion_set &last)
{
	//�Ǝ��̃V���O�����[�V����������K�v������܂�
	_Assert(   s[_T].valid()==true  , "is_same_ch() :   theta axis invalid");
	_Assert( last.s[_T].valid()==true  , "is_same_ch() : last theta axis invalid");
	//�덷3�����炢�͓���CH�Ƃ݂Ȃ��܂�
	if( f_eq( s[_T].pos , last.s[_T].pos , 3 ))	return	true;
	return false;
}
//�i���C���ԍ��̔���p	�F	���n��ɂă��C�����ς�������ǂ����j
//�ߋ��̃��[�V�����ƁA���݂̃��[�V�����i*this)���A�������C���ɑ����Ă��邩�ǂ�����Ԃ��B
bool	_motion_set::is_same_line(const _motion_set &last)
{
	bool	same_z=false,same_w=false;		//X���AZ���AY���AZ���͕K���Ȃ��ƂȂ�܂���B
	_Assert( s[_X].valid() && s[_Y].valid() && s[_V].valid() && s[_Z].valid() && s[_W].valid() ,"is_same_line error");
	_Assert( last.s[_X].valid() && last.s[_Y].valid() && last.s[_V].valid() && last.s[_Z].valid() && last.s[_W].valid() ,"is_same_line ");

	//X���������Ă��邩�ǂ����́A
	//�P���J�b�g�̏ꍇ�ɁA�J�b�g���Ă��鎲�����]�����Ȃ��悤�ɂ��Ȃ��ƂȂ�Ȃ��A�i�����Е��̎��̓t���[�j
	//�����ł����͂��ɃJ�b�g����悤�ȏꍇ�́A�����Ă���ق��̎�������Y�ʒu���L�[�v���Ă��Ȃ���΁A�������C���Ƃ݂͂Ȃ���Ȃ��B
	//��toku �����́A���낢��ȃJ�b�g�������Ă݂āA�s�s��������ΕύX���Ă����悤�ɂ��Ȃ��ƂȂ�Ȃ���

	const double d = .005;		//double�̔����ł��B5��m�ȓ��̏ꍇ�ɓ���Y���ʒu�Ƃ��܂�

	if( flg.cut_z ) {	//����Z1���؂荞��ł���ꍇ�A�O���Z�ʒu�𔻒肵�܂��B
		if(	f_eq( s[_Y].pos , last.s[_Y].pos , d )	== true	)	{	same_z=true;	}
	}else {same_z=true;}
	//
	if( flg.cut_w ) {	//����Z1���؂荞��ł���ꍇ�A�O���Z�ʒu�𔻒肵�܂��B
		if(	f_eq( s[_V].pos , last.s[_V].pos , d )	== true	)	{	same_w=true;	}
	}else {same_w=true;}

	return	(bool)((same_z==true) && (same_w ==true) );
}
_motion_set	&_motion_set::invalid(void)
{
	_multi_motion::invalid();		//���N���X��invaid()
	int i;for(i=0 ; i < MOTION_SET_N_SINGLE ; i++) {s[i].invalid();	}
	return	*this;
}

//�i��̃J�b�g����̔�r�p)
//��̃��[�V����������덷�i���ԓI�ȁj�ȓ��ň�v���Ă��邩�ǂ�����Ԃ��܂��B
//������́A1�`�b�N��(1ms�j�덷�������܂��B

//���ɂ���Ă��̕��������Ⴄ�̂ŁA
//bool	_motion_set::is_same(const _motion_set &a ,int gosa_ms)	//����v���덷	gosa_ms[ms]���ȓ��ň�v���Ă��邩�ǂ���
//{
//	//�������A�����Ă����������l�̏ꍇ�A
//	//�����{1�`�b�N��̈ʒu�̊ԂɁA	���肪����
//	int i;for(i=0 ; i < _AXIS_IDX_MAX ; i++ ) {
//		if(	s[i].is_same( a.s[i],gosa_ms ) != true )	{
//_printf("not same: [%d] pos[%lf] pos[%lf] \n" , i , s[i].pos , a.s[i].pos );
//			return false;
//		}
//	}
//	return true;
//}
unsigned int	cutline::allocated(int type)
{
	if		( type == _SINGL )	{	return	_n_allocated[_SINGL] / sizeof(_single_motion);	}
	else if	( type == _MULTI )	{	return	_n_allocated[_MULTI] / sizeof(_multi_motion);	}
	else	{
		_Assert(0 ,"allocated() type illegal(%d)" , type);
	}
}
//�O���t�@�C������}���`�E�V���O�����[�V������ǂ݂����܂��B
//���W���[���ւ̕ύX�𔺂��̂ŁA���[�h��Ƃ͂��̃I�u�W�F�N�g�ōs���܂��B
//�Δ�Ƃ��āAsave()���p�ӂ����ق����������Ǝv�����̂ł����A���������͂������Ǝv���Ă�߂܂����B
bool	cutline::load(HANDLE fh,int n_multi,int n_single)
{
	//�t�@�C������f�[�^��ǂ݂܂��B�ꉞ�T�C�Y���`�F�b�N���܂��B
	_Assert(n_multi	< allocated(_MULTI)		, "cutline::load(): multi size oveflow");
	_Assert(n_single < allocated(_SINGL)	, "cutline::load(): single size oveflow");

	::EnterCriticalSection(&_critsec);	DWORD	r;
	if( ::ReadFile(fh,(void*)&_multi[0]		,	n_multi*sizeof(_multi_motion)	, &r,NULL) == 0 )	{	_msg( "cutline::load()file read error(0): [%s]" , LastErrorMsg());	goto	_error;		}
	if( ::ReadFile(fh,(void*)&_single[0]	,	n_single*sizeof(_single_motion)	, &r,NULL) == 0 )	{	_msg( "cutline::load()file read error(1): [%s]" , LastErrorMsg());	goto	_error;		}
	//�����܂ŗ��ꂽ��ǂ߂܂����B
	_n_multi = n_multi;
	_n_single = n_single;
	::LeaveCriticalSection(&_critsec);
	return true;
_error:
	::LeaveCriticalSection(&_critsec);
	return false;
}
_interval<double>	cutline::t(const _interval<int>& i)	
{
	_interval<double>	r (i.valid() ? _multi[i.start].t : 0 , i.valid()?_multi[i.end].t:0);
	return	r;
}
//double�̎��Ԕ͈́�unsigned int �̎��Ԕ͈�
_interval<unsigned int>cutline::t_uint(const _interval<int>&i)
{
	_interval<double>dr=t(i);
	_interval<unsigned int>	r ((unsigned int)dr.start , (unsigned int)dr.end);
	return	r;
}

//

//int	cutline::_add_singl_motion(int n,_MOTION _m_array[])
//{
//	int	sav_n_motion = _n_singl;		//�o�^�J�n���̃C���f�b�N�X�ł��B
//	_Assert( _n_singl_motion + n < allocated_singl_pool() , "set_motion() motion overflow");
//	_Assert( n>0 , "set_sgl_motion n==0" );
//	::EnterCriticalSection(&_critsec);
//		int i ;for(i=0 ; i < n ; i++ )	{	_single[_n_singl++]	=	_m_array[i];	}
//	::LeaveCriticalSection(&_critsec);
//	return	sav_n_motion;				//�擪�̃C���f�b�N�X��
//}
//----------------------------------------------------------------------------------------------------------------
//	�������[�V�����ǂ����̔���ɂ���
//----------------------------------------------------------------------------------------------------------------
//�`�b�N�̂Ƃ肩���ŁA���[�V�����������ɂ���邱�Ƃ�����
//	1ms - 1.5ms �ŁA���`�b�N�ł͂��邪�A�J�b�g�͈͂ɂ������������`�b�N���덷�������܂��B
//	���̏ꍇ�A1ms�̌덷��������Ƃ�������
//	�߂��肪1ms�̂��̂����ŁA����ڐ��̂Ƃ���ŃJ�b�g�͈͂ɂ������������B
//	�����Е��ł́A���̂���������čő�A1ms�̒x��ɂȂ�B
//	
//	�t���O��	ON	�^�C�~���O�́A�ő�1ms	
//				OFF	�^�C�~���O�́A�ő�1ms
//	���̂��́A�x���ꍇ������B
//	���̏ꍇ�A���Ƃ���100mm/sec�œ��삵�Ă��鎲�̏ꍇ�B
//	x = vt -> 100 * 0.001 = 0.1 mm (100��m)����邱�ƂɂȂ�

//	
//	�v���덷���{1ms�Ƃ��Čv�Z���܂��B
//	���̍ۂ́A�e���̓��쑬�x�ɉ����Č덷���v�Z���āA���̌덷���Ȃ��ł����
//	�덷�A�A
//	�Е��̃��[�V���������̃��[�V�����̈ʒu�A���x��

//�������[�V�������L�^���Ă���ꍇ�A
//	���ƂŌ��o�������[�V�������A
//		��s�̃��[�V�����@+�@�����[�V�����Ƃ̊Ԃɂ���Ƃ����Ă���Ƃ��܂��B
//		�����[�V�����̗\���́A���ۂ̎����[�V�����̏ꍇ�A�f�[�^���Ȃ��ꍇ�ɍ���̂ŁA
//		���݂̑��x�A�ʒu�A�����x����A�����[�V������\�����āA���̊Ԃɓ����Ă�����̂���
//----------------------------------------------------------------------------------------------------------------
