//================================================================================================
//	Work.cpp		OS依存なしに作りたいです。	加工記録を残す
//================================================================================================
#include	"Work.h"
#include	"commonmodule.h"

//#include	"vm_interface.h"				//VMの記録。
#include	"axis_obj.h"					//axisobj独自の
#include	"axis_record_read.h"			//


//local_axisを意識するかどうか
#include	"local_axis_interface.h"		//
#include	<stdlib.h>						//abs()です
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
//コンストラクタ時点でファイルが決まっていなくても良いです。
//make()を行った時点で、ファイルが指示されていればよいです。
//コンストラクタ時点ではファイル名をなしにしておきます。
work::work()	:	_line(_vm)	{	_filename[0]='\0';	}
//------------------------------------------------------------------------------------------------
//	destructor
//------------------------------------------------------------------------------------------------
work::~work()		{	;	}

//------------------------------------------------------------------------------------------------
//	class imprement
//------------------------------------------------------------------------------------------------
//	X軸動作時に切りこんでいればそれはカットラインとして記録する。
//		呼び出し側は、時間ごとの、各軸のモーション（位置と速度)を送ってきます。
//	うけとる軸は、Z,W,	をまず
//	MsPosX=CtMsPosX=NRead("MS_POSX");
//	MsPosY=CtMsPosY=NRead("ALU_CY_HI");

//	SpdlPosX=NRead("SPDL_POSX");
//	return (pos*-1)+CutParam->SpdlPosX+CutParam->WorkCenterX;	
//	toku WorkCenterX;		は、CSPの多枚貼りの場合のワーク中心位置

//ct_posz , blade_scopeが、時間によってことなる場合どうする￥
//	typedef	struct	{	int phase;	double	t;	double	pos;	double	v;	double a;	} _MOTION;		//ある瞬間の軸の動きです。

//各ZYZモーションからk切りこみワーク中心とCT位置からの座標変換したものをラインに書き込む必要がある

//切りこんでいるものかどうかをチェックします。
//	bit	0	Z1が切りこんでいる		bit	1	Z2が切りこんでいる。
static int __aho;	//debug
bool	work::_kirikomi(int ch ,  _MOTION z , _MOTION x ,_MOTION y)
{
	if ( z.pos < (_vm.dev.work_thick	+	_vm.dev.tape_thick) )	{	//Z方向の位置は、切りこんでいる場所。
#if	1	//
	__aho =1;	//toku breakp
#endif

		//	x,y	がワークの内部にはいっているかを判定します。
		if( _vm.dev.cut_pat == _SQUARE )	{
			int size_x,size_y;	_vm.dev.size(ch,&size_x,&size_y);		//ワークサイズを取得します。
			if	(	(fabs(y.pos) <= size_y/2) && (fabs(x.pos) <= size_x/2)	) {	goto _in_work;	}
		} else {											//_ROUND
			double	r = _vm.dev.work_size_r/2;
			if	(f_eq_less(	fabs(y.pos), _vm.dev.work_size_r/2) ) {				//yが半径以内であれば
//_printf("r[%lf]  y[%lf] \n"  , r , y.pos );
//_printf("r*r=[%lf]  y*y=[%lf] ->%lf \n"  , (r*r) , (y.pos * y.pos) , (r*r) - (y.pos * y.pos) );
				if ( f_eq_less(fabs(x.pos) , sqrt((r*r) - (y.pos * y.pos))) )	{	//xがワーク範囲内であれば
//_printf("x.pos:%lf  < sqrt(%lf)\r\n"  , fabs(x.pos) , sqrt((r*r) - (y.pos * y.pos)) );
					goto _in_work;			//余裕幅を含めるかどうか。?は、また	ここでワーク範囲内です。
				}
			}
		}
	}
	return false;
_in_work:
	return true;
}
//6軸モーションを指示してください。
bool	work::kirikomi( _MOTION m[] , int axis )
{
	int ch = _line.cut_ch(m[_T]);	//toku θモーションから、chを求める関数は、cutlineオブジェクトにあります。
	//まだθがカットチャンネルのどれにもあてはまらない場合にはfalseとします。
	if( ch < 0 ) return false;

	if (axis == _Z2 )	{
		return	_kirikomi( ch,m[_W] , m[_X] , m[_V] );
	}
	return	_kirikomi( ch,m[_Z] , m[_X] , m[_Y] );
}
//------------------------------------------------------------------------------------------------------------
//	ワークオブジェクトでは、各軸の座標を、
//	Y軸について：
//	該当のモーションが、	ブレード（カット動作）での該当位置なのか、
//							顕微鏡での位置（ｱﾗｲﾒﾝﾄや、ｶｰﾌﾁｪｯｸ)なのか、
//							わからない。

