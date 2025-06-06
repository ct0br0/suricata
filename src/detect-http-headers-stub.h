/* Copyright (C) 2007-2019 Open Information Security Foundation
 *
 * You can copy, redistribute or modify this Program under the terms of
 * the GNU General Public License version 2 as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * Stub for per HTTP header detection keyword. Meant to be included into
 * a C file.
 */

/**
 * \ingroup httplayer
 *
 * @{
 */

#include "suricata-common.h"
#include "flow.h"

#include "htp/htp_rs.h"

#include "detect.h"
#include "detect-parse.h"
#include "detect-engine.h"
#include "detect-engine-buffer.h"
#include "detect-engine-mpm.h"
#include "detect-engine-prefilter.h"

#include "util-debug.h"
#include "rust.h"

static int g_buffer_id = 0;

#ifdef KEYWORD_TOSERVER
static InspectionBuffer *GetRequestData(DetectEngineThreadCtx *det_ctx,
        const DetectEngineTransforms *transforms, Flow *_f,
        const uint8_t _flow_flags, void *txv, const int list_id)
{
    SCEnter();

    InspectionBuffer *buffer = InspectionBufferGet(det_ctx, list_id);
    if (buffer->inspect == NULL) {
        htp_tx_t *tx = (htp_tx_t *)txv;

        if (htp_tx_request_headers(tx) == NULL)
            return NULL;

        const htp_header_t *h = htp_tx_request_header(tx, HEADER_NAME);
        if (h == NULL || htp_header_value(h) == NULL) {
            SCLogDebug("HTTP %s header not present in this request",
                       HEADER_NAME);
            return NULL;
        }

        const uint32_t data_len = (uint32_t)htp_header_value_len(h);
        const uint8_t *data = htp_header_value_ptr(h);

        InspectionBufferSetupAndApplyTransforms(
                det_ctx, list_id, buffer, data, data_len, transforms);
    }

    return buffer;
}

static InspectionBuffer *GetRequestData2(DetectEngineThreadCtx *det_ctx,
        const DetectEngineTransforms *transforms, Flow *_f, const uint8_t _flow_flags, void *txv,
        const int list_id)
{
    SCEnter();

    InspectionBuffer *buffer = InspectionBufferGet(det_ctx, list_id);
    if (buffer->inspect == NULL) {
        uint32_t b_len = 0;
        const uint8_t *b = NULL;

        if (SCHttp2TxGetHeaderValue(txv, STREAM_TOSERVER, HEADER_NAME, &b, &b_len) != 1)
            return NULL;
        if (b == NULL || b_len == 0)
            return NULL;

        InspectionBufferSetupAndApplyTransforms(det_ctx, list_id, buffer, b, b_len, transforms);
    }

    return buffer;
}

#endif
#ifdef KEYWORD_TOCLIENT
static InspectionBuffer *GetResponseData(DetectEngineThreadCtx *det_ctx,
        const DetectEngineTransforms *transforms, Flow *_f,
        const uint8_t _flow_flags, void *txv, const int list_id)
{
    SCEnter();

    InspectionBuffer *buffer = InspectionBufferGet(det_ctx, list_id);
    if (buffer->inspect == NULL) {
        htp_tx_t *tx = (htp_tx_t *)txv;

        if (htp_tx_response_headers(tx) == NULL)
            return NULL;

        const htp_header_t *h = htp_tx_response_header(tx, HEADER_NAME);
        if (h == NULL || htp_header_value(h) == NULL) {
            SCLogDebug("HTTP %s header not present in this request",
                       HEADER_NAME);
            return NULL;
        }

        const uint32_t data_len = (uint32_t)htp_header_value_len(h);
        const uint8_t *data = htp_header_value_ptr(h);

        InspectionBufferSetupAndApplyTransforms(
                det_ctx, list_id, buffer, data, data_len, transforms);
    }

    return buffer;
}

static InspectionBuffer *GetResponseData2(DetectEngineThreadCtx *det_ctx,
        const DetectEngineTransforms *transforms, Flow *_f, const uint8_t _flow_flags, void *txv,
        const int list_id)
{
    SCEnter();

    InspectionBuffer *buffer = InspectionBufferGet(det_ctx, list_id);
    if (buffer->inspect == NULL) {
        uint32_t b_len = 0;
        const uint8_t *b = NULL;

        if (SCHttp2TxGetHeaderValue(txv, STREAM_TOCLIENT, HEADER_NAME, &b, &b_len) != 1)
            return NULL;
        if (b == NULL || b_len == 0)
            return NULL;

        InspectionBufferSetupAndApplyTransforms(det_ctx, list_id, buffer, b, b_len, transforms);
    }

    return buffer;
}
#endif

