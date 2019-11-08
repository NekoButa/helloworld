#ifndef		__CUTLINE_H_INCLUDE__
#define		__CUTLINE_H_INCLUDE__

#include	"axis_obj.h"		//_MOTION
//#include	"vm_interface.h"	//VM
#include	"vm_database.h"		//VMデータベース
//------------------------------------------------------------------------------------------------------------------
//	defines
//	軸動作トレース用、クラス
//------------------------------------------------------------------------------------------------------------------
enum	_AXIS_IDX	{	_INVALID_AXIS=-1,	_X=0 , _Y , _V , _Z , _W , _T	,_AXIS_IDX_MAX	};		//軸を表す番号
enum	{	_SINGL , _MULTI					};		//シングル・マルチモーションのどちらかを表す

#define	MOTION_SET_N_SINGLE	8			//motion_set構造体がもつシングルモーション数。

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
//struct	_MOTION	{	int phase;	double	t;	double	pos;	double	v;	double a;	};
//マルチモーションが管理する単軸モーションは時間の要素がなく、軸名の要素があります。
//複数軸分のモーション記録です。

//単軸用モーション記録です。axis_objの_MOTIONへ、付加情報を追加したものです。
struct	_single_motion	:	public	_MOTION
{
	_AXIS_IDX   	axis;	//軸名を表すアスキー文字です。
//	_MOTION			m;		//方向は、速度の符号で表すようにします。
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

};	//	※size	:	28(sizeof(_MOTION)	+	4	=	32 byte
//マルチモーション（複数の軸を表すための)オブジェクト。
//	容量を減らすために、各シングルモーション（単軸動作)は、このオブジェジェクト外部へ記録されます。
//	
class	_multi_motion
{
protected:
public:	//メンバーは基本的にパブリックとします
	double		t;						//これは共通のチックとします								4
	unsigned	int			index;		//これがシングルモーションプールへのポインタとなります。	4	（配列添え字)
	struct	{
		unsigned	int		n_axis			:	4	;		//記録した軸数です。4bitだと15軸まで。
		unsigned 	int		cut_z			:	1	;		//ワークに切りこんでいる。
		unsigned 	int		cut_w			:	1	;		//カットしている。フラグ。
		unsigned	int		ch				:	4	;		//	カットしているチャンネルです。15chまで対応。
		unsigned	int		line			:	10	;		//カットラインは10ビットとります(1024本まで)
		unsigned	int						:	12	;		//パディングです
	} flg;

	//このモーション中に、カットを切りこんでいるかどうか
	//コンストラクタだけを用意しておきます。
	_multi_motion();
	_multi_motion( double _t);
	bool	operator==(const _multi_motion &a)	const;
	bool	valid(void)	const;									//有効/無効を表す
	_multi_motion&	invalid(void);								///無効にする。
	bool	cut(void)		const	{	return	(bool)(flg.cut_z||flg.cut_w);								};
	bool	cut(int axis)	const	{	return	(( axis == 1 ) ? (bool)(flg.cut_w) : (bool)(flg.cut_z)) ;	};

	void	set( double _t, unsigned int _index,int _n_axis , bool _cut_z , bool _cut_w,int ch,int line);
	void	set(int ch, int line);					//

};	//size	:	12 byte

//複数軸モーションと、単軸モーションをセットにしたもの（外部から参照しやすいように)
//外部へ登録されているシングルモーションを、
class	_motion_set	:	public	_multi_motion
{
public:
	_single_motion	s[MOTION_SET_N_SINGLE];									//最大の15軸分の単軸動作。
	_motion_set(){;}										//特になにも・・
	_motion_set(double t) : _multi_motion(t){;}				//
	_motion_set( _multi_motion &m);							//派生元のオブジェクトからのコンストラクタ
	bool	cut(void);										//カットをしているか
	bool	is_same_line(const _motion_set &last);			//二つのモーションが同じラインに属しているものか
	bool	is_same_ch(const _motion_set &last);			//
	_motion_set	predict(double t_ms)	const;				//t ms後のモーションを予測します。
//	bool	in_range(const _motion_set &minus ,const _motion_set &plus) const;	//二つのモーションセットの間に自分自身があるかどうか
//	void	invalid(void);								///無効にする。
	_motion_set &invalid(void);
};
//cutlineにVMを渡すやりかた.

//カットラインとして認識するための。
//マルチモーションを使って

//カット

//一ラインの評価をしやすいようなデータ構造。
//各部分の
//