//						→	単純に顕微鏡中心をさし引くと、顕微鏡中心からのY軸位置になる。

//							blade_scopeを足すと、ブレードがカットしている位置を、顕微鏡がみる位置へ変換されます。
//							
//							問題①	もし、カーフチェック中のY軸位置から、blade_scopeを引いてしまうと、
//									意味のない位置になってしまう。
//									
//									→だけど、この座標は、
//									「いまブレードがある位置」を、ワーク中心を原点とした座標で表すもの。
//									と、とらえると、よい。
//									もし、カーフチェック動作中の場合は、
//									カーフチェックを行っている最中の、ブレードの位置をトレースしていることになります。
//									この場合には、この座標だと合わないので、さらに顕微鏡の位置へ変換する必要があります。


//	この時点での、座標変換は
//		X:	ワーク中心（Spndl_PosX基準)からの相対的な、ブレードの位置です。
//		Y:	ワーク中心位置(Alu_Cy_Hi 基準)からの相対的な、ブレードの位置です。
//		Z:	CT上面からの、ブレードの位置です。（ハイト量）
//		T：	HOME_POSTからの相対距離としておきます。カット中であれば基本的に0または90°となります。
//	return (pos*-1)+CutParam->SpdlPosX+CutParam->WorkCenterX;

//		切りこんでいる判定。
//	→	座標が、
void	work::workcenter_origin(unsigned int t , _MOTION m[] )
{
	_vm.update(t);											//VMをこの時刻へセットします。

	//X
	m[_X].pos	-=	_vm.mac.spdl_posx;							//CT中心からの座標は	spndl_posxa

	//Y
	{
		m[_Y].pos	-=	_vm.mac.alu_cy_hi;							//顕微鏡中心座標。
//toku	ali_pos_yかも
		m[_Y].pos	-=	_vm.mac.b_blade_scope;						//ブレードスコープを
	}
	//V
	{
		m[_V].pos	=	_vm.mac.b_blade_scope_v	-	m[_V].pos;	//
		m[_V].pos	-=	_vm.mac.alu_cy_hi;							//顕微鏡中心座標。
	}
	//Z
	{
		m[_Z].pos	-=	_vm.mac.b_ct_posz;	
	m[_W].pos	-=	_vm.mac.b_ct_posw;	
		m[_Z].pos	*= -1;					m[_W].pos	*= -1;					//ハイト量にするので-1をかけます。
	}
	//T]
	{	m[_T].pos	-=	_vm.mac.homepos['T'];		}

	//return m;
}
//	motion_array[]	は	6軸(XYVZWT)分。
//	モーション配列	m[]は、enum	{	_X , _Y , _V , _Z , _W , _T	};の順序でいれてください。
static	int	__a=0,__b=0,__c=0;		//
extern	int	__ca,__cb,__cc,__cd;	//
extern	int	__ce,__cf,__cg,__ch;
void	_debug_reset(void)
{
	
}


