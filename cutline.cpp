//------------------------------------------------------------------------------------------------------------------
//	motion_pool.cpp
//	モーション・プール	大量の軸モーションを記録するための場所です。
//	実は二つのモーションプールを比較するために、二個分のプールが必要です。
//	動的に習得する必要がある？？
//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
//	include
//------------------------------------------------------------------------------------------------------------------
#include	"cutline.h"
#include	"commonmodule.h"
#include	"local_axis_interface.h"		//ローカル軸に複数軸に

#include	<stdlib.h>

//------------------------------------------------------------------------------------------------------------------
//	defines
//------------------------------------------------------------------------------------------------------------------
//複数軸のトレースを行うための

#define		_DEFAULT_SINGL_MOTION_SIZE	(8*1024*1024)							//8MByteとります	(419430 個)	全	419sec(単軸あたり)4軸だと100sec程度
#define		_DEFAULT_MULTI_MOTION_SIZE	(2*1024*1024)							//2MByteとります	(174762	個)	全 174sec分

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
//デフォルトコンストラクタを許可しないようにします。
static	vm_database	_vm_invalid;;	//デフォルトコンストラクタのデータ参照です。
cutline::cutline()	:	_vm(_vm_invalid)	{	_Assert(0,"cutline::cutline() default constructor not allowed");	}
cutline::cutline(vm_database &__vm) : _vm(__vm)
{
	_n_single = _n_multi = 0;		//各ｲﾝﾃﾞｯｸｽ
	//メモリを確保していきます。
	HANDLE hheap = ::GetProcessHeap();	//プロセスヒープを取得して
	{
		_multi		=	(_multi_motion*) ::HeapAlloc(hheap ,HEAP_ZERO_MEMORY,_DEFAULT_MULTI_MOTION_SIZE);	//プール領域を確保します。
		_single		=	(_single_motion*)::HeapAlloc(hheap ,HEAP_ZERO_MEMORY,_DEFAULT_SINGL_MOTION_SIZE);	//プール領域を確保します。
		_Assert(_single	!=	NULL , "singl motion alloc failed(%s)" , LastErrorMsg() );
		_Assert(_multi	!=	NULL , "multi motion alloc failed(%s)" , LastErrorMsg() );
	}
	_n_allocated[_SINGL]	=	::HeapSize(hheap , 0, _single );	//
	_n_allocated[_MULTI]	=	::HeapSize(hheap , 0 , _multi );	//
	_n_single = _n_multi = 0;										//現在の書き込みインデックスです。

	::InitializeCriticalSection(&_critsec);		//同期用クリティカルセクション
}
cutline::cutline( const cutline &a)	:_vm(_vm_invalid)
{
	_Assert(0,"cutline::cutline(cutline &a) : copy constructor not allowed(allocated memory transfer unsupported yet)");
	//toku もし対応するなら、改めてメモリを確保して、内容をコピーする処理を書かないとなりません。
	//		operator=　も、同様の理由にて許可しないようにします。
}
cutline::~cutline()
{	//確保したメモリを解放します。
	HANDLE hheap = ::GetProcessHeap();			//プロセスヒープを取得して
_printf("cutline: destruct: memoru free\n");
	::HeapFree(hheap ,0 , _single);	::HeapFree(hheap ,0 , _multi);
}
//--------------------------------------------------------------------------------------------------------------
//	いろいろな検索キーから、マルチモーションの登録インデックスを探す。
//--------------------------------------------------------------------------------------------------------------
//指定されたマルチモーションのｲﾝﾃﾞｯｸｽを返します。
int	cutline::_index(const _multi_motion &m)
{
	int i;for(i=0 ; i < _n_multi ; i++ ) { if( m == _multi[i])	return i;	}
	_Assert(0 , "cutline::index() : index not found" );
	return -1;	//
}

