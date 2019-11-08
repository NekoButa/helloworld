#ifndef		__CUTLINE_H_INCLUDE__
#define		__CUTLINE_H_INCLUDE__

#include	"axis_obj.h"		//_MOTION
//#include	"vm_interface.h"	//VM
#include	"vm_database.h"		//VM�f�[�^�x�[�X
//------------------------------------------------------------------------------------------------------------------
//	defines
//	������g���[�X�p�A�N���X
//------------------------------------------------------------------------------------------------------------------
enum	_AXIS_IDX	{	_INVALID_AXIS=-1,	_X=0 , _Y , _V , _Z , _W , _T	,_AXIS_IDX_MAX	};		//����\���ԍ�
enum	{	_SINGL , _MULTI					};		//�V���O���E�}���`���[�V�����̂ǂ��炩��\��

#define	MOTION_SET_N_SINGLE	8			//motion_set�\���̂����V���O�����[�V�������B

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
//struct	_MOTION	{	int phase;	double	t;	double	pos;	double	v;	double a;	};
//�}���`���[�V�������Ǘ�����P�����[�V�����͎��Ԃ̗v�f���Ȃ��A�����̗v�f������܂��B
//���������̃��[�V�����L�^�ł��B

//�P���p���[�V�����L�^�ł��Baxis_obj��_MOTION�ցA�t������ǉ��������̂ł��B
struct	_single_motion	:	public	_MOTION
{
	_AXIS_IDX   	axis;	//������\���A�X�L�[�����ł��B
//	_MOTION			m;		//�����́A���x�̕����ŕ\���悤�ɂ��܂��B
	_single_motion()	{ axis	=	_INVALID_AXIS ;	}
	_single_motion(	_AXIS_IDX _idx,  const _MOTION &_m)	:	_MOTION(_m)	{axis = _idx;	}
	bool	valid(void)	const	 {	return (bool)(axis!=-1);		}
	void	invalid(void)		{	axis=_INVALID_AXIS;				}
	int		dir(void)	{	return	(v<0)	? -1 : 1;				}
	_single_motion	predict(double t)	const;
	bool	is_same(const _single_motion &a,int gosa_ms) const;
	int		is_same_pos(const _single_motion &a,int gosa_ms) const;
	int		is_same_spd(const _single_motion &a , int gosa_ms) const;

//	bool	in_range(const _single_motion &minus ,const  _single_motion &plus) const;

};	//	��size	:	28(sizeof(_MOTION)	+	4	=	32 byte
//�}���`���[�V�����i�����̎���\�����߂�)�I�u�W�F�N�g�B
//	�e�ʂ����炷���߂ɁA�e�V���O�����[�V�����i�P������)�́A���̃I�u�W�F�W�F�N�g�O���֋L�^����܂��B
//	
class	_multi_motion
{
protected:
public:	//�����o�[�͊�{�I�Ƀp�u���b�N�Ƃ��܂�
	double		t;						//����͋��ʂ̃`�b�N�Ƃ��܂�								4
	unsigned	int			index;		//���ꂪ�V���O�����[�V�����v�[���ւ̃|�C���^�ƂȂ�܂��B	4	�i�z��Y����)
	struct	{
		unsigned	int		n_axis			:	4	;		//�L�^���������ł��B4bit����15���܂ŁB
		unsigned 	int		cut_z			:	1	;		//���[�N�ɐ؂肱��ł���B
		unsigned 	int		cut_w			:	1	;		//�J�b�g���Ă���B�t���O�B
		unsigned	int		ch				:	4	;		//	�J�b�g���Ă���`�����l���ł��B15ch�܂őΉ��B
		unsigned	int		line			:	10	;		//�J�b�g���C����10�r�b�g�Ƃ�܂�(1024�{�܂�)
		unsigned	int						:	12	;		//�p�f�B���O�ł�
	} flg;