//toku intervalは、intとか、doubleで持ちたい場合があり、これはテンプレートに適していると思われる。
//ある区間を表すオブジェクトを型を一般化しておきます。
template<class T>
struct	_interval
{
//	int	ch;	int line;															//カット区間のchとインデックスです。
	T start;	T	end;														//カット区間のインデックス
	_interval(){start=end=0;}													//デフォルトで0とします。
	_interval(const T &s,const T &e)		{	start=s;end=e;				}		//コピーします。
	virtual	bool valid(void)	const	{	return(start>0 && end>0);	}		//0,0で無効とします。・・
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

//検索用キャッシュ用データ構造です。とりあえずindex-double検索用。です。
//ですが、共通のインターフェイスをもつ。インデックスと値の組み合わせのコンテナ・・・vectorでよいような
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
	int p;	//現在のインデックスです
public:
	cahce(){p=0;}	~cache();
	void	allclr(void)		{int i;for( i=0 ; i<MAX_CACHE ; i++)	{_v[i].clr();	}}
	int		search(int _idx)	{	int i;for(i=0;i<MAX_CACHE;i++){ if(_v[i].idx==_idx) { return	i;	}	}	return -1;	}
	void set( int _i, double _t)	{	//速度のためにここでは自発的に重複チェックをしない.呼び出しもとですでにキャッシュされているかを確かめる。
		int idx;if( (idx=search(-1)) < 0 )	{	allclr();	idx=0;	}	//なかった場合にはキャッシュを全部クリアしてインデックス0からやり直します
		_v[idx].set(_i,_t);
	}
	void set(const val &__v)	{	set(__v.idx,__v.tick);	}
	//指示されたtickに一番近いキャッシュエントリを探します。
	int least(double _t)	{
		val res(0,0);	//一番最近の登録から探します
		int i;for( i=p ; i>=0 ; i++	)	{
			if( f_eq_less(_v[i].tick , _t) == true )
				if( f_eq_less( res.tick, _v[i].tick )==true)	//vには結果が入ります。
						res = _v[i];
		}
		if( ! f_eq( res.tick , _t) )	set( _v[i] );	//もしそのものが見つかっていない場合にはキャッシュへセットします。
		return res.idx;	//
	}
};

class	cutline
{
	//シングルモーションをたくさん。8MB.
	//オブジェクトを作るときに、動的に取るようにします。静的でもいいけど
private:
	unsigned int		_n_allocated[2];				//確保できたデータサイズ

	//検索用キャッシュとしてメンバを持ちます
//配列は、インデックスとtickの
	cache				_cache;					//

protected:
	_multi_motion		*_multi;						//動的確保したメモリ（マルチモーション
	_single_motion		*_single;						//動的確保したメモリ
	int					_n_multi,_n_single;				//

	CRITICAL_SECTION		_critsec;					//クリティカルセクション(プールアクセス用)

	//マルチモーションでは、登録されていないシングルモーションがある（停止している・一定の動作など）
	_motion_set&	_complete(const _motion_set &m,_motion_set &dst);		//与えられたモーションセットで、記録のない軸のシングルモーションを、補完します。
	_motion_set		_complete(const _motion_set &m);						//与えられたモーションセットで、記録のない軸のシングルモーションを、補完します。
	int				_index(const _multi_motion &m);							//
	int				_index(double tick);

	//θモーション（位置）から、カットしているCHを特定するための	//第二引数abs_posは、0からの絶対座標を返すか、home_pos[T]からの相対位置を返すか
	int				ch_theta_count(int ch , bool abs=false );

	//関連するVMデータを参照するためのVMを記録する
	vm_database	&_vm;		//VMデータです。

public:
	cutline();	~cutline();
	cutline( vm_database &__vm );		//デバイスデータ
	cutline( const cutline &a);

	//動的に確保した各モーションプールのサイズ
	unsigned int	allocated(int type);												//確保した個数
	void			add(const _motion_set &add);										//モーションの登録
	bool			add( double t , _MOTION org[] , bool cut_z  , bool cut_w );			//モーションの登録

	int				cut_ch(_MOTION t);						//カットチャンネルを特定します。

	//モーションセットの取得
	_motion_set		get_motion_set(int idx , bool complete_singl_motion=true);					//モーションを取得します。	のろい
	_motion_set	&	get_motion_set(_motion_set &set,int idx,bool complete_single_motion= true);	//モーションセットを取得します。（参照）
	_motion_set	&	get_motion_set(_motion_set &set ,double tick );								//指示されたチックのモーションを返します。
																								//

