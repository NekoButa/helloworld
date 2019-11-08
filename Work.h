#ifndef		__WORK_H_INCLUDE__
#define		__WORK_H_INCLUDE__

//------------------------------------------------------------------------------------------------
//	include
//------------------------------------------------------------------------------------------------
#include	"axis_obj.h"		//MOTION構造体
#include	"cutline.h"			//カットラインクラスは別のオブジェクトにしました。
#include	"vm_database.h"	


//------------------------------------------------------------------------------------------------
//	defines
//------------------------------------------------------------------------------------------------
enum	{	_Z1	,	_Z2						};		//カット軸
//enum	{	_X , _Y , _V , _Z , _W , _T		};		//軸定義です、モーション配列を作る場合は、この順序で並べる。
enum	{	_SQUARE		,	_ROUND			};		//ワーク形状

//ワークオブジェクトの保存の時に使うヘッダです。
typedef struct	tag_WORK_SAVE_HEADER {
	unsigned	int 	header_size;
	unsigned 	int		filename_size;				//ファイル名(バッファ)サイズです。
	unsigned	int		multi_motion_size;			//マルチモーション構造体のサイズ。
	unsigned	int		single_motion_size;			//シングルモーション構造体のサイズ
	unsigned	int		n_multi_motion;				//保存されているマルチモーション個数。
	unsigned	int		n_single_motion;			//保存されているシングルモーション個数。
} WORK_SAVE_HEADER;

//
class work
{
protected:
	vm_database		_vm;
//	bool			_data_update(unsigned int t){	_vm.update(t);	return true;}
	char			_filename[FILENAME_MAX];			//ファイル名です。これはVMデータベースも同じ名前にて作られるのです。
public:
	cutline			_line;								//これがカットラインです。
public:
	work();	~work();										//
	void	workcenter_origin(unsigned int t , _MOTION m[] );

	//ワークに切りこんでいるかどうか
	bool	kirikomi( _MOTION m[] , int axis );
	bool	_kirikomi(int ch ,  _MOTION z , _MOTION x ,_MOTION y);

	bool	record( unsigned long tick,_MOTION m[] );

	//外部からVMを参照するための
	const vm_database	& get_vm(void)			{	_vm.update(0);	return (const vm_database&)_vm;}

	//ワークオブジェクトを作ります。
	bool	run(const char*filename);		//指示されたファイル名で軸動作を実行して記録します。

	//ワークオブジェクトのファイル保存・読み出し
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