	//���̃��[�V�������ɁA�J�b�g��؂肱��ł��邩�ǂ���
	//�R���X�g���N�^������p�ӂ��Ă����܂��B
	_multi_motion();
	_multi_motion( double _t);
	bool	operator==(const _multi_motion &a)	const;
	bool	valid(void)	const;									//�L��/������\��
	_multi_motion&	invalid(void);								///�����ɂ���B
	bool	cut(void)		const	{	return	(bool)(flg.cut_z||flg.cut_w);								};
	bool	cut(int axis)	const	{	return	(( axis == 1 ) ? (bool)(flg.cut_w) : (bool)(flg.cut_z)) ;	};

	void	set( double _t, unsigned int _index,int _n_axis , bool _cut_z , bool _cut_w,int ch,int line);
	void	set(int ch, int line);					//

};	//size	:	12 byte

//���������[�V�����ƁA�P�����[�V�������Z�b�g�ɂ������́i�O������Q�Ƃ��₷���悤��)
//�O���֓o�^����Ă���V���O�����[�V�������A
class	_motion_set	:	public	_multi_motion
{
public:
	_single_motion	s[MOTION_SET_N_SINGLE];									//�ő��15�����̒P������B
	_motion_set(){;}										//���ɂȂɂ��E�E
	_motion_set(double t) : _multi_motion(t){;}				//
	_motion_set( _multi_motion &m);							//�h�����̃I�u�W�F�N�g����̃R���X�g���N�^
	bool	cut(void);										//�J�b�g�����Ă��邩
	bool	is_same_line(const _motion_set &last);			//��̃��[�V�������������C���ɑ����Ă�����̂�
	bool	is_same_ch(const _motion_set &last);			//
	_motion_set	predict(double t_ms)	const;				//t ms��̃��[�V������\�����܂��B
//	bool	in_range(const _motion_set &minus ,const _motion_set &plus) const;	//��̃��[�V�����Z�b�g�̊ԂɎ������g�����邩�ǂ���
//	void	invalid(void);								///�����ɂ���B
	_motion_set &invalid(void);
};
//cutline��VM��n����肩��.

//�J�b�g���C���Ƃ��ĔF�����邽�߂́B
//�}���`���[�V�������g����

//�J�b�g

//�ꃉ�C���̕]�������₷���悤�ȃf�[�^�\���B
//�e������
//

//toku interval�́Aint�Ƃ��Adouble�Ŏ��������ꍇ������A����̓e���v���[�g�ɓK���Ă���Ǝv����B
//�����Ԃ�\���I�u�W�F�N�g���^����ʉ����Ă����܂��B
template<class T>
struct	_interval
{
//	int	ch;	int line;															//�J�b�g��Ԃ�ch�ƃC���f�b�N�X�ł��B
	T start;	T	end;														//�J�b�g��Ԃ̃C���f�b�N�X
	_interval(){start=end=0;}													//�f�t�H���g��0�Ƃ��܂��B
	_interval(const T &s,const T &e)		{	start=s;end=e;				}		//�R�s�[���܂��B
	virtual	bool valid(void)	const	{	return(start>0 && end>0);	}		//0,0�Ŗ����Ƃ��܂��B�E�E
	T	n(void)	{	return	end - start;	}
};
struct	cut_interval	:	_interval<int>
{
	int ch;
	int line;
	cut_interval()				{	ch=line=-1;			}
	cut_interval(int _c,int _l)	{	ch=_c;	line=_l;	}
	virtual	bool	valid(void)	const	{	return(		(ch>=0)&&(line>=0)	);					}
};