/**
 * \brief this function setup the http.header keyword used in the rule
 *
 * \param de_ctx   Pointer to the Detection Engine Context
 * \param s        Pointer to the Signature to which the current keyword belongs
 * \param str      Should hold an empty string always
 *
 * \retval 0       On success
 */
static int DetectHttpHeadersSetupSticky(DetectEngineCtx *de_ctx, Signature *s, const char *str)
{
    if (SCDetectBufferSetActiveList(de_ctx, s, g_buffer_id) < 0)
        return -1;

    if (SCDetectSignatureSetAppProto(s, ALPROTO_HTTP) < 0)
        return -1;

    return 0;
}

static void DetectHttpHeadersRegisterStub(void)
{
    sigmatch_table[KEYWORD_ID].name = KEYWORD_NAME;
#ifdef KEYWORD_NAME_LEGACY
    sigmatch_table[KEYWORD_ID].alias = KEYWORD_NAME_LEGACY;
#endif
    sigmatch_table[KEYWORD_ID].desc = KEYWORD_NAME " sticky buffer for the " BUFFER_DESC;
    sigmatch_table[KEYWORD_ID].url = "/rules/" KEYWORD_DOC;
    sigmatch_table[KEYWORD_ID].Setup = DetectHttpHeadersSetupSticky;
#if defined(KEYWORD_TOSERVER) && defined(KEYWORD_TOSERVER)
    sigmatch_table[KEYWORD_ID].flags |=
            SIGMATCH_OPTIONAL_OPT | SIGMATCH_INFO_STICKY_BUFFER | SIGMATCH_SUPPORT_DIR;
#else
    sigmatch_table[KEYWORD_ID].flags |= SIGMATCH_NOOPT | SIGMATCH_INFO_STICKY_BUFFER;
#endif

#ifdef KEYWORD_TOSERVER
    DetectAppLayerMpmRegister(BUFFER_NAME, SIG_FLAG_TOSERVER, 2, PrefilterGenericMpmRegister,
            GetRequestData, ALPROTO_HTTP1, HTP_REQUEST_PROGRESS_HEADERS);
    DetectAppLayerMpmRegister(BUFFER_NAME, SIG_FLAG_TOSERVER, 2, PrefilterGenericMpmRegister,
            GetRequestData2, ALPROTO_HTTP2, HTTP2StateDataClient);
#endif
#ifdef KEYWORD_TOCLIENT
    DetectAppLayerMpmRegister(BUFFER_NAME, SIG_FLAG_TOCLIENT, 2, PrefilterGenericMpmRegister,
            GetResponseData, ALPROTO_HTTP1, HTP_RESPONSE_PROGRESS_HEADERS);
    DetectAppLayerMpmRegister(BUFFER_NAME, SIG_FLAG_TOCLIENT, 2, PrefilterGenericMpmRegister,
            GetResponseData2, ALPROTO_HTTP2, HTTP2StateDataServer);
#endif
#ifdef KEYWORD_TOSERVER
    DetectAppLayerInspectEngineRegister(BUFFER_NAME, ALPROTO_HTTP1, SIG_FLAG_TOSERVER,
            HTP_REQUEST_PROGRESS_HEADERS, DetectEngineInspectBufferGeneric, GetRequestData);
    DetectAppLayerInspectEngineRegister(BUFFER_NAME, ALPROTO_HTTP2, SIG_FLAG_TOSERVER,
            HTTP2StateDataClient, DetectEngineInspectBufferGeneric, GetRequestData2);
#endif
#ifdef KEYWORD_TOCLIENT
    DetectAppLayerInspectEngineRegister(BUFFER_NAME, ALPROTO_HTTP1, SIG_FLAG_TOCLIENT,
            HTP_RESPONSE_PROGRESS_HEADERS, DetectEngineInspectBufferGeneric, GetResponseData);
    DetectAppLayerInspectEngineRegister(BUFFER_NAME, ALPROTO_HTTP2, SIG_FLAG_TOCLIENT,
            HTTP2StateDataServer, DetectEngineInspectBufferGeneric, GetResponseData2);
#endif

    DetectBufferTypeSetDescriptionByName(BUFFER_NAME, BUFFER_DESC);

    g_buffer_id = DetectBufferTypeGetByName(BUFFER_NAME);
}