bool	work::record( unsigned long tick, _MOTION m[] )
{
	//---------------------------------------------------------------------------------------------------
	//	この前段にて、ファイル保存された記録データを読み込み、
	//---------------------------------------------------------------------------------------------------
//	int i,n=n_record();		AXIS_RECORD_DATA	d;	//(12 byte)
//	for( i=0 ; i < n ; i++ ) {
//		if ( get_record(i,d) != true)	{	_msg("cut_confirm() get_record_error"); goto _error;	}
//		unsigned long t;	for( t=old_tick ; t < d.tick ; t++ )	{	//この時間のVM読み込みです。
//			_work.record( t ,get_multi_motion( (double)t , "XYVZWT", motion_array , 6 , true));
//
////toku XYVZWT	6軸分の今tickのmotionです。
//m[]
//		}
//	}
//CHを特定できない場合。。。Tが動作中の場合など。
//そのときに、どのCHとするか
	//---------------------------------------------------------------------------------------------------
	//motion _arra
	DWORD	t = GetTickCount();
	_vm.update(tick);		//VMの状況を指示されたチックにします。
{	__a +=  GetTickCount() - t;	t = GetTickCount();	}

	//ワーク中心原点＆Zはハイトに変換します。
	workcenter_origin(tick,m);		//座標を変換します。ワーク中心/CT上面/θ原点位置	を原点としたブレードの位置です。

{	__b +=  GetTickCount() - t;	t = GetTickCount();	}

//toku このときθは原点から
	_line.add(tick, m , kirikomi(m,_Z1) , kirikomi(m,_Z2) );

{	__c +=  GetTickCount() - t;	t = GetTickCount();	}

	return true;
}

//ワークオブジェクトが同じ加工かどうかを検証します。
bool	work::operator==( work& a)
{	//マシンデータは一致している必要がある。
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
		//toku この時点で、cutline lが、メモリを解放してしまう！！
		//そのため、
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
	//たんなるCSVとして
		_printf("%d,%lf,%d,%d,%d,%d,%lf,%d,%lf,%d,%lf,%d,%lf,%d,%lf,%d\n"	,
			i					,
			(p+i)->t-(p)->t		,
			(p+i)->flg.ch		,
			(p+i)->flg.line		,
			(p+i)->flg.cut_z	,
			(p+i)->flg.cut_w	,
			s.s[_X].pos	,	s.s[_X].phase	,	s.s[_Y].pos	,	s.s[_Y].phase	,	s.s[_Z].pos	,	s.s[_Z].phase	,
			s.s[_V].pos	,	s.s[_V].phase	,	s.s[_W].pos	,	s.s[_W].phase		);
		::Sleep(1);	//ダンプが全部でないときがあるので
	}
}