//�����p�L���b�V���p�f�[�^�\���ł��B�Ƃ肠����index-double�����p�B�ł��B
//�ł����A���ʂ̃C���^�[�t�F�C�X�����B�C���f�b�N�X�ƒl�̑g�ݍ��킹�̃R���e�i�E�E�Evector�ł悢�悤��
#define	MAX_CACHE	16
class	cache
{
private:
	struct	val{
		int idx; double tick;
		val()	{clr();}
		val(int _i,double _t)		{	idx=_i; tick=_t;	}
		void clr(void)				{	idx=-1;				}
		void set(int _i,double _t)	{	idx=_i;tick=_t;		}
	}_v[MAX_CACHE];	//
	int p;	//���݂̃C���f�b�N�X�ł�
public:
	cahce(){p=0;}	~cache();
	void	allclr(void)		{int i;for( i=0 ; i<MAX_CACHE ; i++)	{_v[i].clr();	}}
	int		search(int _idx)	{	int i;for(i=0;i<MAX_CACHE;i++){ if(_v[i].idx==_idx) { return	i;	}	}	return -1;	}
	void set( int _i, double _t)	{	//���x�̂��߂ɂ����ł͎����I�ɏd���`�F�b�N�����Ȃ�.�Ăяo�����Ƃł��łɃL���b�V������Ă��邩���m���߂�B
		int idx;if( (idx=search(-1)) < 0 )	{	allclr();	idx=0;	}	//�Ȃ������ꍇ�ɂ̓L���b�V����S���N���A���ăC���f�b�N�X0�����蒼���܂�
		_v[idx].set(_i,_t);
	}
	void set(const val &__v)	{	set(__v.idx,__v.tick);	}
	//�w�����ꂽtick�Ɉ�ԋ߂��L���b�V���G���g����T���܂��B
	int least(double _t)	{
		val res(0,0);	//��ԍŋ߂̓o�^����T���܂�
		int i;for( i=p ; i>=0 ; i++	)	{
			if( f_eq_less(_v[i].tick , _t) == true )
				if( f_eq_less( res.tick, _v[i].tick )==true)	//v�ɂ͌��ʂ�����܂��B
						res = _v[i];
		}
		if( ! f_eq( res.tick , _t) )	set( _v[i] );	//�������̂��̂��������Ă��Ȃ��ꍇ�ɂ̓L���b�V���փZ�b�g���܂��B
		return res.idx;	//
	}
};

class	cutline
{
	//�V���O�����[�V��������������B8MB.
	//�I�u�W�F�N�g�����Ƃ��ɁA���I�Ɏ��悤�ɂ��܂��B�ÓI�ł���������
private:
	unsigned int		_n_allocated[2];				//�m�ۂł����f�[�^�T�C�Y

	//�����p�L���b�V���Ƃ��ă����o�������܂�
//�z��́A�C���f�b�N�X��tick��
	cache				_cache;					//

protected:
	_multi_motion		*_multi;						//���I�m�ۂ����������i�}���`���[�V����
	_single_motion		*_single;						//���I�m�ۂ���������
	int					_n_multi,_n_single;				//

	CRITICAL_SECTION		_critsec;					//�N���e�B�J���Z�N�V����(�v�[���A�N�Z�X�p)

	//�}���`���[�V�����ł́A�o�^����Ă��Ȃ��V���O�����[�V����������i��~���Ă���E���̓���Ȃǁj
	_motion_set&	_complete(const _motion_set &m,_motion_set &dst);		//�^����ꂽ���[�V�����Z�b�g�ŁA�L�^�̂Ȃ����̃V���O�����[�V�������A�⊮���܂��B
	_motion_set		_complete(const _motion_set &m);						//�^����ꂽ���[�V�����Z�b�g�ŁA�L�^�̂Ȃ����̃V���O�����[�V�������A�⊮���܂��B
	int				_index(const _multi_motion &m);							//
	int				_index(double tick);

	//�ƃ��[�V�����i�ʒu�j����A�J�b�g���Ă���CH����肷�邽�߂�	//������abs_pos�́A0����̐�΍��W��Ԃ����Ahome_pos[T]����̑��Έʒu��Ԃ���
	int				ch_theta_count(int ch , bool abs=false );

	//�֘A����VM�f�[�^���Q�Ƃ��邽�߂�VM���L�^����
	vm_database	&_vm;		//VM�f�[�^�ł��B

public:
	cutline();	~cutline();
	cutline( vm_database &__vm );		//�f�o�C�X�f�[�^
	cutline( const cutline &a);