	_motion_set		get_last_motion_set(void);													//直近のモーション	※のろい廃止予定
	_motion_set&	get_last_motion_set(_motion_set &m , bool complete_single_motion=true );	//
	_motion_set		get_last_cut_motion_set(unsigned int bit_assigned_z=0x3);			//直近のカットモーション

	//シングルモーションの取得
	int				search_single_motion(int axis,int multi_idx);							//シングルモーションの検索です。
	int				search_last_single_motion(int axis,double tick , double *found_tick=0);	//

	_single_motion&	get_single_motion(_single_motion &s , int axis, int idx );
	_single_motion&	get_last_single_motion(_single_motion &s ,int axis, double tick ,double *found_tick=0 );	//
	_single_motion&	calc_single_motion(_single_motion &s , int axis , double tick);

	//これらはコピーが入るので時間がかかると思います。
	_single_motion	get_single_motion(int axis, int idx );
	_single_motion	get_last_single_motion(int axis, double tick ,double *found_tick=0 );	//
	_single_motion	calc_single_motion(int axis , double tick);

	bool	operator==(cutline &lb);		//カットラインが同じかどうかの判定
	bool	operator=(cutline &lb){	_Assert(0,"cutline::operator = is not allowed(because using heap memory)");	};			//カットラインのコピー、はいまのところ許さないように

	//あるCHのLINEを返します。時間がかかるようなら、インデックス(intewrvalの配列）をあらかじめ用意しておきます。
	cut_interval	get_a_line(int ch , int line , int search_start_idx/*=0*/)	const;
	cut_interval	get_next_line(const cut_interval &prev_line)	const;

	//あるラインのうちn番目のカット区間を返します。
	cut_interval	get_cut_part(const cut_interval &range ,int z)	const;
	cut_interval	get_cut_part(const cut_interval &whole_line , int z , int n)	const;

	//インデックス範囲から時間の範囲を返します。
	_interval<double>	t(const _interval<int>& i);
	_interval<unsigned int>	t_uint(const _interval<int>&i);

	//取得したモーションを、アライメント位置・CTPOS位置へ変換して
	_motion_set&	alipos_origin(_motion_set &m);		//変換して変更してしまうバージョンコピーの場合にのろいのでは

	//外部からのアクセス用
	int		n_multi(void)	{	return _n_multi;	}
	int		n_single(void)	{	return _n_single;	}
	const	_multi_motion	*multi_adr(void)	{	
//_printf("_multi_adr():0x%x\r" , _multi);
		return	(const _multi_motion*)&_multi[0];	
	}
	const	_single_motion	*single_adr(void)	{	return	(const _single_motion*)&_single[0];	}
	//外部ファイルから、マルチモーションと、シングルモーションの
	bool	load(HANDLE fh,int n_multi,int n_single);



};

#endif		//__CUTLINE_H_INCLUDE__
//--------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------
//	※
//		直線運動は記録しない場合には、motion～motionまでの間は補完する必要があります。
//						(一定速度の場合は、時間×速度で距離を補完しないとなりません。）
//	MULTI_MOTIONの、t0 から t1  の間のチックは、同じ速度が続くとする。（距離は変化していくが、それを記録しない)
//				[t0] 		[t1]	 [t2]	 ( t 記録なし)				[t3]   	 [t4]		 [t5]
//	X			v0,pos		  				<--- 定速運動 --->								v1, pos(t1)
//	 		 (加速終了)						v, pos = v0*(t-t0)							（v1=v0、pos(t1)=v0(t1-t0)のはず）v	(減速開始)
//
//	Y			0, pos	<----------------------- とまっている: 0,pos)------------------->
//	V			0, pos	<----------------------- とまっている: 0,pos)------------------->
//	Z			0, pos  <----------------------- とまっている: 0,pos)------------------->
//	W			0, pos	<----------------------- とまっている: 0,pos)------------------->
//	cut_flag_z	0             1													0
//						(↑ここでワークにさしかかった）					(↑ワークからはずれた)
//	cut_flag_w	0					1									0
//								(Z2側がワークに差し掛かった)	（↑ワークからはずれた)
//							フラグの変化点においては、
//							他の軸に動作変化がない場合は、モーションプールへの記録をせずに、インデックス
//							無効として、その場所の各軸モーションは計算とするか、
//							
//							
//							もしくは、まったく記録しないか。それはどちらでもよい、間違いの可能性があるなら、これは記録してもよい、
//
//		モーションの記録：
//			記録される容量を抑えるために、
//			各軸について、加速・減速中、
//			切りこみフラグに変化がある場合。
//			速度とフラグに変化がある場合に記録され、