void	work::dump(void)
{
	_motion_set	s;	bool	cutf[2];
	int	nest;	interval	l;
	const _multi_motion*p =_line.multi_adr();

	interval	total_t,cut_t;		//トータル時間と一ライン分の時間

	//ダンプが追い付かないときがあるのでスリープをいれながら
	for( l = _line.get_next_line(l) ; l.valid() ; l=_line.get_next_line(l) ) {
		//一ライン分の時間です。
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
			//カットのある部分だけ表示してみます。？
			//(Z1側)
			dumped=false;
			for(int axis=0 ; axis < 2 ; axis++) {
//tok	_multi[l.start - l.end] が、ch**,line**の一群である。
//		CHとLINEは、Run()の際に割り付けている。
//		・新しい番号の割り付けは、
//			・カット切りこみがあり
//			・CH	:	ΘカウンタからのCH番号。
//			・LINE	:	前回のラインがあれば、そのラインと比較して、
//
//			切りこんでいるZ軸に相当している	Y軸位置	が	同じであれば同じラインと判断している。
//			Y軸位置がことなれば、違うと判断している。
//			Z1もZ2も切りこんでいなければ、前回のCHとLINEと同じに属していると判定している。
//		→そのため、cutline::get_a_line()で取得できるカットラインは、
//			最初の切り込み位置（ワークに差し掛かったところ）	～	切り終わり	～	次のラインの切り込み位置の直前	まで
//			のセットになっている。
//			
//			※戻りのときなどに、切りこみが発生すると、違うラインとして計算される。（次のラインは実際ライン番号がずれることになる。)
//		→	このカット動作は、
//				基本的には、一度フラグが経つと、どちらかのブレードが絶え間なくワークに入るので、
//				カット開始、カット終了、が最初にきて、その後何もフラグがたたないで切り抜け～次ラインの切り始めがあとにくっついた形になる
//
//		→	チョッパートラバースでは、空白の（ブレードが切りこんでいない状況の部分が途中に現れると思います）
//			それは問題なし。
//			
//		→	スローイン、アウトでは、切りこんでいる途中で、速度が変化する記録が残っているだろう。
//			dump()では、cutfがたってから、cutfが抜ける部分まで飛ばしてしまうので、
//			cutfが落ちるのを待つときに、
//				もし各軸に速度変化がある（CONSTでないフェイズがあるなら）、それもdumpにしてみること。

//		→	カット途中の
//				カーフチェック／オートセットアップ動作・・
//				localで動作させるか？→動作させない。
//				realでの記録の場合→動作する。その記録はのこしてよい
//				それをcutconfirmではセットアップ動作とか、カーフチェックと認識・・
//				その場合、・・
//				
//			
//		
//		オートセットアップ動作などがはいる場合に、LISTと競合してへんな動作になることを、見極めたい。
//		AxisAbsが、誰が読んだかも記録したほうがいいかも。(LISTなのか、他のプロセスなのか)
//		

				if( !cutf[axis] && (p+i)->cut(axis) ) {							//カットが始まった。
					s = _line.get_motion_set(i,true);
					{	cut_t.start = (unsigned int)s.t;	if( total_t.start<0 )total_t.start = (unsigned int)s.t;}		//カット開始時間

					{int i;for(i=0;i<nest;i++){ _printf("  ");	}}
					_printf("   <-- [Z%d] (%d) x[%lf][%lf][%d] Z1<y[%lf] z[%lf]> Z2<v[%lf] w[%lf]>\n" , axis+1 , i , s.s[_X].pos , s.s[_X].v , s.s[_X].phase , s.s[_Y].pos , s.s[_Z].pos , s.s[_V].pos , s.s[_W].pos);
					::Sleep(1);	//ダンプが全部でないときがあるので
					cutf[axis]=true;	nest++;
					dumped=true;
				} else if( cutf[axis] ) {
					s = _line.get_motion_set(i,true);	//時間かかってしまうかな

					if( !(p+i)->cut(axis) ) {	//カットがぬけた。
						--nest;
						{int i;for(i=0;i<nest;i++){ _printf("  ");	}}
						{	cut_t.end = total_t.end = (unsigned int)s.t;	}		//時間です。
						_printf("       [Z%d] (%d) x[%lf][%lf][%d] Z1<y[%lf] z[%lf]> Z2<v[%lf] w[%lf]> --> (%d ms) \n" , axis+1 , i , s.s[_X].pos , s.s[_X].v ,  s.s[_X].phase , s.s[_Y].pos , s.s[_Z].pos , s.s[_V].pos , s.s[_W].pos , cut_t.n());
						::Sleep(1);	//ダンプが全部でないときがあるので
						cutf[axis]=false;
						dumped=true;
					} else {					//カット中です。
/*
						//toku スローイン。アウトの場合には、ここでカット速度が変化することがあります。
						int a;for(a=0 ; a < _AXIS_IDX_MAX ; a++ ) {
							if( (s.s[a].phase!=PHASE_CONST) && (s.s[a].phase!=PHASE_STOP)	)	{	//動作している軸が
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
			//ここまでカットラインのダンプでした
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
	//カットトータル時間
	_printf("\n == CUT FIN (total %d ms)\n" , total_t.n());
}

//	int i;for( i=0; i < _line.n_multi()  ; i++)	{
//		const _multi_motion*p =_line.multi_adr();
//		//カットフラグ～カットフラグ寝るまでをダンプしてみます。
//if( i==0 )_printf("p+i[%d]:[0x%x]\n" ,  i , p+i);

//	//toku これはZ1側がカットしている場合の
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
//	//カットライン以外の軸の軌跡をダンプしてみる？
//	{
//		int i ;for(i= 0 ; i < _line.n_multi() ; i++ )	{
//			
//		}
//	}

//toku	セーブとロードを作ります。
//
bool	work::save(void)
{
	//vm関連はファイル名を保存するだけにします。
	_Assert( _filename[0]!='\0' , "work::save() filename not exist(execute run()");
	HANDLE fh = open((const char*)_filename,true);
	WORK_SAVE_HEADER	h;	DWORD	w;
	//------------------------------------------------------------------------------
	//	ヘッダ作成・書き込み
	//------------------------------------------------------------------------------
	int total_size =0;	//トータルサイズを確かめます。最後サムを入れたほうがいいかも
	{
		h.header_size			=	sizeof(WORK_SAVE_HEADER);
		h.filename_size			=	sizeof(_filename);				//ファイル名(バッファ)サイズです。
		h.multi_motion_size		=	sizeof(_multi_motion);			//マルチモーション構造体のサイズ。
		h.single_motion_size	=	sizeof(_single_motion);			//シングルモーション構造体のサイズ
		h.n_multi_motion		=	_line.n_multi();				//保存されているマルチモーション個数。
		h.n_single_motion		=	_line.n_single();				//保存されているシングルモーション個数。
		//ヘッダを書きこみます。
		if(	::WriteFile( fh , (void*)&h , sizeof(WORK_SAVE_HEADER), &w , 0) != TRUE )	{	_printf("write err(header)[%s]" , LastErrorMsg()); goto _error;	}
		total_size =w;
	}
	//------------------------------------------------------------------------------
	//	データ部分書きこみ
	//------------------------------------------------------------------------------
	if(	::WriteFile( fh , (void*)&_filename[0] , h.filename_size , &w , 0) != TRUE )	{	_printf("write err(fname)"); goto _error;	}
	total_size +=w;
	//マルチモーションの書き込みです。
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

//	//まずヘッダを読み込みます。
	if( ::ReadFile(fh,(void*)&h , sizeof(WORK_SAVE_HEADER) , &r,NULL) == 0 ) {_msg( "work::load()::readfile error: [%s]" , LastErrorMsg());	goto _error;	}
	//ヘッダを読み、各サイズをチェックします。
		if(	h.header_size			!=	sizeof(WORK_SAVE_HEADER))	{ _msg("work::load()::_h.header_size illegal(org=%d / read=%d )"		, sizeof(WORK_SAVE_HEADER) ,h.header_size );		goto	_error;	}
		if(	h.filename_size			>	sizeof(_filename))			{ _msg("work::load()::_h.filename_size illegal(org=%d / read=%d )"		, sizeof(_filename) , h.filename_size );			goto	_error;	}
		if(	h.multi_motion_size		!=	sizeof(_multi_motion))		{ _msg("work::load()::_h.multi_motion_size illegal(org=%d / read=%d )"	, sizeof(_multi_motion), h.multi_motion_size	);	goto	_error;	}
		if(	h.single_motion_size	!=	sizeof(_single_motion))		{ _msg("work::load()::_h.single_motion_size illegal(org=%d / read=%d )"	, sizeof(_single_motion), h.single_motion_size	);	goto	_error;	}
		if(	h.n_multi_motion		>	_line.allocated(_MULTI))	{ _msg("work::load()::_h._n_multi overflow(org=%d / read=%d )"			, _line.allocated(_MULTI) , h.n_multi_motion	);	goto	_error;	}
		if(	h.n_single_motion		>	_line.allocated(_SINGL))	{ _msg("work::load()::_h._n_single overflow(org=%d / read=%d )"			, _line.allocated(_SINGL) , h.n_single_motion	);	goto	_error;	}

	//サイズ大丈夫なのでデータをコピーしていきます。
	//ファイルポインタは現在ヘッダの終点です。順番に読んでいきます。
	{
		if( ::ReadFile(fh,(void*)&_filename[0]	, h.filename_size	, &r,NULL) == 0 )	{	_msg( "work::load()file read error(0): [%s]" , LastErrorMsg());	goto	_error;		}
		if( _line.load(fh,h.n_multi_motion , h.n_single_motion)!=true	)				{	_msg( "work::load()file read error(1): [%s]" , LastErrorMsg());	goto	_error;		}
	}

	//ここでファイルがよめました。ファイル名に応じてVMと軸データを読み込みます。
	{
_printf("file=[%s]\n" , _filename);
		if(	_vm.read(_filename)	!=	true)	{	goto _error;	}
	}
	return true;
_error:	
if(fh>=0)	::CloseHandle(fh);

	return	false;
}
#define		VOLUME		""															//ファイルアクセスするためのボリューム名。パスの前につける必要があります
#define		EXT			".work"																	//KcMapファイルの拡張子
#define		_FILE_DIR	"C:\\Disco\\6000_toku\\V250_CUT_CONFIRM\\soft\\NT\\cut_confirm\\"		//保存先ディレクトリです。
static	void	_make_path(char*buff , const char*fname ,unsigned int max )
{
	_Assert( (strlen(VOLUME) + strlen(_FILE_DIR) , strlen(fname) + strlen(EXT)) < max , "[axis_record:_make_path] buff not enough file=%s max=%d" , fname , max );
	sprintf(buff,"%s%s%s%s" ,VOLUME , _FILE_DIR , fname , EXT);
}
HANDLE	work::open(const char *filename , bool write)
{
	char path[512];	_make_path(path,filename,sizeof(path));		//filenameは、ファイル名だけを指示します。適当な拡張子をつけて開きます。
	unsigned long r;	//ReadFileの読み込みバイト数
	//既にファイルが存在している場合はそのまま返します
//	if( _access( filename , 0 ) == 0)	return true;
	//ファイルオープン
	//
	DWORD	dwCreationDisposition;	//オープンするときのファイル存在するかどうかを
	if( write ){
		if( _access( path , 0 ) == 0 ) {
				dwCreationDisposition=	TRUNCATE_EXISTING;
		} else {	dwCreationDisposition=	CREATE_ALWAYS;	}
	}else {		//読み込みの場合にはかならずファイルが存在する前提とします。
		dwCreationDisposition=OPEN_EXISTING;
	}

	HANDLE	fh = ::CreateFile(
				path									,		//ファイル名
				write ? GENERIC_WRITE: GENERIC_READ		,		//読み込みオープン
				(write ? (FILE_SHARE_WRITE):0) | (FILE_SHARE_READ)	,		//共有モード：読み込みは許可
				NULL					,		//子プロセスにハンドルの継承を許すか（NULL:許さない）
				dwCreationDisposition	,		//
				FILE_ATTRIBUTE_NORMAL	,		//ファイル属性：属性なし
				NULL );
	//オープン失敗
	_Assert( fh != INVALID_HANDLE_VALUE , "[work::fileopen] FileOpen error %s [%s]\n" , filename , LastErrorMsg()	);
	return	fh;
}

//指示
bool	work::run(const char*filename )
{
	_printf("_MOTION=%d / single_motion=%d / multi_motoino=%d / motion_set=%d\n" , sizeof(_MOTION), sizeof(_single_motion) , sizeof(_multi_motion) , sizeof(_motion_set));
	//各
	if(	_vm.read(filename)	!=	true)	{	goto _error;	}//デバイスデータ・マシンデータを読み込みます。

	//軸動作記録を読み込みます。
	if( axis_file_read(filename)!= true	)	{	goto _error;	}

	//まずVMから、各種基準点を求めます。
	//変数は、時刻によって変化したりします。
	//さしあたり書いてみます。
	//ローカル軸を用意して、記録通りに動作させてみます。
	{	local_axis_init(&_vm);		}	//これでローカル軸ができました。

	//軸動作をチェックしていきます。
	{
		unsigned long prev_tick=0;						//ひとつ前の
		int i,n=n_record();		AXIS_RECORD_DATA	d;	//(12 byte)
#if 0//
		for( i=0 ;  i < 50 ; i++ )	{
#else
		for( i=0 ;  i < n ; i++ )	{
#endif
			//	（軸動作命令の記録）	を一つ取得します。
			if ( get_record(i,d) != true)	{	_msg("cut_confirm() get_record_error"); goto _error;	}

			//前回の指示から、今回の指示までのモーションをワークオブジェクトに教えます。
			if( prev_tick ) {
				_MOTION motion_array[6];	//6軸分のモーションです。
DWORD _t = GetTickCount();
//ここに何秒もかかってしまうようです。
				unsigned long t;	for( t=prev_tick ; t < d.tick ; t++ )	{	//この時間のVM読み込みです。
					record( t ,get_multi_motion( (double)t , "XYVZWT", motion_array , 6 , true));
				}

_printf("n(%d) ->  t[%d]ms  __a[%d] __b[%d] __c[%d] __ca[%d] __cb[%d] __cc[%d] __cd[%d]\n" ,	d.tick - prev_tick , GetTickCount() - _t  , __a ,__b ,__c,__ca,__cb,__cc,__cd);
_printf("  __ce[%d] __cf[%d] __cg[%d] __ch[%d]\n" , __ce=0,__cf=0,__cg=0,__ch=0);
			}
			//取得したデータから、ローカルの軸を動作させます。
			local_axis_call(d.tick , d.r4_axis, d.r0_cmd , d.r5 , d.r6 , d.r7 );
			prev_tick = d.tick;		//前回のチックを保存します
		}
	}
	//ここでできました。ワークオブジェクトのファイル名をここで確定とします。
	strcpy_lim(	_filename , sizeof(_filename) , (char*)filename );
	return	true;
_error:
	_filename[0]='\0';	//失敗した場合にはファイル名を無効にします。
	return	false;
}


//
//	//サイズをチェックします。
//	{
//		//ヘッダのフォーマットが合っているかどうか
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
//	//サイズ大丈夫なのでデータをコピーしていきます。
//	//ファイルポインタは現在ヘッダの終点です。順番に読んでいきます。
//	{
//		//各種 VMENTRY
//		if( ::ReadFile(fh,(void*)&NEntry	, _h.N_entry_size		, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(0): [%s]" , LastErrorMsg());	goto	_error;		}
//		if( ::ReadFile(fh,(void*)&SEntry	, _h.S_entry_size		, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(1): [%s]" , LastErrorMsg());	goto	_error;		}
//		if( ::ReadFile(fh,(void*)&BEntry	, _h.B_entry_size		, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(2): [%s]" , LastErrorMsg());	goto	_error;		}
//		if( ::ReadFile(fh,(void*)&VM_MMTBL	, _h.vm_mmtbl_size		, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(3): [%s]" , LastErrorMsg());	goto	_error;		}
//		if( ::ReadFile(fh,(void*)&VMDATA	, _h.vmdata_size		, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(4): [%s]" , LastErrorMsg());	goto	_error;		}
//		//ハッシュテーブル
//		if( ::ReadFile(fh,(void*)&NHashTbl[0], _h.vm_hash_tbl_size	, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(5): [%s]" , LastErrorMsg());	goto	_error;		}
//		if( ::ReadFile(fh,(void*)&SHashTbl[0], _h.vm_hash_tbl_size	, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(6): [%s]" , LastErrorMsg());	goto	_error;		}
//		if( ::ReadFile(fh,(void*)&BHashTbl[0], _h.vm_hash_tbl_size	, &r,NULL) == 0 )	{	_msg( "vm_read_data::file read e//rror(7): [%s]" , LastErrorMsg());	goto	_error;		}
//		//VMトレース
//		if( ::ReadFile(fh,(void*)&_trace_record[0], _h.vmtrace_record_size,&r,NULL)==0)	{	_msg( "vm_read_data::file read e//rror(7): [%s]" , LastErrorMsg());	goto	_error;		}
//		if( ::ReadFile(fh,(void*)&_trace_pool[0], _h.vmtrace_pool_size,&r,NULL)==0)		{	_msg( "vm_read_data::file read e//rror(7): [%s]" , LastErrorMsg());	goto	_error;		}
//	}
//
//	return true;
//	//これでコピーはおしまいです。
//	//アドレスを変換しますか?
//	//ファイル作成できたら即クローズします
//	//成功としてファイル名を
//	strcpy_lim(	_filename , sizeof(_filename) , (char*)filename );
//	::CloseHandle(fh);
//	return true;
//_error:
//	::CloseHandle(fh);
//	_filename[0]=0;	//失敗の場合は、未初期化としておきます。
//////////////////////