	//���I�Ɋm�ۂ����e���[�V�����v�[���̃T�C�Y
	unsigned int	allocated(int type);												//�m�ۂ�����
	void			add(const _motion_set &add);										//���[�V�����̓o�^
	bool			add( double t , _MOTION org[] , bool cut_z  , bool cut_w );			//���[�V�����̓o�^

	int				cut_ch(_MOTION t);						//�J�b�g�`�����l������肵�܂��B

	//���[�V�����Z�b�g�̎擾
	_motion_set		get_motion_set(int idx , bool complete_singl_motion=true);					//���[�V�������擾���܂��B	�̂낢
	_motion_set	&	get_motion_set(_motion_set &set,int idx,bool complete_single_motion= true);	//���[�V�����Z�b�g���擾���܂��B�i�Q�Ɓj
	_motion_set	&	get_motion_set(_motion_set &set ,double tick );								//�w�����ꂽ�`�b�N�̃��[�V������Ԃ��܂��B
																								//

	_motion_set		get_last_motion_set(void);													//���߂̃��[�V����	���̂낢�p�~�\��
	_motion_set&	get_last_motion_set(_motion_set &m , bool complete_single_motion=true );	//
	_motion_set		get_last_cut_motion_set(unsigned int bit_assigned_z=0x3);			//���߂̃J�b�g���[�V����

	//�V���O�����[�V�����̎擾
	int				search_single_motion(int axis,int multi_idx);							//�V���O�����[�V�����̌����ł��B
	int				search_last_single_motion(int axis,double tick , double *found_tick=0);	//

	_single_motion&	get_single_motion(_single_motion &s , int axis, int idx );
	_single_motion&	get_last_single_motion(_single_motion &s ,int axis, double tick ,double *found_tick=0 );	//
	_single_motion&	calc_single_motion(_single_motion &s , int axis , double tick);

	//�����̓R�s�[������̂Ŏ��Ԃ�������Ǝv���܂��B
	_single_motion	get_single_motion(int axis, int idx );
	_single_motion	get_last_single_motion(int axis, double tick ,double *found_tick=0 );	//
	_single_motion	calc_single_motion(int axis , double tick);

	bool	operator==(cutline &lb);		//�J�b�g���C�����������ǂ����̔���
	bool	operator=(cutline &lb){	_Assert(0,"cutline::operator = is not allowed(because using heap memory)");	};			//�J�b�g���C���̃R�s�[�A�͂��܂̂Ƃ��닖���Ȃ��悤��

	//����CH��LINE��Ԃ��܂��B���Ԃ�������悤�Ȃ�A�C���f�b�N�X(intewrval�̔z��j�����炩���ߗp�ӂ��Ă����܂��B
	cut_interval	get_a_line(int ch , int line , int search_start_idx/*=0*/)	const;
	cut_interval	get_next_line(const cut_interval &prev_line)	const;

	//���郉�C���̂���n�Ԗڂ̃J�b�g��Ԃ�Ԃ��܂��B
	cut_interval	get_cut_part(const cut_interval &range ,int z)	const;
	cut_interval	get_cut_part(const cut_interval &whole_line , int z , int n)	const;

	//�C���f�b�N�X�͈͂��玞�Ԃ͈̔͂�Ԃ��܂��B
	_interval<double>	t(const _interval<int>& i);
	_interval<unsigned int>	t_uint(const _interval<int>&i);

	//�擾�������[�V�������A�A���C�����g�ʒu�ECTPOS�ʒu�֕ϊ�����
	_motion_set&	alipos_origin(_motion_set &m);		//�ϊ����ĕύX���Ă��܂��o�[�W�����R�s�[�̏ꍇ�ɂ̂낢�̂ł�

	//�O������̃A�N�Z�X�p
	int		n_multi(void)	{	return _n_multi;	}
	int		n_single(void)	{	return _n_single;	}
	const	_multi_motion	*multi_adr(void)	{	
//_printf("_multi_adr():0x%x\r" , _multi);
		return	(const _multi_motion*)&_multi[0];	
	}
	const	_single_motion	*single_adr(void)	{	return	(const _single_motion*)&_single[0];	}
	//�O���t�@�C������A�}���`���[�V�����ƁA�V���O�����[�V������
	bool	load(HANDLE fh,int n_multi,int n_single);



};

