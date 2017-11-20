#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rtpavcsend.h"

#pragma pack(1)	// 余計なパディングが入らないようにする
// RTPヘッダ
typedef struct rtphead_t {
	unsigned char head1;		// v=2/p=0/x=0/cc=0
	unsigned char ptype;		// MSB=Marker, 残り7bit=payload_type
	unsigned short seqno;	// sequence number
	unsigned long timestamp;	// タイムスタンプ (90kHz)
	unsigned long ssrc;		// SSRC値
} RtpHead;
#pragma pack()

/**
 * RTP送信用ソケットとコンテキストを作成する
 */
int rtpopen(RtpSend** pctx_out, unsigned long ssrc, int ptype, int sock, struct sockaddr_in *peer)
{
	RtpSend* ctx = (RtpSend*)malloc(sizeof(RtpSend));
	memset(ctx, 0, sizeof(RtpSend));
	// コンテキストのパラメータを初期化
	memcpy(&ctx->peer, peer, sizeof(struct sockaddr_in));
	ctx->seqno = 1;		// 本当は乱数
	ctx->ssrc  = ssrc;
	ctx->ptype = ptype;
	ctx->sock  = sock;
	*pctx_out = ctx;
	return 0;
}

/**
 * NAL１つをRTPパケットに入れて送信する。必要に応じてフラグメント処理。
 */
static int rtpsend_nal(RtpSend* ctx, const unsigned char* data, int datalen, unsigned long timestamp)
{
	unsigned char nal_unit_type;
	unsigned char* payload;
	unsigned char buff[4096];
	int single_NAL = 1;
	RtpHead *head = (RtpHead*)buff;
	head->head1 = 0x80;
	head->ptype = ctx->ptype;
	head->seqno = htons(ctx->seqno);
	head->timestamp = htonl(timestamp);
	head->ssrc = htonl(ctx->ssrc);

	payload = (unsigned char*)(head+1);	// RTPペイロードの書き込み位置
	nal_unit_type = data[0] & 0x1f;

	// NAL unit type
	switch (nal_unit_type) {
	case 7://SPS
	case 8://PPS
	case 6://SEI
	case 9://AUD
		//printf("nal_unit_type=%d\n",nal_unit_type);
		head->ptype |= 0x80;//by aphero!小日本编写错误，找了半天，才发现sps pps 中的maker标志为不正确，暂时在这里修改
		break;
	default:
		if (datalen > 1300)
			single_NAL = 0;	// 1300バイトよりい多いときはフラグメント化
		break;
	}
	if (single_NAL) {
		// フラグメントせずにNALをコピーしてそのまま送る
		memcpy(payload, data, datalen);
		// RTPパケット送信
		if (sendto(ctx->sock, buff, datalen+sizeof(RtpHead), 0, (struct sockaddr*)&ctx->peer, sizeof(ctx->peer)) < 0)
			return -1;
		ctx->seqno++;
		return 0;	// フラグメントじゃないのはここで終了
	}

	//フラグメント
	payload[0] = ((data[0]&0x60) | (28/*FU-A*/&0x1f));	// FUindicator: nal_ref_idcはそのままコピーしてNALtypeを28に。
	payload[1] = (data[0]&0x1f);							// FUheader:    NALtypeをコピー
	data++; datalen--; 	// 元のNALユニット・ヘッダをスキップ

	payload[1] |= 0x80;	// 初回はFUheaderのStartビットを立てる
	while (datalen > 0) {
		int len = datalen < 1300 ? datalen : 1300;	// 最大でも1300バイト
		memcpy(payload+2, data, len);	// FUの後ろにデータをコピー
		if (len == datalen) {//最後のデータ
			payload[1] |= 0x40;	// FUheaderにEndフラグを立てる
			head->ptype |= 0x80;	// マーカービットも立てる
		}
		// RTPパケット送信
		if (sendto(ctx->sock, buff, sizeof(RtpHead)+2+len, 0, (struct sockaddr*)&ctx->peer, sizeof(ctx->peer)) < 0)
			return -1;
		ctx->seqno++;	//次に送るシーケンス番号を更新
		head->seqno = htons(ctx->seqno);
		payload[1] &= 0x7f;	// Startビットをクリア
		data += len; datalen -= len;
	}

	return 0;
}

void rtpclose(RtpSend* ctx)
{
	close(ctx->sock);
	free(ctx);
}

// RTPタイムスタンプ値(90kHz)を現在時刻から生成する
unsigned long long last_ts64 =0;
static unsigned long make_timestamp()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long long ts64 = (unsigned long long)tv.tv_sec * 90000 + tv.tv_usec*90/1000;
	//printf("timestamp=%lld\n",ts64-last_ts64);
	last_ts64=ts64;
	return (unsigned long)(ts64 & 0xffffffff);
}

// H.264バイトストリームを解析してNALごとにRTPで送信する
int AvcAnalyzeAndSend(RtpSend* ctx, const unsigned char* data, int datalen)
{
	const unsigned char _startcode[] = {0,0,1};
	const unsigned long ts = make_timestamp();	// タイムスタンプは１NAL内で共通。
	int nal;	// nalの先頭位置
	int i;		// 現在の解析位置
	int begin;	// 今回startcodeを探索開始する位置

	if (ctx->pending_len == 0) {
		// 初回。startcodeを探してそこまでは破棄
		for (i=0; i<datalen-sizeof(_startcode); i++) {
			if (memcmp(data+i, _startcode, sizeof(_startcode)) == 0) {
				// startcodeが見つかった -> その次がNALの先頭
				i += sizeof(_startcode);
				memcpy(ctx->pending_buff, data+i, datalen-i);
				data = ctx->pending_buff;
				datalen = ctx->pending_len = datalen-i;
				begin = 0;
				break;
			}
		}
		if (ctx->pending_len == 0)	// みつからなかった
			return 0;
	}
	else {
		begin = ctx->pending_len - sizeof(_startcode)+1;	// 前回の最後からstartcode分手前
		if (begin < 0) begin = 0;
		// すでにデータが入っている -> 続きに追加
		memcpy(ctx->pending_buff + ctx->pending_len, data, datalen);
		data = ctx->pending_buff;
		datalen = ctx->pending_len = ctx->pending_len + datalen;
	}

	nal = 0;
	for (i=begin; i<datalen-sizeof(_startcode); i++) {
		// startcodeを探す
		if (memcmp(&data[i], _startcode, sizeof(_startcode)) != 0)
			continue;
		else {
			// startcodeが見つかったのでその手前までを送信する
			int pre0 = 0;// startcodeの前にある0の個数
			while (nal < i-pre0 && data[i-pre0-1] == 0) pre0++;
			const int nallen = i-nal - pre0;
			if (nallen > 0)
				rtpsend_nal(ctx, &data[nal], nallen, ts);
		}
		nal = i + sizeof(_startcode);// 次のNAL先頭
		i = nal-1;
	}
	if (nal > 0 && datalen - nal > 0) { // 残った分をpending_buffに。
		memmove(&ctx->pending_buff[0], &data[nal], datalen-nal);
		ctx->pending_len = datalen - nal;
	}

	return 0;
}


