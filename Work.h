#ifndef		__WORK_H_INCLUDE__
#define		__WORK_H_INCLUDE__

//------------------------------------------------------------------------------------------------
//	include
//------------------------------------------------------------------------------------------------
#include	"axis_obj.h"		//MOTION�\����
#include	"cutline.h"			//�J�b�g���C���N���X�͕ʂ̃I�u�W�F�N�g�ɂ��܂����B
#include	"vm_database.h"	


//------------------------------------------------------------------------------------------------
//	defines
//------------------------------------------------------------------------------------------------
enum	{	_Z1	,	_Z2						};		//�J�b�g��
//enum	{	_X , _Y , _V , _Z , _W , _T		};		//����`�ł��A���[�V�����z������ꍇ�́A���̏����ŕ��ׂ�B
enum	{	_SQUARE		,	_ROUND			};		//���[�N�`��

//���[�N�I�u�W�F�N�g�̕ۑ��̎��Ɏg���w�b�_�ł��B
typedef struct	tag_WORK_SAVE_HEADER {
	unsigned	int 	header_size;
	unsigned 	int		filename_size;				//�t�@�C����(�o�b�t�@)�T�C�Y�ł��B
	unsigned	int		multi_motion_size;			//�}���`���[�V�����\���̂̃T�C�Y�B
	unsigned	int		single_motion_size;			//�V���O�����[�V�����\���̂̃T�C�Y
	unsigned	int		n_multi_motion;				//�ۑ�����Ă���}���`���[�V�������B
	unsigned	int		n_single_motion;			//�ۑ�����Ă���V���O�����[�V�������B
} WORK_SAVE_HEADER;

//
class work
{
protected:
	vm_database		_vm;
//	bool			_data_update(unsigned int t){	_vm.update(t);	return true;}
	char			_filename[FILENAME_MAX];			//�t�@�C�����ł��B�����VM�f�[�^�x�[�X���������O�ɂč����̂ł��B
public:
	cutline			_line;								//���ꂪ�J�b�g���C���ł��B
public:
	work();	~work();										//
	void	workcenter_origin(unsigned int t , _MOTION m[] );

	//���[�N�ɐ؂肱��ł��邩�ǂ���
	bool	kirikomi( _MOTION m[] , int axis );
	bool	_kirikomi(int ch ,  _MOTION z , _MOTION x ,_MOTION y);

	bool	record( unsigned long tick,_MOTION m[] );

	//�O������VM���Q�Ƃ��邽�߂�
	const vm_database	& get_vm(void)			{	_vm.update(0);	return (const vm_database&)_vm;}

	//���[�N�I�u�W�F�N�g�����܂��B
	bool	run(const char*filename);		//�w�����ꂽ�t�@�C�����Ŏ���������s���ċL�^���܂��B

	//���[�N�I�u�W�F�N�g�̃t�@�C���ۑ��E�ǂݏo��
	HANDLE	open(const char *filename , bool write);
	bool	save(void);
	bool	load(const char*filename);
	void	dump(void);
	void	dump_all(int from=0 , int to=0);
	bool	operator==( work& a);

	//
	const char*	fname(void)		{	return _filename;	}

	//
};

//------------------------------------------------------------------------------------------------
#endif		//__WORK_H_INCLUDE__