#endif		//__CUTLINE_H_INCLUDE__
//--------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------
//	��
//		�����^���͋L�^���Ȃ��ꍇ�ɂ́Amotion�`motion�܂ł̊Ԃ͕⊮����K�v������܂��B
//						(��葬�x�̏ꍇ�́A���ԁ~���x�ŋ�����⊮���Ȃ��ƂȂ�܂���B�j
//	MULTI_MOTION�́At0 ���� t1  �̊Ԃ̃`�b�N�́A�������x�������Ƃ���B�i�����͕ω����Ă������A������L�^���Ȃ�)
//				[t0] 		[t1]	 [t2]	 ( t �L�^�Ȃ�)				[t3]   	 [t4]		 [t5]
//	X			v0,pos		  				<--- �葬�^�� --->								v1, pos(t1)
//	 		 (�����I��)						v, pos = v0*(t-t0)							�iv1=v0�Apos(t1)=v0(t1-t0)�̂͂��jv	(�����J�n)
//
//	Y			0, pos	<----------------------- �Ƃ܂��Ă���: 0,pos)------------------->
//	V			0, pos	<----------------------- �Ƃ܂��Ă���: 0,pos)------------------->
//	Z			0, pos  <----------------------- �Ƃ܂��Ă���: 0,pos)------------------->
//	W			0, pos	<----------------------- �Ƃ܂��Ă���: 0,pos)------------------->
//	cut_flag_z	0             1													0
//						(�������Ń��[�N�ɂ������������j					(�����[�N����͂��ꂽ)
//	cut_flag_w	0					1									0
//								(Z2�������[�N�ɍ����|������)	�i�����[�N����͂��ꂽ)
//							�t���O�̕ω��_�ɂ����ẮA
//							���̎��ɓ���ω����Ȃ��ꍇ�́A���[�V�����v�[���ւ̋L�^�������ɁA�C���f�b�N�X
//							�����Ƃ��āA���̏ꏊ�̊e�����[�V�����͌v�Z�Ƃ��邩�A
//							
//							
//							�������́A�܂������L�^���Ȃ����B����͂ǂ���ł��悢�A�ԈႢ�̉\��������Ȃ�A����͋L�^���Ă��悢�A
//
//		���[�V�����̋L�^�F
//			�L�^�����e�ʂ�}���邽�߂ɁA
//			�e���ɂ��āA�����E�������A
//			�؂肱�݃t���O�ɕω�������ꍇ�B
//			���x�ƃt���O�ɕω�������ꍇ�ɋL�^����A

//			�L�^���Ă��Ȃ������ł͒��O�̏�ԁi���x���t���O�j���p�����Ă���Ƃ݂Ȃ��B
//		�����̊Ԃ̑��u����́A���x���Ƃ��āA�ʒu�̕ω�����������B
//			�i�m�F�̂��߂ɁA��L�ł́@t1 - t5 �̊ԁAX������葬�x�œ��삵�Ă��邪�At1����̎��Ԍo�߂� pos �� pos(t0) + v��t 
//			�ƂȂ��Ă��邩�ǂ������m�F����B
//			
//		
//		�J�b�g���C���̋L�^�F
//			�J�b�g���C���́Acut_flag_z = 1 - - 0 �̊ԁAZ1���J�b�g���Ă���B
//			�r���ŁAX�����Ƃ܂�����A�i�t���[����щz���Ȃǂ̓���̂��߁j
//			�`���b�p�[�g���o�[�X�Ȃ�
//			���̂����ɁA�ꎞ�I��cut_flag_z/w �t���O����������A���邱�Ƃ�����B
//
//		�������C�����ǂ����̔���
//			cut_flag_z	��	1	
//			cut_flag_z��0�ƂȂ��Ă��A�܂����C���͌p���Ƃ��āA�ēx�Acut_flag_z��1�ɂȂ����ۂɁA
//			Y���J�E���^������	X���i�s����������
//			

