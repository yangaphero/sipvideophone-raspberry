#ifndef _RTPAVCSEND_H
#define _RTPAVCSEND_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// RTP 送信用コンテキスト
typedef struct RtpSend_t {
	int sock;					// RTP送信用ソケット
	struct sockaddr_in peer;	// RTP送信宛先アドレス
	unsigned short seqno;	// 次に送信するパケットのシーケンス番号
	unsigned char ptype;		// ペイロードタイプ
	unsigned long ssrc;		// SSRC
	unsigned char pending_buff[1024*640];	// 送信待ちのバッファ。1フレーム分
	int pending_len;	// 送信待ちバッファに入っているバイト数
} RtpSend;

//=============================
int rtpopen(RtpSend** pctx_out, unsigned long ssrc, int ptype, int sock, struct sockaddr_in *peer);
void rtpclose(RtpSend* ctx);

//=============================
int AvcAnalyzeAndSend(RtpSend* ctx, const unsigned char* data, int datalen);
#endif	/* _RTPAVCSEND_H */
