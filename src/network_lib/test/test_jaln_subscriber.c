/**
 * @file test_jaln_subscriber.c This file contains tests for jaln_subscriber.c functions.
 *
 * @section LICENSE
 *
 * Source code in 3rd-party is licensed and owned by their respective
 * copyright holders.
 *
 * All other source code is copyright Tresys Technology and licensed as below.
 *
 * Copyright (c) 2012 Tresys Technology LLC, Columbia, Maryland, USA
 *
 * This software was developed by Tresys Technology LLC
 * with U.S. Government sponsorship.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <jalop/jaln_network.h>
#include <jalop/jaln_network_types.h>
#include <string.h>
#include <test-dept.h>
#include <vortex.h>

#include "jal_alloc.h"
#include "jaln_subscriber.h"
#include "jaln_channel_info.h"
#include "jaln_connection.h"
#include "jaln_connection_callbacks_internal.h"
#include "jaln_context.h"
#include "jaln_session.h"
#include "jaln_subscriber.h"
#include "jaln_subscriber_callbacks_internal.h"
#include "jaln_subscriber_state_machine.h"


#define FAKE_HOST "localhost"
#define FAKE_PORT "8888"

#define CHAN_NUM 3
#define NONCE "some_id"
#define OFFSET 5
#define HOST "some_host"

jaln_session *session;
static jaln_context *ctx;
VortexChannel *chan;
VortexFrame *frame;
static bool fail;
static int isMsgSent = 0; // 1 TRUE, 0 FALSE

int fake_vortex_channel_get_number(
	__attribute__((unused)) VortexChannel *chan)
{
	return CHAN_NUM;
}

void fake_vortex_channel_set_received_handler(
	__attribute__((unused)) VortexChannel *chan,
	__attribute__((unused)) VortexOnFrameReceived received,
	__attribute__((unused)) axlPointer user_data)
{
	return;
}

void fake_vortex_connection_set_on_close_full(__attribute__((unused)) VortexConnection *conn,
					__attribute__((unused)) VortexConnectionOnCloseFull on_close,
					__attribute__((unused)) axlPointer data)
{
	return;
}

void fake_jaln_sub_state_reset(
	__attribute__((unused)) jaln_session *session)
{
	return;
}

VortexFrameType mock_vortex_frame_get_type_success(__attribute__((unused)) VortexFrame *frame)
{
	return VORTEX_FRAME_TYPE_ANS;
}

VortexFrameType mock_vortex_frame_get_type_failure(__attribute__((unused)) VortexFrame *frame)
{
	return VORTEX_FRAME_TYPE_ERR;
}

axl_bool mock_frame_handler_failure(__attribute__((unused)) jaln_session *session, 
				__attribute__((unused)) VortexFrame *frame, 
				__attribute__((unused)) uint64_t payload_offset, 
				__attribute__((unused)) axl_bool flag_more)
{
	return axl_false;
}

axl_bool fake_vortex_channel_send_msg_success(
	__attribute__((unused)) VortexChannel *chan, 
	__attribute__((unused)) const void *msg, 
	__attribute__((unused)) size_t msg_size, 
	__attribute__((unused)) int* msg_no)
{
	isMsgSent = 1;
	return axl_true;
}


axl_bool fake_vortex_channel_send_msg_fail(
	__attribute__((unused)) VortexChannel *chan, 
	__attribute__((unused)) const void *msg, 
	__attribute__((unused)) size_t msg_size, 
	__attribute__((unused)) int* msg_no)
{
	isMsgSent = 0;
	return axl_false;
}

void fake_vortex_channel_set_complete_flag(
	__attribute__((unused)) VortexChannel *chan, 
	__attribute__((unused)) axl_bool value)
{
	return;
}

axl_bool fake_vortex_channel_close_full(
	__attribute__((unused)) VortexChannel *chan,
	__attribute__((unused)) VortexOnClosedNotificationFull on_closed,
	__attribute__((unused)) axlPointer user_data)
{
	return axl_true;
}

int fake_subscriber_callbacks_is_valid(struct jaln_subscriber_callbacks *subscriber_callbacks) {
	return (NULL != subscriber_callbacks);
}

int fake_connection_callbacks_is_valid(struct jaln_connection_callbacks *conn_callbacks) {
	return (NULL != conn_callbacks);
}

VortexConnection  *fake_vortex_connection_new(
		__attribute__((unused)) VortexCtx *ctx,
		__attribute__((unused)) const char *host,
		__attribute__((unused)) const char *port,
		__attribute__((unused)) VortexConnectionNew on_connected,
		__attribute__((unused)) axlPointer user_data)
{
	return (VortexConnection*) 0xbadf00d;
}

axl_bool fake_vortex_connection_is_ok(__attribute__((unused)) VortexConnection *connection,
		__attribute__((unused)) axl_bool free_on_fail)
{
	return axl_true;
}

axl_bool mock_frame_handler_success(__attribute__((unused)) jaln_session *session,
				__attribute__((unused)) VortexFrame *frame,
				__attribute__((unused)) uint64_t payload_offset,
				__attribute__((unused)) axl_bool flag_more)
{
	return axl_true;
}

axl_bool mock_vortex_channel_close_full(__attribute__((unused)) VortexChannel *channel, __attribute__((unused)) VortexOnClosedNotificationFull on_closed, __attribute__((unused)) axlPointer user_data)
{
	fail = true;
	return true;
}

VortexChannel *fake_vortex_channel_new(
		__attribute__((unused)) VortexConnection *connection,
		__attribute__((unused)) int channel_num,
		__attribute__((unused)) const char *profile,
		__attribute__((unused)) VortexOnCloseChannel close,
		__attribute__((unused)) axlPointer close_user_data,
		__attribute__((unused)) VortexOnFrameReceived received,
		__attribute__((unused)) axlPointer received_user_data,
		__attribute__((unused)) VortexOnChannelCreated on_channel_created,
		__attribute__((unused)) axlPointer user_data)
{
	return (VortexChannel*) 0xbadf00d;
}

void setup()
{
	fail = false;
	session = jaln_session_create();
	session->role = JALN_ROLE_SUBSCRIBER;
	session->sub_data = jaln_sub_data_create();
	session->sub_data->sm = jaln_sub_state_create_log_machine();
	session->sub_data->sm->curr_state->frame_handler = &mock_frame_handler_success;
	ctx = jaln_context_create();
	ctx->conn_callbacks = jaln_connection_callbacks_create();
	ctx->sub_callbacks = jaln_subscriber_callbacks_create();
	chan = (VortexChannel *) "dummy";
	frame = (VortexFrame *) "dummy";
	replace_function(vortex_channel_close_full, mock_vortex_channel_close_full);
	replace_function(vortex_channel_get_number, 
			fake_vortex_channel_get_number);
	replace_function(vortex_channel_set_received_handler,
			fake_vortex_channel_set_received_handler);
	replace_function(jaln_sub_state_reset, fake_jaln_sub_state_reset);
	replace_function(vortex_channel_send_msg,
			fake_vortex_channel_send_msg_success);
	replace_function(vortex_channel_set_complete_flag,
			fake_vortex_channel_set_complete_flag);
	replace_function(jaln_subscriber_callbacks_is_valid, fake_subscriber_callbacks_is_valid);
	replace_function(jaln_connection_callbacks_is_valid, fake_connection_callbacks_is_valid);
	replace_function(vortex_connection_new, fake_vortex_connection_new);
	replace_function(vortex_connection_is_ok, fake_vortex_connection_is_ok);
	replace_function(vortex_channel_new, fake_vortex_channel_new);

	isMsgSent = 0;
}

void teardown()
{
	if (session && session->sub_data && session->sub_data->sm) {
		jaln_sub_state_machine_destroy(&(session->sub_data->sm));
	}
	jaln_session_destroy(&session);
	session = NULL;
	restore_function(vortex_channel_close_full);
	restore_function(vortex_frame_get_type);
	restore_function(vortex_channel_get_number);
	restore_function(vortex_channel_set_received_handler);
	restore_function(jaln_sub_state_reset);
	restore_function(vortex_channel_set_complete_flag);
	restore_function(vortex_channel_send_msg);
}

void test_jaln_subscriber_record_frame_handler_fails_with_bad_input()
{
	replace_function(vortex_frame_get_type, mock_vortex_frame_get_type_success);

	jaln_subscriber_record_frame_handler(NULL, chan, NULL, NULL);
	assert(fail);

}

void test_jaln_subscriber_record_frame_handler_fails_with_bad_frame_type()
{
	replace_function(vortex_frame_get_type, mock_vortex_frame_get_type_failure);

	jaln_subscriber_record_frame_handler(session, chan, NULL, frame);
	assert(fail);

}

void test_jaln_subscriber_record_frame_handler_frame_handler_returns_false()
{
	session->sub_data->sm->curr_state->frame_handler = &mock_frame_handler_failure;
	replace_function(vortex_frame_get_type, mock_vortex_frame_get_type_success);

	jaln_subscriber_record_frame_handler(session, chan, NULL, frame);
	assert(fail);
}

void test_jaln_subscriber_record_frame_handler_success()
{
	replace_function(vortex_frame_get_type, mock_vortex_frame_get_type_success);

	jaln_subscriber_record_frame_handler(session, chan, NULL, frame);
	assert(!fail);
}

int fake_get_subscribe_request_success(
	__attribute__((unused)) jaln_session *sess,
	__attribute__((unused)) const struct jaln_channel_info *ch_info, 
	enum jaln_record_type type,
	char **nonce,
	__attribute__((unused)) uint64_t *offset)
{
	if (JALN_RTYPE_JOURNAL == type) {
		*nonce = strdup(NONCE);
	}
	*offset = OFFSET;
	return JAL_OK;
}

int fake_get_subscribe_request_success_zero_offset(
	__attribute__((unused)) jaln_session *sess,
	__attribute__((unused)) const struct jaln_channel_info *ch_info,
	__attribute__((unused)) enum jaln_record_type type,
	__attribute__((unused)) char **nonce,
	__attribute__((unused)) uint64_t *offset)
{
	*nonce = strdup(NONCE);
	*offset = 0;
	return JAL_OK;
}

int fake_get_subscribe_request_null_nonce(
	__attribute__((unused)) jaln_session *sess,
	__attribute__((unused)) const struct jaln_channel_info *ch_info,
	__attribute__((unused)) enum jaln_record_type type,
	__attribute__((unused)) char **nonce,
	__attribute__((unused)) uint64_t *offset)
{
	*nonce = NULL;
	*offset = OFFSET;
	return JAL_OK;
}

int fake_get_subscribe_request_fail(
	__attribute__((unused)) jaln_session *sess,
	__attribute__((unused)) const struct jaln_channel_info *ch_info, 
	__attribute__((unused)) enum jaln_record_type type,
	__attribute__((unused)) char **nonce,
	__attribute__((unused)) uint64_t *offset)
{
	return JAL_E_INVAL;
}

void test_jaln_configure_sub_session_no_lock_succeeds()
{	// Pre-conditions
	assert_not_equals(session->rec_chan, chan);
	assert_not_equals(session->rec_chan_num, CHAN_NUM);
	jaln_sub_data_destroy(&session->sub_data);

	session->ch_info->type = JALN_RTYPE_JOURNAL;
	assert_equals(JAL_OK,
		jaln_configure_sub_session_no_lock(chan, session));
	assert_equals(session->rec_chan, chan);
	assert_equals(session->rec_chan_num, CHAN_NUM);
	assert_not_equals(NULL, session->sub_data);
	assert_not_equals(NULL, session->sub_data->sm);
	assert_not_equals(NULL, session->sub_data->curr_frame_handler);

	jaln_session_destroy(&session);
	session = jaln_session_create();
	session->role = JALN_ROLE_SUBSCRIBER;
	session->ch_info->type = JALN_RTYPE_AUDIT;

	assert_equals(JAL_OK,
		jaln_configure_sub_session_no_lock(chan, session));
	assert_equals(session->rec_chan, chan);
	assert_equals(session->rec_chan_num, CHAN_NUM);
	assert_not_equals(NULL, session->sub_data);
	assert_not_equals(NULL, session->sub_data->sm);
	assert_not_equals(NULL, session->sub_data->curr_frame_handler);

	jaln_session_destroy(&session);
	session = jaln_session_create();
	session->role = JALN_ROLE_SUBSCRIBER;
	session->ch_info->type = JALN_RTYPE_LOG;
	assert_equals(JAL_OK,
		jaln_configure_sub_session_no_lock(chan, session));

	// Post-conditions
	assert_equals(session->rec_chan, chan);
	assert_equals(session->rec_chan_num, CHAN_NUM);
	assert_not_equals(NULL, session->sub_data);
	assert_not_equals(NULL, session->sub_data->sm);
	assert_not_equals(NULL, session->sub_data->curr_frame_handler);

	jaln_session_destroy(&session);
}

void test_jaln_configure_sub_session_no_lock_fails_bad_input()
{	// Pre-conditions
	assert_not_equals(session->rec_chan, chan);
	assert_not_equals(session->rec_chan_num, CHAN_NUM);
	jaln_sub_data_destroy(&session->sub_data);
	assert_equals((void*) NULL, session->sub_data);

	assert_not_equals(JAL_OK,
		jaln_configure_sub_session_no_lock(NULL, session));

	assert_not_equals(JAL_OK,
		jaln_configure_sub_session_no_lock(chan, NULL));

	// Post-conditions
	assert_not_equals(session->rec_chan, chan);
	assert_not_equals(session->rec_chan_num, CHAN_NUM);
}

void test_jaln_subscriber_send_subscribe_request_succeeds()
{	// Start w/ valid session object
	session->jaln_ctx = jaln_context_create();;
	session->jaln_ctx->sub_callbacks = jaln_subscriber_callbacks_create();
	session->jaln_ctx->sub_callbacks->get_subscribe_request =
		fake_get_subscribe_request_success;

	session->rec_chan = (VortexChannel *) 0xbadf00d;
	isMsgSent = 0;
	session->ch_info->type = JALN_RTYPE_AUDIT;
	jaln_subscriber_send_subscribe_request(session);
	assert_equals(1, isMsgSent);
	
	isMsgSent = 0;
	session->ch_info->type = JALN_RTYPE_LOG;
	jaln_subscriber_send_subscribe_request(session);
	assert_equals(1, isMsgSent);

	isMsgSent = 0;
	session->ch_info->type = JALN_RTYPE_JOURNAL;
	jaln_subscriber_send_subscribe_request(session);
	assert_equals(1, isMsgSent);
}

void test_jaln_subscriber_send_subscribe_request_fails_bad_input()
{
	// Test session NULL
	jaln_subscriber_send_subscribe_request(NULL);
	assert_equals(0, isMsgSent);
	
	// Test jaln_ctx NULL
	assert_equals((void*)NULL, session->jaln_ctx);
	jaln_subscriber_send_subscribe_request(session);
	assert_equals(0, isMsgSent);
	
	// Test sub_callbacks NULL
	session->jaln_ctx = jaln_context_create();
	assert_equals((void*)NULL, session->jaln_ctx->sub_callbacks);
	jaln_subscriber_send_subscribe_request(session);
	assert_equals(0, isMsgSent);
	
	// Test ch_info NULL
	struct jaln_channel_info *ch_info =  session->ch_info;
	session->ch_info = NULL;
	session->jaln_ctx->sub_callbacks = jaln_subscriber_callbacks_create();
	jaln_subscriber_send_subscribe_request(session);
	assert_equals(0, isMsgSent);
	session->ch_info = ch_info;
	ch_info = NULL;
	
	// Test rec_chan NULL
	assert_equals((void*)NULL, session->rec_chan);
	jaln_subscriber_send_subscribe_request(session);
	assert_equals(0, isMsgSent);
}

void test_jaln_subscriber_send_subscribe_request_fails_internal()
{	// Start w/ valid session object
	session->jaln_ctx = jaln_context_create();
	session->jaln_ctx->sub_callbacks = jaln_subscriber_callbacks_create();
	session->rec_chan = (VortexChannel *) 0xbadf00d;
	
	session->jaln_ctx->sub_callbacks->get_subscribe_request =
		fake_get_subscribe_request_fail;
	jaln_subscriber_send_subscribe_request(session);
	assert_equals(0, isMsgSent);
}

void test_jaln_subscriber_create_session_fails_with_bad_input()
{
	jaln_session *sess = jaln_subscriber_create_session(NULL, HOST, JALN_RTYPE_LOG);
	assert_equals(1, ctx->ref_cnt);
	assert_pointer_equals((void *) NULL, sess);
	assert_equals(1, ctx->ref_cnt);

	sess = jaln_subscriber_create_session(ctx, NULL, JALN_RTYPE_LOG);
	assert_pointer_equals((void *) NULL, sess);
	assert_equals(1, ctx->ref_cnt);

	sess = jaln_subscriber_create_session(ctx, HOST, 0);
	assert_pointer_equals((void *) NULL, sess);
	assert_equals(1, ctx->ref_cnt);
}

void test_jaln_subscriber_create_session_works()
{
	assert_equals(1, ctx->ref_cnt);
	jaln_session *sess = jaln_subscriber_create_session(ctx, HOST, JALN_RTYPE_LOG);
	assert_not_equals((void *) NULL, sess);
	assert_equals(2, ctx->ref_cnt);
	assert_equals(JALN_ROLE_SUBSCRIBER, sess->role);
	assert_equals(JALN_RTYPE_LOG, sess->ch_info->type);
	assert_string_equals(HOST, sess->ch_info->hostname);
	jaln_session_unref(sess);
}

void test_jaln_subscribe_does_not_crash_with_bad_input()
{
	assert_pointer_equals((void *) NULL, jaln_subscribe(NULL, FAKE_HOST, FAKE_PORT, JALN_RTYPE_ALL, JALN_ARCHIVE_MODE, NULL));
	assert_pointer_equals((void *) NULL, jaln_subscribe(ctx, NULL, FAKE_PORT, JALN_RTYPE_ALL, JALN_ARCHIVE_MODE, NULL));
	assert_pointer_equals((void *) NULL, jaln_subscribe(ctx, FAKE_HOST, NULL, JALN_RTYPE_ALL, JALN_ARCHIVE_MODE, NULL));
	assert_pointer_equals((void *) NULL, jaln_subscribe(ctx, FAKE_HOST, FAKE_PORT, 0, JALN_ARCHIVE_MODE, NULL));
	assert_pointer_equals((void *) NULL, jaln_subscribe(ctx, FAKE_HOST, FAKE_PORT, JALN_RTYPE_ALL, JALN_UNKNOWN_MODE, NULL));
}

void test_jaln_subscribe_fails_when_missing_conn_callbacks()
{
	jaln_connection_callbacks_destroy(&ctx->conn_callbacks);
	assert_pointer_equals((void *)NULL, jaln_subscribe(ctx, FAKE_HOST, FAKE_PORT, JALN_RTYPE_ALL, JALN_ARCHIVE_MODE, NULL));
}

void test_jaln_subscribe_fails_when_missing_sub_callbacks()
{
	jaln_subscriber_callbacks_destroy(&ctx->sub_callbacks);
	assert_pointer_equals((void *)NULL, jaln_subscribe(ctx, FAKE_HOST, FAKE_PORT, JALN_RTYPE_ALL, JALN_ARCHIVE_MODE, NULL));
}

void test_jaln_subscribe_fails_when_already_connected()
{
	ctx->is_connected = axl_true;
	assert_pointer_equals((void *) NULL, jaln_subscribe(ctx, FAKE_HOST, FAKE_PORT, JALN_RTYPE_JOURNAL, JALN_ARCHIVE_MODE, NULL));
}

void test_jaln_subscribe_success_for_all_types()
{
	replace_function(vortex_connection_set_on_close_full, fake_vortex_connection_set_on_close_full);
	struct jaln_connection *conn = NULL;
	
	conn = jaln_subscribe(ctx, FAKE_HOST, FAKE_PORT, JALN_RTYPE_ALL, JALN_ARCHIVE_MODE, NULL);
	assert_not_equals((void *) NULL, conn);
	jaln_connection_destroy(&conn);
	restore_function(vortex_connection_set_on_close_full);
}