//			�V���O���E���[�V�����Ƃ���
//
//			cut_flag_w=1�̂Ƃ��́@Z2���̃J�b�g���C��
//			cut_flag_z�Ƃ͕ʂ̃^�C�~���O�Ńt���O�����B

//			�S���[�V�����̋L�^�Ƃ͕ʂɁA
//			�J�b�g���C���p�̋L�^�i�S���[�V�����L�^�ւ̎Q�ƂŃ��C��[idx] �̊J�nt�A�I��t�̑g�ݍ��킹�j]
//			���ȁE�EMOTION�ɁA�L�^���Ă��悢�B
//			
//			
//			�؂�I���`���ǂ�̃��[�V�����́Acut_flag_z���Q�Ă��܂��B
//
//
//		�V���O�����[�V�����̐�	
//			���[�V�����v�[�����A���SMByte
//			���[�V�����E�v�[���̃T�C�Y�ł��B

//

//X����
//---�ȉ��v�R�[�h
//-----------------------------------------------------------------------------------------------------
//		�J�b�g���C��
//-----------------------------------------------------------------------------------------------------
//#include	"math.h"		//fabs()
//cutline::cutline()		{	reset();	}	//	�f�[�^�����N���A���܂��B
//cutline::~cutline()		{	;			}	//	
//bool	cutline::_equal(double t0, double t1 , double t2 , double gosa=0.01)
//{	//����������l�͈̔͂Ȃ��ł����
//	if(	(fabs(t0 - t1) < gosa) && (fabs(t0 - t2) < gosa))	{	return true;	}
//	return false;
//}
//bool	cutline::set(const _MOTION &x , const _MOTION &y , const _MOTION &z , bool index_inclement)
//{
//	//����t�͑S�������łȂ��ƂȂ�܂���A�`�F�b�N���܂�
//	//double�Ȃ̂�0.1�ň�v���Ă���̂œ����Ƃ݂Ȃ��܂�
//	_Assert( equal(x.t,y.t,z.t , 0,01)==true , "cutline::set() tick not equal");
//	if ( n < CUTLINE_N_POINT)	{
//		m_x[n] = x;		m_y[n] = y;		m_z[n] = z;		n++;	return true;
//	}
//	_Assert(0 , "cutline::set() point overflow");
//	return false;
//}
////
////	���ݓo�^����Ă���ŐV�̃��[�V�������擾���܂��B
////	set�ɂ�_n++���ꂿ����Ă��邯��
////	
//_3AXIS_MOTION	cutline::last_motion(void)
//{
//	if ( !_n ) {	//�܂��������܂�Ă��Ȃ�
//		_3AXIS_MOTION
//	}
//	//�|�C���^ _n �́A���̏������݃|�C���g���w���Ă��܂��B
//	if( _n > 0 ) {		return	_m[_n-1];		}
//	else 
//}
//
//void	cutline::reset(void)							{	n=0;	}
//bool	cutline::operator==( const cutline& a , const cutline &b)
//{
//	//�J�b�g���C������v���Ă�����̂������Ă����܂��B
//	//�܂�Z����v
//	if	( a.Z	!=	b,Z		)	{	return	false;	}
//	if	( a._n	!=	b._n	)	{	return	false;	}
//
//	//�J�b�g���C���ɂ����ẮA�`�b�N
//	//�P���Ɉꃉ�C���̋O�ՂȂ̂őS�|�C���g���r����
//	//���������̑��̎��ɂ��Ă��΂�΂�ł悢�Ǝv���܂��B
//	{
//		int i;	for( i = 0 ; i < a._n ; i++ ) {
//			if(		a.m_x[i]  =! b.m_x[i]	)	return	false;
//			if(		a.m_y[i]  =! b.m_y[i]	)	return	false;
//			if(		a.m_z[i]  =! b.m_z[i]	)	return	false;
//		}
//	}
//	return true;
//}