//tickにおける、_multi[]に記録されている動作の
int	cutline::_index(double tick)
{

#if	1	
	DWORD	__t = ::GetTickCount();
#endif
	//かならずtickははじめに記録された時刻よりも先にないとなりません。
	_Assert( f_eq_less( _multi[0].t, tick) , "cutline::_index() : tick is before start(_multi[0] %lf), (tick:%lf)\n" , _multi[0].t , tick );
	_Assert( _n_multi > 0 , "cutline::_index() : no registration (_multi)");	//登録がないとなりません。

	//もし登録されているものすべてよりもあとのtickが指示された場合は、最後のインデックスを返します。
	if( tick > _multi[_n_multi-1].t) return _n_multi-1;

	//それ以外は登録の範囲の中にありますので探します。
//検索範囲を狭めたい以前に指示のあったチックと、インデックスの対応があればそこを開始地点に
	//検索キャッシュから、チックにもっとも近い
	int search_start = _cache.least(tick);	//

	//
	int i;for(i=search_start ; i < _n_multi ; i++ ) {
		if( f_eq(_multi[i].t , tick) ==true)	{

#if	1	//
	{	__dg +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif
				return i;		
			}	//完全に一致する場所があれば、そこです。
		else if( tick < _multi[i].t)			{
#if	1	//
	{	__dh +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif
				return 	i-1;	}	//完全に一致しなくとも、tickを超えた場合にはひとつ前を返します。
	}

	_Assert(0 , "cutline::index(doublt tick) : index not found" );
	return -1;	//
}
//--------------------------------------------------------------------------------------------------------------
//	モーションセット（複数軸）	モーションの取得
//--------------------------------------------------------------------------------------------------------------
//指示されたインデックスでのモーションセットを作ります。
_motion_set	&cutline::get_motion_set(_motion_set &set,int idx,bool complete_single_motion  /* = true */)
{
#if	1	
	DWORD	__t = ::GetTickCount();
#endif
	_Assert( idx <  _n_multi , "cutline;::get() illegal indx(%d/%d)" , idx , _n_multi );
	::EnterCriticalSection(&_critsec);
		set=_multi[idx];			//指示されたidxのマルチモーションを元に作っていきます。
		//シングルモーションをコピーしていきます。
#if	1	//
	{	__dd +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif
		int i;for(i = 0 ;i < (int)set.flg.n_axis ; i++ ) {			//登録された軸数分シングルモーションを登録します
			_single_motion s	=	_single[ set.index + i];		//
_Assert(s.axis >= 0  ,"get_motion_set(): no data (s.axis illegal:%d)" , s.axis);
			set.s[s.axis]	=	s;									//シングルモーション配列の添え字は軸インデックスです
		}

#if	1	//
	{	__de +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif

//tokutoku
		if( complete_single_motion	)	_complete(set,set);		//ないシングルモーションを
#if	1	//
	{	__df +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif

	::LeaveCriticalSection(&_critsec);
	return	set;
}
//toku これは時間かかっちゃう
_motion_set	cutline::get_motion_set(int idx,bool complete_single_motion  /* = true */)
{
	_motion_set set;	return	get_motion_set(set,idx,complete_single_motion);
}
//toku これは
//チックを指示して、そのときのモーションセットを返します。
_motion_set	&	cutline::get_motion_set(_motion_set &set ,double tick )
{
	//toku tickはdoubleですが、


	//	int idx	= _index(tick);	//toku これは指示tickの直前のindexが帰ってきます。
	//	例：	idx		  5				  6				  7				  8
	//			tick	10.0			20.0			30.0			40.0
	//											23.5
	//											↑ tick
	//									  ↑ _index() return 6
//_motion_set	&cutline::get_motion_set(_motion_set &set,int idx,bool complete_single_motion  /* = true */)

//toku カット中のチックが進んでも、カット開始時点でのマルチモーションが帰ります。
//カットフラグが
#if	1	
	DWORD	__t = ::GetTickCount();
#endif
//tokutoku
if( f_eq(tick , 81925.0)==true)	{
	_printf("tick! %lf -> %lf\n" , _multi[0].t , tick);	
}
//toku ここが時間くう。①
	get_motion_set(set , _index(tick),true);	//中身をすべて補完します
	//それで、tickは直近からずれている可能性があります。

#if	1	//
	{	__db +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif

	//tickに進めるのです。ここでtick分進めたときに、記録が軸がの加速減速があるとAssertになります。
	//ここはデータの空白部分が、等速運動またはstopの部分だけがとばされていて、

	//時間を置き変えて処理します。
	if( set.t < tick ) {	//これは、指示されたチックが、登録インデックスと異なっている場合に、軸の動作を進める処理です。
		//等速運動のものを見つけて、先に進めます。
		set.t = tick;
		int i;for(i=_X ; i < _AXIS_IDX_MAX ; i++ ) {
			_Assert( ((set.s[i].phase==PHASE_CONST) || (set.s[i].phase==PHASE_STOP)),"get_motion_set(double tick): illegal phase(%d,%d)" , i, set.s[i].phase);
			if( set.s[i].phase == PHASE_CONST )	{		//等速運動の場合の
//toku さらにここの時間食う。②。
				calc_single_motion( set.s[i],i,tick);
			}
		}
	}

#if	1	//
	{	__dc +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif
	return set;
}


//直近のマルチモーションを返します。（戻り値は、シングルモーションを含む、モーション・セットを返します)
_motion_set&	cutline::get_last_motion_set(_motion_set &m , bool complete_single_motion/*=true*/ )
{
	if( _n_multi > 0 )	{		get_motion_set(m,_n_multi-1 ,complete_single_motion);	}	//データがある場合には返します。
	else m.invalid();	//データがない場合はデフォルトの無効なオブジェクトを返します。
	return m;
}
_motion_set	cutline::get_last_motion_set(void)	{	_motion_set set;	return	get_last_motion_set( set , true);	}
//直近のカットしていたモーションを探して返します。
//引数：	
//		unsigned int	bit_assinged_z		:	ﾋﾞｯﾄ割り付けで	bit0:Z1 bit1:Z2
//		_motion_set		&m					:	
//bool	cutline::get_last_cut_motion_set(_motion_set &m , unsigned int bit_assinged_z /*=0x3*/ )
_motion_set	cutline::get_last_cut_motion_set(unsigned int bit_assigned_z/*=0x3**/)
{
	int i;for( i = _n_multi-1 ; i >= 0 ; i-- )	{	//
		if	( 	bit_assigned_z & 0x1	)	{	if ( _multi[i].flg.cut_z )	goto found;		}
		if	(	bit_assigned_z & 0x2	)	{	if ( _multi[i].flg.cut_w )	goto found;		}
	}
	return	_motion_set();	//デフォルトの無効なオブジェクトを返します。
found:
	return	get_motion_set(i);
//	m = get_motion_set(i);	return true;
}


//interval構造体を返します。見つからない場合には、無効なintervalを返します。
//カットフラグは、何度も落ちたりすることがあるかもしれません。
//
//	interval
//	---------+		 +-------+		 +------+	 +-------------
//			 |_______|		 |_______|		|____|			   
//	↑		 												  ↑
// start   								  					  end
//toku これは指示されたchとlineをもつライン区間を返します。実質これは使えなくなりました。
//interval	cutline::get_a_line(int ch , int line , int search_start_idx/*=0*/)	const
cut_interval	cutline::get_a_line(int ch , int line , int search_start_idx/*=0*/)	const
{
	_Assert( search_start_idx < _n_multi , "get_a_line :: multi_" );
	cut_interval	r(ch ,line);		//intervalのchとlineは指示通りのものです。

	int i;
//_search_start:-----------------------------------------------------------
	for( i=search_start_idx ; i< _n_multi ; i++ )	{	//ライン開始位置を見つけます。
		if(	((int)_multi[i].flg.ch == ch ) && ((int)_multi[i].flg.line == line)) {	r.start = i;goto _search_end;}
	}
	goto	_not_found;		//見つかりません。
_search_end:
	for( 					; i< _n_multi ; i++ )	{	//ラインエンド位置を探します。
		if(	((int)_multi[i].flg.ch != ch) ||	((int)_multi[i].flg.line != line))	{
			r.end=i-1;	break;	}
	}	//toku 最後まで来てしまった場合もラインエンドとします。
	{	if(r.end<0) r.end = _n_multi-1;	}	//最後まで来てしまった場合には、最後を登録します。
	return r;
_not_found:
	return cut_interval();
}
//指示されたカット区間の、次のカット区間を返します。
cut_interval	cutline::get_next_line(const cut_interval &prev_line)	const
{
	int idx;	if( (idx = (prev_line.valid() ? prev_line.end + 1 : 0)) < _n_multi ) 	{
		return	get_a_line(	(int)_multi[idx].flg.ch	, (int)_multi[idx].flg.line ,  idx);
	}
	//検索開始インデックスを決めます。前回のラインの最後の次から開始です。
_error:
	return	cut_interval();
}

//指示された範囲で、指示された軸のカット範囲を見つけて返します。
cut_interval	cutline::get_cut_part(const cut_interval &range , int z )	const
{
	//与えられたカットラインのうち、n番目のカット線分を返します。
	cut_interval	r(range.ch,range.line);
	int i;bool cutting=false;
	for( 	i=range.start	;	i <= range.end ; i++ )	{
		if( !cutting )	{	if(	_multi[i].cut(z) == true )	{
//toku ここで何かしらの軸が切りかかった。
			r.start = i;	cutting=true;		}}
		else			{	if( _multi[i].cut(z) == false)	{
//toku ここで何かしらの軸が
			r.end	= i;	goto _found;		}}
	}
	//ここまできてしまったら見つかっていません。
	return	cut_interval();
_found:
	return	r;
}
//n番目の線分を取得します。
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
	return	cut_interval();	//ない場合には無効な区間を返します。
}
//カットラインが同じかどうかの判定をするモジュール
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
		//無効なラインがある場合。
		if ( !line_a.valid() || !line_a.valid() )	{	if( ! line_a.valid() && !line_b.valid() )	goto _fin;	//	両方ない場合は終了です。		
		//これは、まだ片方にはカットラインが存在する
			set_lasterr("(ch:%d,line:%d cut line invalid (%d,%d)", line_a.ch, line_a.line , line_a.valid() ,line_b.valid());	
			_printf("ch:%d,line:%d cut line invalid (%d,%d)", line_a.ch, line_a.line , line_a.valid() ,line_b.valid());	
			goto _error;
		}
		//カットしているchとかラインが違う場合をチェックします
		if( (line_a.ch != line_b.ch) || (line_a.line != line_b.line) )	{
			set_lasterr("ch:%d,line:%d/ ch /line is differrent  (b.ch: %d , b.line: %d)", line_a.ch, line_a.line , line_b.ch ,line_b.line);	
			_printf("ch:%d,line:%d/ ch /line is differrent  (b.ch: %d , b.line: %d)", line_a.ch, line_a.line , line_b.ch ,line_b.line);
			goto _error;
		}
//toku チャンネルの比較開始としてダンプします。
_printf("[ch:%d][line:%d]  A: [%d t(%d)]-[%d t(%d)]    B:[%d t(%d)] [%d t(%d)]\n ", line_a.ch, line_a.line 
,	line_a.start	,	(int)_multi[line_a.start].t		,	line_a.end		,	 (int)_multi[line_a.end].t	,
	line_b.start	,	(int)lb._multi[line_b.start].t	,	line_b.end		,	(int)lb._multi[line_b.end].t );

#if	1	
	__da=__db=__dc=__dd=__de=__df=__dh=__dg=0;	__t = ::GetTickCount();
#endif

//toku 1ライン中には
		//さらにラインの中の線分を取り出して比較していきます。
		{
			bool	fin[2];	int n;for( n=0 , fin[0]=fin[1]=false ; ( (fin[0] !=true) || (fin[1]!=true) ) ; n++ )	{	//1ライン中のカット区間を一つづつ評価していきます。
//toku デュアルカットの場合には、Z1Z2独立してみていきます。
				int	z;for( z=0 ; z<2 ; z++)	{
					if (fin[z])		continue;	//終わっている軸の評価はしない。
					cut_interval a	= get_cut_part(line_a ,z , n );	cut_interval b	= lb.get_cut_part(line_b , z , n );

					//無効な線分がある場合。
					if( !a.valid() || !b.valid() ) {
						if( !a.valid() && !b.valid() )	{		fin[z] = true;continue;		}	//両軸ともカットラインが見つかりませんので終わりです。
//						set_lasterr("(ch:%d,line:%d cut part invalid (%d,%d)", line_a.ch,line_a.line,a.valid() , b.valid());
						//toku チックの1msの間に、ワークぎりぎりの場合に切りこみが発生しない場合がある。（ちょっとしか切らない場合）
						//問題は、本当はちょっとしか切らないが、本当は切れている部分を見つけないとなりません。
						//				→	add()のときに、1msごとに走査していくさいにカットラインンが見つけられない。
						
						//とりあえず、片方にしかないラインについて、そのように表示するようにします。
//						_printf("(ch:%d,line:%d cut part invalid (%d,%d)", line_a.ch,line_a.line,a.valid() , b.valid());
//						goto _error;
					}

					//チックの数を比較します。同じモーションが記録されているなら、1チックより小さい誤差になっている
					//チック数は問題ない、endの位置が
//toku ここを無視してみる。
//					if ( abs( a.n() - b.n() ) > 1 )	{	set_lasterr("ch:%d,line:%d cut part n() not same(%d,%d) ",line_a.ch, line_a.line , a.n() , b.n() );	goto _error;	}

//toku ここで

					//カット範囲内のチックの数は、
					//カット区間中は、まったく同じモーションであることを確認します
//カット区間において、チックごとに比較していきます。
//_motion_set	&	cutline::get_motion_set(_motion_set &set ,double tick )

//片方がi有効でない場合も流します。
//					_interval<double>ta	{	( a.valid() ? (_multi[a.start].t	,	_multi[a.end].t)	,	(0,0)	};
//					interval<unsigned int>tb	{	( b.valid()	? (lb._multi[b.start].t	,	lb._multi[b.end].t)	,	(0,0)	};
//					unsigned int	tick_a		=	a.valid()?_multi[a.start].t	:0	,	tick_b		=	b.valid()?lb._multi[b.start].t	:0;
//					unsigned int	tick_a_end	=	a.valid()?_multi[a.end].t	:0	,	tick_b_end	=	b.valid()?lb._multi[b.end].t	:0;
					_interval<unsigned int>tick_a=t(a)	,	tick_b=t(b);
					unsigned int tick_a=(unsigned int)t(a).start , tick_b = (unsigned int)t(b).start;
					for( ;	(tick_a < (unsigned int)t(a).end) || (tick_b<(unsigned int)t(b).end ) ; )	{

//if( (line_a.ch==2) && (line_a.line ==5) ){	_printf("[%d][%d] [%d][%d]\n" , tick_a , tick_a_end , tick_b ,tick_b_end);}

						//アライメント位置原点で比較していきます。toku この時点で、軸モーションの補完をしてはいけないです。
						//というのは、VM（マシンデータ、デバイスデータ）は、this->_line と、 b._line	が参照するワークオブジェクトの
						//VMファイルが違う。そのため、現状のVMの状況が合っているかどうかが、わからないので、
						//get_motion_setとか、alipos_oroginなど_macや_vevを使うような関数では
						//
						//いちいち
		//toku だめだ。ここでVMを使えない。alipos_originを呼べません。
		//いや呼べる。	vmdatabase::update()	にて、ファイルを更新するから、大丈夫だと思います。

		//toku 各もーしょん。の比較です。
		//		片方は、Z2が切りこむのでずれるはず。

						_motion_set ma,mb;
						//toku もし、ここでどちらか片方が終わっている場合に、片方にしかないことを記録するようにします。
						if ( tick_a == tick_a_end)	{	//Aが終わっている。
if( (line_a.ch==2) && (line_a.line ==5) ){	_printf("only B[%d][%lf]\n" , tick_b , (double)tick_b );	}
							mb= lb.alipos_origin(lb.get_motion_set(mb , (double)tick_b));
							_printf("    -- [B only:end] tick[--][%d] idx[--][%d] [%c]                            /  pos[%8.3lf]spd[%8.3lf][%d] \n",tick_b ,lb._index(tick_b) , 'X' , mb.s[_X].pos , mb.s[_X].v , mb.s[_X].phase );
							++tick_b;continue;
						}
						if ( tick_b == tick_b_end)	{	//Bが終わっている。
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
						//tokuここが同じモーションかどうかの判定です。誤差1ms（チックのずれ分）で判定します。
						//カットをしていない軸については判定をしないようにします。
						//なめらかと、通常の場合で、逃げ量がことなるため、is_sameがfalseを返してしまう。
						//一つずつ軸の動作が同じかどうかを判定していきます。
						//ここでモーションが、異なる場合に、X軸の進行方向に応じて、どちらかを待つようにします？

						//X軸の位置を比較して、違う場合には、遅いほうを、独自の
						{
							if( ma.s[_X].is_same_pos(mb.s[_X],1.0)!=true) {	//X軸の位置がことなる場合、先に時間を進めます。（まつ)
								if(		((ma.s[_X].v >= 0 )&& (ma.s[_X].pos > mb.s[_X].pos))	
									||	((ma.s[_X].v < 0 ) && (ma.s[_X].pos < mb.s[_X].pos))	){		//aが進んでいる場合
									//bだけ進めます。もうすすめない場合は、。。。	BがAに追いついてないので記録します
									_printf("    -- [B only slow] tick[%d][%d] idx[%d][%d] [%c] pos[%8.3lf]spd[%8.3lf][%d] /  pos[%8.3lf]spd[%8.3lf][%d] \n",tick_a,tick_b, _index(tick_a) ,_index(tick_b) ,'X' ,ma.s[_X].pos , ma.s[_X].v , ma.s[_X].phase , mb.s[_X].pos , mb.s[_X].v , mb.s[_X].phase );
									if( ++tick_b > tick_b_end ){	tick_b=tick_b_end;	_printf("    -- [error] B do not reach to A \n");		}//(X軸が到達しない)
									continue;
								} else {
									_printf("    -- [A only slow] tick[%d][%d] idx[%d][%d] [%c] pos[%8.3lf]spd[%8.3lf][%d] /  pos[%8.3lf]spd[%8.3lf][%d] \n",tick_a,tick_b, _index(tick_a) ,_index(tick_b) ,'X' ,ma.s[_X].pos , ma.s[_X].v , ma.s[_X].phase , mb.s[_X].pos , mb.s[_X].v , mb.s[_X].phase );
									if( ++tick_a > tick_a_end ){	tick_a=tick_a_end;	_printf("    -- [error] A do not reach to B \n");		}//(X軸が到達しない)
									continue;
								}
							}
						}

		//ラインには、入り側と抜け側がある。
		//通常は
		//また、モーションの記録のインデックスでは位置が
						//_X位置は同じですが、速度が違うかもしれないから
						{
							const char _axis[_AXIS_IDX_MAX] = {'X','Y','V','Z','W','T'	};	_Assert( sizeof(_axis)==_AXIS_IDX_MAX, "axis define illegal\n");
							int i;for(i=_X ; i < _AXIS_IDX_MAX ; i++ ) {
								if( (z==0)	&&	((i==_V) || (i==_W))	)	{	continue;	}	//Z1の評価の場合にはZ2側は評価しない
								if( (z==1)	&&	((i==_Y) || (i==_Z))	)	{	continue;	}
								if( ma.s[i].is_same_pos(mb.s[i],1.0)	!=	true	)	{	//位置の評価です。
										_printf("    -- [pos        ] tick[%d][%d]idx[%d][%d] [%c] pos[%8.3lf]spd[%8.3lf][%d] /  pos[%8.3lf]spd[%8.3lf][%d] \n",tick_a,tick_b,_index(tick_a) ,_index(tick_b) ,_axis[i],ma.s[i].pos , ma.s[i].v , ma.s[i].phase ,mb.s[i].pos , mb.s[i].v , mb.s[i].phase );
								}
								if( ma.s[i].is_same_spd(mb.s[i],1.0)	!=	true	)	{	//速度の評価です。加速などのフェーズ違いも見てもいいかも
										_printf("    -- [spd        ] tick[%d][%d]idx[%d][%d] [%c] pos[%8.3lf]spd[%8.3lf][%d] /  pos[%8.3lf]spd[%8.3lf][%d] \n",tick_a,tick_b,_index(tick_a) ,_index(tick_b) ,_axis[i],ma.s[i].pos , ma.s[i].v , ma.s[i].phase , mb.s[i].pos , mb.s[i].v , mb.s[i].phase );
								}
							}
						}
		//				if( ma.is_same(mb , 1 ) !=true)	{	set_lasterr("ch:%d,line:%d motion not same", line_a.ch, line_a.line);	goto _error;	}
						//toku	ここまできた場合には、両方のチックをインクリメントします。
						//bだけ進めます。もうすすめない場合は、。。。
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
_error:	//一致しない
	//一致しない場合、どのラインでダメかを詳細にしる必要がある
	return false;
}
//--------------------------------------------------------------------------------------------------------------
//	シングル（単軸）モーションの取得
//--------------------------------------------------------------------------------------------------------------
//指示されたインデックスで、指示された軸のシングルモーションを返します、ない場合はfalseです。
//toku これは、指示されたインデックスでのシングルモーションを返すものです。
//過去のモーションの参照を返してもいいのでは

//インデックスを返すバージョン。	見つからない場合には-1を返します。
int	cutline::search_single_motion(int axis,int idx)
{
	_Assert( idx <  _n_multi		,	"cutline;::get_single_motion() illegal indx(%d/%d)" , idx , _n_multi );
	_Assert ( axis < _AXIS_IDX_MAX	,	"cutline::get_single_motion() axis illegal(%d)" , axis);
	::EnterCriticalSection(&_critsec);
	{
		int i;for(i = 0 ;i < (int)_multi[idx].flg.n_axis ; i++ )	{	//登録された軸数分シングルモーションを登録します
			if( _single[ _multi[idx].index + i].axis == axis)	{
				::LeaveCriticalSection(&_critsec);
				return	_multi[idx].index + i;
			}
		}
	}
	::LeaveCriticalSection(&_critsec);
	return -1;	//見つからない場合には-1です。
}
//オブジェクト参照を受けてそれにコピーして返すバージョン。時間的には変わらなかった。
_single_motion&	cutline::get_single_motion(_single_motion &s , int axis, int idx )
{
	int s_idx;	if ( (s_idx = search_single_motion(axis,idx)) < 0 ) {	return	(s=_single_motion());	}	return	s=_single[s_idx];
}
_single_motion	cutline::get_single_motion(int axis, int idx )	{	_single_motion s;	get_single_motion(s,axis,idx);	return s;	}
//指示された軸で、指示されたチックからさかのぼって
//みつかったモーションの時刻はfound_tickへ記録します。
int	cutline::search_last_single_motion(int axis,double tick , double *found_tick/*=0*/)
{
	int idx = _index(tick);						//指示されたチックのインデックスです。
	int i;for( i = idx ; i >= 0 ; --i )	{		//さかのぼって、シングルモーションのある場所を探します。
		int s_idx;	if( (s_idx = search_single_motion(axis,i)) >= 0 ) {	//みつかった
			if( found_tick )	{
				*found_tick = _multi[i].t;					//見つかったチックを書き戻します。
				_Assert( found_tick >= 0 , "search_last_single_motion() found_tick is illegal");//時刻は昔でないとなりません
			}
			return s_idx;
		}
	}
	//もしみつからない場合は、アサーとです。このデータベースは、必ず一番初めにすべての軸のモーションを書くようにします。
	//そのため、シングルモーションは必ず見つかる前提とします。
	_Assert( 0 , "cutline::search_last_single_motion() not found axis=%d" ,axis);
	return	-1;	//無効なインデックスで
}

_single_motion	&cutline::	get_last_single_motion(_single_motion &s,int axis, double tick ,double *found_tick/*=0*/ )
{
	int idx = _index(tick);						//指示されたチックのインデックスです。
	int i;for( i = idx ; i >= 0 ; --i )	{		//さかのぼって、シングルモーションのある場所を探します。
		//インデックスを見つけるバージョンにしてみます。
		if ( get_single_motion(s,axis,i).valid() == true)	{
			if( found_tick )	{
				*found_tick = _multi[i].t;					//見つかったチックを書き戻します。
				_Assert( found_tick >= 0 , "get_last_single_motion() found_tick is illegal");//時刻は昔でないとなりません
			}
			return s;
		}
	}
	//もしみつからない場合は、アサーとです。このデータベースは、必ず一番初めにすべての軸のモーションを書くようにします。
	//そのため、シングルモーションは必ず見つかる前提とします。
	_Assert( 0 , "cutline::get_last_single_motion() not found axis=%d" ,axis);
	return s=_single_motion();	//無効なシングルモーションを返します。
}
_single_motion	cutline::get_last_single_motion(int axis, double tick ,double *found_tick/*=0*/ )
{
	_single_motion	s;	get_last_single_motion( s , axis ,tick ,found_tick);	return s;
}

//--------------------------------------------------------------------------------------------------------------
//	シングル（単軸）モーションの補完	（登録のない部分のシングルモーションを計算、作成する）
//--------------------------------------------------------------------------------------------------------------
//あるチックでの(登録のない)シングルモーション	（単軸の挙動)	を計算します。
_single_motion	&cutline::calc_single_motion(_single_motion &s ,int axis , double tick)
{
	double	found_tick;	//直近に記録されたモーションの時刻です。
#if	1
	s = _single[search_last_single_motion(axis,tick,&found_tick)];
#else
	get_last_single_motion( s , axis , tick , &found_tick);		//tickからさかのぼって最後に記録されたもの
#endif

	_Assert( s.valid()== true , "calc_single_motion() single motion not found");//ないとアサートします。
	//シングルモーション見つかりました。直近のシングルモーションが状況を調べます。
	if	( s.phase == PHASE_CONST )	{			//一定速度で動作している場合には、この時刻でのモーションを計算します。
		s.pos	+=	s.v*(tick - found_tick);		//いま現在の位置を計算します。
		//toku これは、動作方向によって符号がかわるはず
	}
	else if( s.phase==PHASE_STOP)	{;}	//とまっている場合。シングルモーションは変化していませんのでそのままを返します。
	else	{	_Assert(0,"calc_single_motion:: illegal phase(%d)" , s.phase);	}
//それ以外のモーションはいまのところ、許可しないようにします。
//各モーションは、加速・減速の終わりには、かならずSTOPやCONSTが一度記録されるようにします。
	return	s;	
}
_single_motion	cutline::calc_single_motion(int axis , double tick)
{
	_single_motion s;	calc_single_motion( s , axis , tick );	return s;
}
//モーションセットに未登録のシングルモーションを、補完します。
_motion_set	&cutline::_complete( const _motion_set &org , _motion_set &dst )
{
//結局ここでコピーが入りまくるが。・・
//toku ここのcopyをなくす
	//srcとdstが同じなら
	if(	&org	!=	&org) {
		dst = org;	//まずコピー
	}
//_single_motion	&cutline::calc_single_motion(_single_motion &s ,int axis , double tick)

	int i;for( i=0; i < _AXIS_IDX_MAX ; i++ ) {
//		if( org.s[i].axis == -1 )	{	dst.s[i] = calc_single_motion(i , org.t);		}	//登録なしの場合計算する。
//参照を渡すように
		if( org.s[i].axis == -1 )	{	calc_single_motion(dst.s[i] , i , org.t);		}	//登録なしの場合計算する。
//		else						{	dst.s[i] = org.s[i];							}	//コピー
	}
	dst.flg.n_axis = _AXIS_IDX_MAX;	//前軸分のデータがあります。
	return dst;
}
//コピーしない（自分自身を書き換える

//toku これは時間がかかるので
_motion_set	cutline::_complete( const _motion_set &org)
{
	_motion_set s;	_complete( org , s );	return s;
/*
	_motion_set	set = org;	//まずコピー
	int i;for( i=0; i < _AXIS_IDX_MAX ; i++ ) {
		if( set.s[i].axis == -1 )	{	set.s[i] = calc_single_motion(i , org.t);		}	//登録なしの場合計算する。
	}
	set.flg.n_axis = _AXIS_IDX_MAX;	//前軸分のデータがあります。
	return set;
*/
}
//--------------------------------------------------------------------------------------------------------------
//	シングルモーションの登録
//--------------------------------------------------------------------------------------------------------------
//補完されたとして、そのモーションが、chなにの、何ライン目、というのが
//あるCHは、Θ軸がある程度動いたら。
//ラインは、
//	Y軸	、

//マルチ、モーションにて、
//	X軸が動作を開始して、一度とまるまで。これが一ラインです。
//	フレーム飛び越しなどで、一度とまる。
//	Zを上げて、再度Xが動作し、Zを下げて、Xが動作する。
//	そのときに、カットフラグが再度経ちます。
//	そのさいに、同じラインかどうかを判定するのに、前にカットフラグが立っていたマルチモーションを探し、
//	その時の、YV軸位置、

//	Y軸、V軸の位置が変わらず、X軸の方向が一緒であれば、同ラインとみなす。
//	同ラインにて、
//	

//	※	もし。、。。切りこんでいるのに、Y軸に加速や、減速、など位置が変わるようなことがある場合
//		それをどうとらえるか、それはエラー
//
//		カットフラグが経ちっぱなしのときに
//		YVを動かすことがあっても、それは、同じラインです。（エラーを検出するか？）
//		
//		同じラインかどうかの判定は、
//


//モーションセットをデータベースへ登録します。
void	cutline::add( const _motion_set &set)
{
	_Assert( _n_multi	< allocated(_MULTI)	,	"cutline::add(motion_set) _n_multi overflow");	//
	_Assert( _n_single	< allocated(_SINGL)	,	"cutline::add(motion_set) _n_single overflow");	//

	::EnterCriticalSection(&_critsec);		//モーション・セットを登録します。
		_multi[_n_multi++] = (_multi_motion&)set;												//multi
//toku set.s[xx]は、とびとびで入っています。軸固定にて入っています。そちらのほうが扱いやすいと思うので
		int	i_s, i;for(i_s=0,i=0 ; i <(int)set.flg.n_axis ; i++ ) {
			//保存するシングルモーションを探します。
			for( ; i_s < MOTION_SET_N_SINGLE ; i_s++) {
				if( set.s[i_s].axis != _INVALID_AXIS ) { _single[_n_single++] = set.s[i_s];	i_s++;goto _next_axis; }
			}
			_Assert( 0 , "cutline::add(motion set): single motion not enough(flg.n_axis=%d)" , set.flg.n_axis);
_next_axis:;
		}	//single
	::LeaveCriticalSection(&_critsec);
}
//ある時刻のマルチモーションを記録します。
//この時点では６軸分データ
bool	cutline::add( double t , _MOTION org[] , bool cut_z  , bool cut_w )
{
#if	1	
DWORD	__t;
#endif

	::EnterCriticalSection(&_critsec);

	//今回のマルチモーションを作リます。
	_motion_set	now(t);		//toku これは

#if 1//
	{if(cut_z || cut_w ) {
		__aho = 1;
	}
	if(cut_z){
		__aho=2;
	}
	}
#endif

	//今回切っているかどうか
	{
		now.flg.cut_z	=	cut_z;		//指定されたカットフラグ
		now.flg.cut_w	=	cut_w;		//指定されたカットフラグ
		//記録する軸を決めます。	※ここで、記録するために、
		//記録軸の条件：
		//		①	マルチモーションがはじめて記録される場合。
		//		②	記録が2度目以降にて、加減速を行っている場合。(加速・減速の最後に、定常的な動作に入る、その一度目だけを登録する)
#if	1	
__t = ::GetTickCount();
//toku ここは、一定速度や止まっているモーションだけ選択する、モーションが動いている場合は最後のモーションが、
//時間のかかる処理とすれば最後のシングルモーションの検索
#endif
		int axis;for(axis = 0 ; axis < _AXIS_IDX_MAX ; axis++)	{
			if( _n_multi > 0 ) {	//最初の登録ではない場合、定常的な動作は重複して記録しないようにします。
				if( ( org[axis].phase == PHASE_STOP) || ( org[axis].phase == PHASE_CONST) )	{
#if	1				//以下にすれば早い
					_single_motion	&s = _single[search_last_single_motion(axis,t)];
#else
					_single_motion	s = get_last_single_motion(axis,t);	//該当軸で、最後に登録されたモーションを探します。
#endif

					if( (s.phase==PHASE_STOP) || (s.phase ==PHASE_CONST) )	{continue;}	//↑それが、STOPであれば、重複して登録しない。
				}
			}
			//加速終点、減速終点でのPHASE_STOP、PHASE_CONSTは入れたほうが
//toku ここで軸を割り付ける必要があるか。割りつけのほうがわかりやすいかも。・・・
			now.s[axis]	=	_single_motion( (_AXIS_IDX)axis , org[axis] );
			now.flg.n_axis++;
		}


		//もし今回切りこみがある場合は、カットラインを特定します。
		//カットラインがあろうが、なかろうか、何かしらのカットラインに属しますか？
		//きり終わり～戻り～次のラインの始まりまでは、前回のラインに属することになります。
//toku もうすこし簡潔にならないか
//toku is_same_ch/is_same_lineを書くこと。

		//切りこみ（フラグ）がある場合、CHとLINEを特定します。切りこみがない場合は、直近のモーションと同じにします。
//このモーションでのchは、0から数えた、順序となります。
//実際のCHは、ABモードの場合 1, 2 , ｻﾌﾞｲﾝﾃﾞｯｸｽの場合、CUT_CH
#if	1	//now.s[]で書きこむべき軸（加速軸）のみを
{	__ca +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif
		{
			_motion_set last;						//直近のカットしているモーションです。
			_motion_set _c_now	= _complete(now);	//計算のため、全軸が補完されている現在のモーションセットを用意します。
			if (_c_now.flg.cut_z || _c_now.flg.cut_w) {						//今回なにかしらの切り込みがある場合
				//カットしている場合には、chが特定できる状況にあるはずです（ので見つからない場合にアサートとします)
				int ch = cut_ch(_c_now.s[_T]);	_Assert(ch >= 0 , "cutline::add() cut_ch not found T_cnt=%lf", _c_now.s[_T].pos );
				if(	(last=get_last_cut_motion_set()).valid() == true ) {	//直近のカットラインが取得できたら
					if( _c_now.is_same_ch(last) == true ) {
						now.set(	ch	,	last.flg.line + (_c_now.is_same_line(last)?0:1)	);	//同じラインでない場合はインクリメントです
					} else {	//違うCHになったようです。
#if	1	//	
		__aho=0;
#endif
						now.set( ch, 0);		//このCHの
					}
				}	else{	//直近のラインがみつからない場合にはCHの一番初めとします。
					now.set( ch, 0);
				}
#if	1	//もし切りこんでいる場合にchとラインを特定する。ところ、ここは時間をくわない。
{	__cb +=  GetTickCount() - __t;	__t = GetTickCount();	}
#endif
			} else { //カットラインがない場合は、直近のモーションと同じにします。？
#if	1	
__t = ::GetTickCount();
//toku ここは、一定速度や止まっているモーションだけ選択する、モーションが動いている場合は最後のモーションが、
//時間のかかる処理とすれば最後のシングルモーションの検索
#endif
//				last=get_last_motion_set();			//じかんかかる		total -> t[891998]ms 	completeあり
													//					total -> t[919267]ms 	
													//		_compareを参照版に	→		[872201]ms 

//				get_last_motion_set(last);			//じかんかからん	total -> 		t[476785]ms 	completeあり
													//		_completeを参照版に	→		t[472168]ms		たいして変わらない。
													//		_completeをインデックスに	t[579013]ms 	よけいに時間かかり・・？

#if	0			//run() total -> t[58501]ms t[71106]ms 
				if( get_last_motion_set(last,false).valid() == true) {	now.set( last.flg.ch , last.flg.line );	}
				else	now.set(0,0);		//直近のモーションがみつからない場合には
#else
				//以下のコードで	t[61371]ms 		
				if(_n_multi > 0 ) {
					now.set( _multi[_n_multi-1].flg.ch	,	_multi[_n_multi-1].flg.line	);
				}else{		now.set(0,0);	}		//直近のモーションがみつからない場合には
#endif

#if	1	
{	__cc +=  GetTickCount() - __t;	__t = GetTickCount();	}	//toku ここが支配的のよう
#endif
			}
		}
	}

//toku ここまでが時間かかる-----------------------

	//toku この時点で、動作している。・してないに関係なく6軸全部のモーションがはいっています。
	//toku ここで直近の動作がsetに入っています。
	//書き込みをするかどうかを判断します。
	{
		//初めての書き込みの場合には、書きます。
		if( _n_multi == 0 ) 										goto _write;

		//前回からカットフラグが変わっているかどうか。
		if( _multi[_n_multi-1].flg.cut_z != now.flg.cut_z)	{
//カットラインのはいり・出口では、モーションが一定のために、軸の記録がされていない場合があります。
_printf("ch：%dline:%d (%d->%d)\n" , now.flg.ch, now.flg.line ,_multi[_n_multi-1].flg.cut_z ,  now.flg.cut_z);
			now	= _complete(now);	//全軸を補完します。
			goto _write;
		}
		if( _multi[_n_multi-1].flg.cut_w != now.flg.cut_w)	{
			now	= _complete(now);	//全軸を補完します。
			goto _write;
		}

//フラグの違いで書く場合、もしかしたら、シングルモーションとしてなにも登録がない場合があるかもしれません。
		//そのほか、記録するべき軸がある場合です。
		//すでにシングルノードを作っている場合には、
		if(	now.flg.n_axis )										goto _write;
	}
	::LeaveCriticalSection(&_critsec);
	return	false;
_write:

	//今回の書き込みインデックスは	_n_multi/ _n_singlです。
	//この時点で、motion_setのシングルモーションへの先頭インデックスが決まります。
	now.index =	(now.flg.n_axis>0)?_n_single:0xffffffff;	//シングルの登録がない場合はINVALIDとしておきます。
	add(now);
	::LeaveCriticalSection(&_critsec);


	return true;
}
//モーションセットを、アライメント位置からの相対とします
//Tのカウンタから、カットチャンネルを計算して返します。
//各チャンネルのΘのカウンタ値を返します。
int	cutline::ch_theta_count(int ch , bool abs )
{
	_Assert( (ch>0) && (ch < MAX_CH) ,"ch_theta_count ch illegal(%d) " , ch);
	if (_vm.dev.sub_index() )	
	{	return	(abs?_vm.mac.homepos['T']:0)	+ _vm.dev.post_ch[ch] +	_vm.mac.ali_post[ch];	}
	return	(abs?_vm.mac.homepos['T']:0) + ( (ch==2) ? 90000000 : 0 ) + _vm.mac.ali_post[ch];
}
//θのカウンタ値から、chを返します。
//どのチャンネルにも属さない場合は、-1を返す。
int cutline::cut_ch(_MOTION t)
{	//フェイズが、もし停止状態でない場合、CHはわかりません。（過渡状態)
	if(	t.phase!=PHASE_STOP	)	goto	_not_found;
	//カウンタ値が	1°以内ならば、同じchとします。
	int ch;for(ch=1 ; ch < MAX_CH ; ch++ ) 	{
		if( f_eq((double)ch_theta_count(ch),t.pos  , 1000000)==true) {	return ch;	}
	}
_not_found:
	return -1;
}
//ワークセンターオリジン→アライメント位置基準のmotion_setに変更します。
_motion_set&	cutline::alipos_origin(_motion_set &m)
{
	int ch,ali_posy;
//#if	1
//	DWORD	__t = ::GetTickCount();
//#endif
	_vm.update((unsigned int)m.t);	//マシンデータをこのチックのものにします。
//#if	1
//	{	__dc +=  GetTickCount() - __t;	__t = GetTickCount();	}
//#endif

	if( (ch = cut_ch(m.s[_T])) < 0 )	{	//CHが見つからない
		//ここで、わからない場合には、予想してCHを返してもいい
		goto _error;
	}
	//toku (暫定)ali_post[ch]が0の場合は、ワークセンターとします。(AlI_POSYは、絶対位置です。0の場合には、CUTプログラムにてALU_CY_HIに補完するしくみ）
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
//t_ms ms後のモーションを予測します。（正確ではありません)測定誤差の計算のために使います。
_single_motion	_single_motion::predict(double t)	const
{
	_single_motion s = *this;	//まず自身をコピー
	//	int		phase;	double	pos;	double	v;	double a;
	//	toku 単純に、速度V、加速度Aにて、直線的に加速した場合の、速度と変位です。
	if( t <	0 ) {		//マイナスの場合時間をさかのぼる
		t*=-1;			//符号を元に戻して計算します。
		s.v			= 	v	-	a*t;
		s.pos		=	pos	-	(s.v*t	+	(a*t*t/2));		//初速度は、過去の
	} else {				//プラスの場合。
		s.v			=	v	+	(a*t);
		s.pos		=	pos +	(v*t	+	(a*t*t/2));		//
	}
	//※toku 目標位置を超えて、phaseが本当はとまる場合もあるかもしれない。
	return s;		//
}

//
int		_single_motion::is_same_pos(const _single_motion &a,int gosa_ms) const
{
	_single_motion	s;	double max,min;

	//	============================	位置	=================================
	{	s= predict(gosa_ms);	max = __max(pos , s.pos);	min = __min( pos , s.pos);	}	//gosa_ms分進めます。
	//その中に、あいての位置が存在すれば、それは同じ位置とみなします。
	if( f_eq_less(a.pos,max,1.0) && f_eq_less(min,a.pos,1.0)	)	goto _in_range;
	//もしくは、自分が相手の誤差範囲内にある場合は同じ位置とみなします。
	{	s= a.predict(gosa_ms);	max = __max(a.pos , s.pos);	min = __min( a.pos , s.pos);	}	//gosa_ms分進めます。
	if( f_eq_less(pos,max,1.0) && f_eq_less(min,pos,1.0))			goto _in_range;
	return	false;
_in_range:
	return	true;
}
int		_single_motion::is_same_spd(const _single_motion &a , int gosa_ms) const
{
	_single_motion	s;	double max,min;
	//	============================	速度	=================================
	{	s= predict(gosa_ms);	max = __max( v , s.v);	min = __min( v , s.v);	}	//gosa_ms分進めます。
	//その中に、あいての位置が存在すれば、それは同じ位置とみなします。
	if( f_eq_less(a.v , max ,1.0) && f_eq_less(min,a.v,1.0))	goto _in_range;
	//もしくは、自分が相手の誤差範囲内にある場合は同じ位置とみなします。
	{	s= a.predict(gosa_ms);	max = __max(a.v , s.v);	min = __min( a.v , s.v);	}	//gosa_ms分進めます。
	if( f_eq_less(v,max,1.0) && f_eq_less(min,v,1.0))			goto _in_range;
	return false;
_in_range:
	return true;
}
//
bool	_single_motion::is_same(const _single_motion &a,int gosa_ms) const
{
	//誤差 gosa_ms以内で同じ座標にいるかどうかを評価します。
	//自分
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
//デフォルトコンストラクタとして無効なオブジェクトを作ります。
_multi_motion::_multi_motion()				{	set( 0 , -1 ,0,false,false,0,0);		}
_multi_motion::_multi_motion( double _t)	{	set(_t	, -1 ,0,false,false,0,0);		}
void	_multi_motion::set( double _t, unsigned int _index,int _n_axis , bool _cut_z , bool _cut_w,int ch,int line)
{
	t=_t;	index = _index;		flg.n_axis=_n_axis;	flg.cut_z = _cut_z;	flg.cut_w = _cut_w;	set(ch,line);
}
//chとlineのセットです。
void	_multi_motion::set(int ch, int line)	{	flg.ch=ch;flg.line=line;	}
bool	_multi_motion::operator==(const _multi_motion &a)	const
{
	if(	! f_eq(t,a.t) )	return	false;		//チックが一致
	if(	a.index			!=	index			)	return	false;		//シングルモーションプールへのポインタが一致
	if( a.flg.n_axis	!=	flg.n_axis		)	return	false;		//
	if(	a.flg.cut_z		!=	flg.cut_z		)	return	false;
	if( a.flg.cut_w		!=	flg.cut_w		)	return	false;
	if( a.flg.ch		!=	flg.ch			)	return	false;
	if(	a.flg.line		!=	flg.line		)	return	false;
	return true;
}
bool	_multi_motion::valid(void)const
{
	//シングルインデックスの登録がない・//カットフラグの登録がないと無効です。
	if(	(index==-1)	&& (flg.n_axis==0) )	{if(( flg.cut_z==0) && ( flg.cut_w==0))	{return false;}}
	return	true;	//それ以外は有効です。
}
_multi_motion&	_multi_motion::invalid(void)	{	set(0, -1 ,0,false,false,0,0);return *this;	}
//---------------------------------------------------------------------------------------------------
//		_motion_set	(struct)	imprement
//---------------------------------------------------------------------------------------------------
//指示されたmulti_optionを基にしたコンストラクタ
//toku この方法でよいのかどうかよくわからない。
_motion_set::_motion_set(_multi_motion &m)
{	set(m.t , m.index , m.flg.n_axis , m.flg.cut_z , m.flg.cut_w , m.flg.ch , m.flg.line );	}
//他のモーションセットと、同じCHかどうかの判定です。	θ軸のモーションをみます。
//※
bool	_motion_set::is_same_ch(const _motion_set &last)
{
	//θ軸のシングルモーションがある必要があります
	_Assert(   s[_T].valid()==true  , "is_same_ch() :   theta axis invalid");
	_Assert( last.s[_T].valid()==true  , "is_same_ch() : last theta axis invalid");
	//誤差3°くらいは同じCHとみなします
	if( f_eq( s[_T].pos , last.s[_T].pos , 3 ))	return	true;
	return false;
}
//（ライン番号の判定用	：	時系列にてラインが変わったかどうか）
//過去のモーションと、現在のモーション（*this)が、同じラインに属しているかどうかを返す。
bool	_motion_set::is_same_line(const _motion_set &last)
{
	bool	same_z=false,same_w=false;		//X軸、Z軸、Y軸、Z軸は必ずないとなりません。
	_Assert( s[_X].valid() && s[_Y].valid() && s[_V].valid() && s[_Z].valid() && s[_W].valid() ,"is_same_line error");
	_Assert( last.s[_X].valid() && last.s[_Y].valid() && last.s[_V].valid() && last.s[_Z].valid() && last.s[_W].valid() ,"is_same_line ");

	//X軸が走っているかどうかは、
	//単軸カットの場合に、カットしている軸しか評価しないようにしないとならない、（もう片方の軸はフリー）
	//両軸でちぐはぐにカットするような場合は、浮いているほうの軸も同じY位置をキープしていなければ、同じラインとはみなされない。
	//※toku ここは、いろいろなカットを試してみて、不都合があれば変更していくようにしないとならないと

	const double d = .005;		//doubleの判定基準です。5μm以内の場合に同じY軸位置とします

	if( flg.cut_z ) {	//今回Z1が切り込んでいる場合、前回のZ位置を判定します。
		if(	f_eq( s[_Y].pos , last.s[_Y].pos , d )	== true	)	{	same_z=true;	}
	}else {same_z=true;}
	//
	if( flg.cut_w ) {	//今回Z1が切り込んでいる場合、前回のZ位置を判定します。
		if(	f_eq( s[_V].pos , last.s[_V].pos , d )	== true	)	{	same_w=true;	}
	}else {same_w=true;}

	return	(bool)((same_z==true) && (same_w ==true) );
}
_motion_set	&_motion_set::invalid(void)
{
	_multi_motion::invalid();		//基底クラスのinvaid()
	int i;for(i=0 ; i < MOTION_SET_N_SINGLE ; i++) {s[i].invalid();	}
	return	*this;
}

//（二つのカット動作の比較用)
//二つのモーションがある誤差（時間的な）以内で一致しているかどうかを返します。
//軸動作は、1チック分(1ms）誤差を持ちます。

//軸によってその物差しが違うので、
//bool	_motion_set::is_same(const _motion_set &a ,int gosa_ms)	//ある計測誤差	gosa_ms[ms]分以内で一致しているかどうか
//{
//	//自分が、あいてよりも小さい値の場合、
//	//自分＋1チック先の位置の間に、	相手がいる
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
//外部ファイルからマルチ・シングルモーションを読みだします。
//モジュールへの変更を伴うので、ロード作業はこのオブジェクトで行います。
//対比として、save()も用意したほうがいいかと思ったのですが、書きだしはいいかと思ってやめました。
bool	cutline::load(HANDLE fh,int n_multi,int n_single)
{
	//ファイルからデータを読みます。一応サイズをチェックします。
	_Assert(n_multi	< allocated(_MULTI)		, "cutline::load(): multi size oveflow");
	_Assert(n_single < allocated(_SINGL)	, "cutline::load(): single size oveflow");

	::EnterCriticalSection(&_critsec);	DWORD	r;
	if( ::ReadFile(fh,(void*)&_multi[0]		,	n_multi*sizeof(_multi_motion)	, &r,NULL) == 0 )	{	_msg( "cutline::load()file read error(0): [%s]" , LastErrorMsg());	goto	_error;		}
	if( ::ReadFile(fh,(void*)&_single[0]	,	n_single*sizeof(_single_motion)	, &r,NULL) == 0 )	{	_msg( "cutline::load()file read error(1): [%s]" , LastErrorMsg());	goto	_error;		}
	//ここまで来れたら読めました。
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
//doubleの時間範囲→unsigned int の時間範囲
_interval<unsigned int>cutline::t_uint(const _interval<int>&i)
{
	_interval<double>dr=t(i);
	_interval<unsigned int>	r ((unsigned int)dr.start , (unsigned int)dr.end);
	return	r;
}

//

//int	cutline::_add_singl_motion(int n,_MOTION _m_array[])
//{
//	int	sav_n_motion = _n_singl;		//登録開始時のインデックスです。
//	_Assert( _n_singl_motion + n < allocated_singl_pool() , "set_motion() motion overflow");
//	_Assert( n>0 , "set_sgl_motion n==0" );
//	::EnterCriticalSection(&_critsec);
//		int i ;for(i=0 ; i < n ; i++ )	{	_single[_n_singl++]	=	_m_array[i];	}
//	::LeaveCriticalSection(&_critsec);
//	return	sav_n_motion;				//先頭のインデックスを
//}
//----------------------------------------------------------------------------------------------------------------
//	同じモーションどうかの判定について
//----------------------------------------------------------------------------------------------------------------
//チックのとりかたで、モーションが微妙にずれることがある
//	1ms - 1.5ms で、同チックではあるが、カット範囲にさしかかったチックが誤差を持ちます。
//	その場合、1msの誤差をがあるということ
//	めもりが1msのものさしで、ある目盛のところでカット範囲にさしかかった。
//	もう片方では、ものさしがずれて最大、1msの遅れになる。
//	
//	フラグの	ON	タイミングは、最大1ms	
//				OFF	タイミングは、最大1ms
//	おのおの、遅れる場合がある。
//	その場合、たとえば100mm/secで動作している軸の場合。
//	x = vt -> 100 * 0.001 = 0.1 mm (100μm)ずれることになる

//	
//	計測誤差を＋1msとして計算します。
//	その際の、各軸の動作速度に応じて誤差を計算して、その誤差いないであれば
//	誤差、、
//	片方のモーションが次のモーションの位置、速度と

//同じモーションを記録している場合、
//	あとで検出したモーションが、
//		先行のモーション　+　次モーションとの間にあるとあっているとします。
//		次モーションの予測は、実際の次モーションの場合、データがない場合に困るので、
//		現在の速度、位置、加速度から、次モーションを予測して、その間に入っているものかを
//----------------------------------------------------------------------------------------------------------------