//			記録していない部分では直前の状態（速度＆フラグ）が継続しているとみなす。
//		→その間の装置動作は、速度一定として、位置の変化だけがある。
//			（確認のために、上記では　t1 - t5 の間、X軸が一定速度で動作しているが、t1からの時間経過で pos が pos(t0) + vΔt 
//			となっているかどうかを確認する。
//			
//		
//		カットラインの記録：
//			カットラインは、cut_flag_z = 1 - - 0 の間、Z1がカットしている。
//			途中で、X軸がとまったり、（フレーム飛び越しなどの動作のため）
//			チョッパートラバースなど
//			そのさいに、一時的にcut_flag_z/w フラグが落ちたり、することがある。
//
//		同じラインかどうかの判別
//			cut_flag_z	が	1	
//			cut_flag_zが0となっても、まだラインは継続として、再度、cut_flag_zが1になった際に、
//			Y軸カウンタが同じ	X軸進行方向が同じ
//			

//			シングル・モーションとして
//
//			cut_flag_w=1のときは　Z2側のカットライン
//			cut_flag_zとは別のタイミングでフラグがたつ。

//			全モーションの記録とは別に、
//			カットライン用の記録（全モーション記録への参照でライン[idx] の開始t、終了tの組み合わせ）]
//			かな・・MOTIONに、記録してもよい。
//			
//			
//			切り終わり～もどりのモーションは、cut_flag_zが寝ています。
//
//
//		シングルモーションの数	
//			モーションプールを、数百MByte
//			モーション・プールのサイズです。

//

//X軸は
//---以下没コード
//-----------------------------------------------------------------------------------------------------
//		カットライン
//-----------------------------------------------------------------------------------------------------
//#include	"math.h"		//fabs()
//cutline::cutline()		{	reset();	}	//	データ個数をクリアします。
//cutline::~cutline()		{	;			}	//	
//bool	cutline::_equal(double t0, double t1 , double t2 , double gosa=0.01)
//{	//差分がある値の範囲ないであれば
//	if(	(fabs(t0 - t1) < gosa) && (fabs(t0 - t2) < gosa))	{	return true;	}
//	return false;
//}
//bool	cutline::set(const _MOTION &x , const _MOTION &y , const _MOTION &z , bool index_inclement)
//{
//	//時間tは全部同じでないとなりません、チェックします
//	//doubleなので0.1で一致しているので同じとみなします
//	_Assert( equal(x.t,y.t,z.t , 0,01)==true , "cutline::set() tick not equal");
//	if ( n < CUTLINE_N_POINT)	{
//		m_x[n] = x;		m_y[n] = y;		m_z[n] = z;		n++;	return true;
//	}
//	_Assert(0 , "cutline::set() point overflow");
//	return false;
//}
////
////	現在登録されている最新のモーションを取得します。
////	setにて_n++されちゃっているけど
////	
//_3AXIS_MOTION	cutline::last_motion(void)
//{
//	if ( !_n ) {	//まだ書きこまれていない
//		_3AXIS_MOTION
//	}
//	//ポインタ _n は、次の書き込みポイントを指しています。
//	if( _n > 0 ) {		return	_m[_n-1];		}
//	else 
//}
//
//void	cutline::reset(void)							{	n=0;	}
//bool	cutline::operator==( const cutline& a , const cutline &b)
//{
//	//カットラインが一致しているものかを見ていきます。
//	//まずZが一致
//	if	( a.Z	!=	b,Z		)	{	return	false;	}
//	if	( a._n	!=	b._n	)	{	return	false;	}
//
//	//カットラインにおいては、チック
//	//単純に一ラインの軌跡なので全ポイントを比較する
//	//同じ時刻の他の軸についてもばらばらでよいと思います。
//	{
//		int i;	for( i = 0 ; i < a._n ; i++ ) {
//			if(		a.m_x[i]  =! b.m_x[i]	)	return	false;
//			if(		a.m_y[i]  =! b.m_y[i]	)	return	false;
//			if(		a.m_z[i]  =! b.m_z[i]	)	return	false;
//		}
//	}
//	return true;
//}
