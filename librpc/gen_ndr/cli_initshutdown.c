/*
 * Unix SMB/CIFS implementation.
 * client auto-generated by pidl. DO NOT MODIFY!
 */

#include "includes.h"
#include "../librpc/gen_ndr/cli_initshutdown.h"

struct rpccli_initshutdown_Init_state {
	struct initshutdown_Init orig;
	struct initshutdown_Init tmp;
	TALLOC_CTX *out_mem_ctx;
	NTSTATUS (*dispatch_recv)(struct tevent_req *req, TALLOC_CTX *mem_ctx);
};

static void rpccli_initshutdown_Init_done(struct tevent_req *subreq);

struct tevent_req *rpccli_initshutdown_Init_send(TALLOC_CTX *mem_ctx,
						 struct tevent_context *ev,
						 struct rpc_pipe_client *cli,
						 uint16_t *_hostname /* [in] [unique] */,
						 struct lsa_StringLarge *_message /* [in] [unique] */,
						 uint32_t _timeout /* [in]  */,
						 uint8_t _force_apps /* [in]  */,
						 uint8_t _do_reboot /* [in]  */)
{
	struct tevent_req *req;
	struct rpccli_initshutdown_Init_state *state;
	struct tevent_req *subreq;

	req = tevent_req_create(mem_ctx, &state,
				struct rpccli_initshutdown_Init_state);
	if (req == NULL) {
		return NULL;
	}
	state->out_mem_ctx = NULL;
	state->dispatch_recv = cli->dispatch_recv;

	/* In parameters */
	state->orig.in.hostname = _hostname;
	state->orig.in.message = _message;
	state->orig.in.timeout = _timeout;
	state->orig.in.force_apps = _force_apps;
	state->orig.in.do_reboot = _do_reboot;

	/* Out parameters */

	/* Result */
	ZERO_STRUCT(state->orig.out.result);

	/* make a temporary copy, that we pass to the dispatch function */
	state->tmp = state->orig;

	subreq = cli->dispatch_send(state, ev, cli,
				    &ndr_table_initshutdown,
				    NDR_INITSHUTDOWN_INIT,
				    &state->tmp);
	if (tevent_req_nomem(subreq, req)) {
		return tevent_req_post(req, ev);
	}
	tevent_req_set_callback(subreq, rpccli_initshutdown_Init_done, req);
	return req;
}

static void rpccli_initshutdown_Init_done(struct tevent_req *subreq)
{
	struct tevent_req *req = tevent_req_callback_data(
		subreq, struct tevent_req);
	struct rpccli_initshutdown_Init_state *state = tevent_req_data(
		req, struct rpccli_initshutdown_Init_state);
	NTSTATUS status;
	TALLOC_CTX *mem_ctx;

	if (state->out_mem_ctx) {
		mem_ctx = state->out_mem_ctx;
	} else {
		mem_ctx = state;
	}

	status = state->dispatch_recv(subreq, mem_ctx);
	TALLOC_FREE(subreq);
	if (!NT_STATUS_IS_OK(status)) {
		tevent_req_nterror(req, status);
		return;
	}

	/* Copy out parameters */

	/* Copy result */
	state->orig.out.result = state->tmp.out.result;

	/* Reset temporary structure */
	ZERO_STRUCT(state->tmp);

	tevent_req_done(req);
}

NTSTATUS rpccli_initshutdown_Init_recv(struct tevent_req *req,
				       TALLOC_CTX *mem_ctx,
				       WERROR *result)
{
	struct rpccli_initshutdown_Init_state *state = tevent_req_data(
		req, struct rpccli_initshutdown_Init_state);
	NTSTATUS status;

	if (tevent_req_is_nterror(req, &status)) {
		tevent_req_received(req);
		return status;
	}

	/* Steal possbile out parameters to the callers context */
	talloc_steal(mem_ctx, state->out_mem_ctx);

	/* Return result */
	*result = state->orig.out.result;

	tevent_req_received(req);
	return NT_STATUS_OK;
}

NTSTATUS rpccli_initshutdown_Init(struct rpc_pipe_client *cli,
				  TALLOC_CTX *mem_ctx,
				  uint16_t *hostname /* [in] [unique] */,
				  struct lsa_StringLarge *message /* [in] [unique] */,
				  uint32_t timeout /* [in]  */,
				  uint8_t force_apps /* [in]  */,
				  uint8_t do_reboot /* [in]  */,
				  WERROR *werror)
{
	struct initshutdown_Init r;
	NTSTATUS status;

	/* In parameters */
	r.in.hostname = hostname;
	r.in.message = message;
	r.in.timeout = timeout;
	r.in.force_apps = force_apps;
	r.in.do_reboot = do_reboot;

	status = cli->dispatch(cli,
				mem_ctx,
				&ndr_table_initshutdown,
				NDR_INITSHUTDOWN_INIT,
				&r);

	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}

	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}

	/* Return variables */

	/* Return result */
	if (werror) {
		*werror = r.out.result;
	}

	return werror_to_ntstatus(r.out.result);
}

struct rpccli_initshutdown_Abort_state {
	struct initshutdown_Abort orig;
	struct initshutdown_Abort tmp;
	TALLOC_CTX *out_mem_ctx;
	NTSTATUS (*dispatch_recv)(struct tevent_req *req, TALLOC_CTX *mem_ctx);
};

static void rpccli_initshutdown_Abort_done(struct tevent_req *subreq);

struct tevent_req *rpccli_initshutdown_Abort_send(TALLOC_CTX *mem_ctx,
						  struct tevent_context *ev,
						  struct rpc_pipe_client *cli,
						  uint16_t *_server /* [in] [unique] */)
{
	struct tevent_req *req;
	struct rpccli_initshutdown_Abort_state *state;
	struct tevent_req *subreq;

	req = tevent_req_create(mem_ctx, &state,
				struct rpccli_initshutdown_Abort_state);
	if (req == NULL) {
		return NULL;
	}
	state->out_mem_ctx = NULL;
	state->dispatch_recv = cli->dispatch_recv;

	/* In parameters */
	state->orig.in.server = _server;

	/* Out parameters */

	/* Result */
	ZERO_STRUCT(state->orig.out.result);

	/* make a temporary copy, that we pass to the dispatch function */
	state->tmp = state->orig;

	subreq = cli->dispatch_send(state, ev, cli,
				    &ndr_table_initshutdown,
				    NDR_INITSHUTDOWN_ABORT,
				    &state->tmp);
	if (tevent_req_nomem(subreq, req)) {
		return tevent_req_post(req, ev);
	}
	tevent_req_set_callback(subreq, rpccli_initshutdown_Abort_done, req);
	return req;
}

static void rpccli_initshutdown_Abort_done(struct tevent_req *subreq)
{
	struct tevent_req *req = tevent_req_callback_data(
		subreq, struct tevent_req);
	struct rpccli_initshutdown_Abort_state *state = tevent_req_data(
		req, struct rpccli_initshutdown_Abort_state);
	NTSTATUS status;
	TALLOC_CTX *mem_ctx;

	if (state->out_mem_ctx) {
		mem_ctx = state->out_mem_ctx;
	} else {
		mem_ctx = state;
	}

	status = state->dispatch_recv(subreq, mem_ctx);
	TALLOC_FREE(subreq);
	if (!NT_STATUS_IS_OK(status)) {
		tevent_req_nterror(req, status);
		return;
	}

	/* Copy out parameters */

	/* Copy result */
	state->orig.out.result = state->tmp.out.result;

	/* Reset temporary structure */
	ZERO_STRUCT(state->tmp);

	tevent_req_done(req);
}

NTSTATUS rpccli_initshutdown_Abort_recv(struct tevent_req *req,
					TALLOC_CTX *mem_ctx,
					WERROR *result)
{
	struct rpccli_initshutdown_Abort_state *state = tevent_req_data(
		req, struct rpccli_initshutdown_Abort_state);
	NTSTATUS status;

	if (tevent_req_is_nterror(req, &status)) {
		tevent_req_received(req);
		return status;
	}

	/* Steal possbile out parameters to the callers context */
	talloc_steal(mem_ctx, state->out_mem_ctx);

	/* Return result */
	*result = state->orig.out.result;

	tevent_req_received(req);
	return NT_STATUS_OK;
}

NTSTATUS rpccli_initshutdown_Abort(struct rpc_pipe_client *cli,
				   TALLOC_CTX *mem_ctx,
				   uint16_t *server /* [in] [unique] */,
				   WERROR *werror)
{
	struct initshutdown_Abort r;
	NTSTATUS status;

	/* In parameters */
	r.in.server = server;

	status = cli->dispatch(cli,
				mem_ctx,
				&ndr_table_initshutdown,
				NDR_INITSHUTDOWN_ABORT,
				&r);

	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}

	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}

	/* Return variables */

	/* Return result */
	if (werror) {
		*werror = r.out.result;
	}

	return werror_to_ntstatus(r.out.result);
}

struct rpccli_initshutdown_InitEx_state {
	struct initshutdown_InitEx orig;
	struct initshutdown_InitEx tmp;
	TALLOC_CTX *out_mem_ctx;
	NTSTATUS (*dispatch_recv)(struct tevent_req *req, TALLOC_CTX *mem_ctx);
};

static void rpccli_initshutdown_InitEx_done(struct tevent_req *subreq);

struct tevent_req *rpccli_initshutdown_InitEx_send(TALLOC_CTX *mem_ctx,
						   struct tevent_context *ev,
						   struct rpc_pipe_client *cli,
						   uint16_t *_hostname /* [in] [unique] */,
						   struct lsa_StringLarge *_message /* [in] [unique] */,
						   uint32_t _timeout /* [in]  */,
						   uint8_t _force_apps /* [in]  */,
						   uint8_t _do_reboot /* [in]  */,
						   uint32_t _reason /* [in]  */)
{
	struct tevent_req *req;
	struct rpccli_initshutdown_InitEx_state *state;
	struct tevent_req *subreq;

	req = tevent_req_create(mem_ctx, &state,
				struct rpccli_initshutdown_InitEx_state);
	if (req == NULL) {
		return NULL;
	}
	state->out_mem_ctx = NULL;
	state->dispatch_recv = cli->dispatch_recv;

	/* In parameters */
	state->orig.in.hostname = _hostname;
	state->orig.in.message = _message;
	state->orig.in.timeout = _timeout;
	state->orig.in.force_apps = _force_apps;
	state->orig.in.do_reboot = _do_reboot;
	state->orig.in.reason = _reason;

	/* Out parameters */

	/* Result */
	ZERO_STRUCT(state->orig.out.result);

	/* make a temporary copy, that we pass to the dispatch function */
	state->tmp = state->orig;

	subreq = cli->dispatch_send(state, ev, cli,
				    &ndr_table_initshutdown,
				    NDR_INITSHUTDOWN_INITEX,
				    &state->tmp);
	if (tevent_req_nomem(subreq, req)) {
		return tevent_req_post(req, ev);
	}
	tevent_req_set_callback(subreq, rpccli_initshutdown_InitEx_done, req);
	return req;
}

static void rpccli_initshutdown_InitEx_done(struct tevent_req *subreq)
{
	struct tevent_req *req = tevent_req_callback_data(
		subreq, struct tevent_req);
	struct rpccli_initshutdown_InitEx_state *state = tevent_req_data(
		req, struct rpccli_initshutdown_InitEx_state);
	NTSTATUS status;
	TALLOC_CTX *mem_ctx;

	if (state->out_mem_ctx) {
		mem_ctx = state->out_mem_ctx;
	} else {
		mem_ctx = state;
	}

	status = state->dispatch_recv(subreq, mem_ctx);
	TALLOC_FREE(subreq);
	if (!NT_STATUS_IS_OK(status)) {
		tevent_req_nterror(req, status);
		return;
	}

	/* Copy out parameters */

	/* Copy result */
	state->orig.out.result = state->tmp.out.result;

	/* Reset temporary structure */
	ZERO_STRUCT(state->tmp);

	tevent_req_done(req);
}

NTSTATUS rpccli_initshutdown_InitEx_recv(struct tevent_req *req,
					 TALLOC_CTX *mem_ctx,
					 WERROR *result)
{
	struct rpccli_initshutdown_InitEx_state *state = tevent_req_data(
		req, struct rpccli_initshutdown_InitEx_state);
	NTSTATUS status;

	if (tevent_req_is_nterror(req, &status)) {
		tevent_req_received(req);
		return status;
	}

	/* Steal possbile out parameters to the callers context */
	talloc_steal(mem_ctx, state->out_mem_ctx);

	/* Return result */
	*result = state->orig.out.result;

	tevent_req_received(req);
	return NT_STATUS_OK;
}

NTSTATUS rpccli_initshutdown_InitEx(struct rpc_pipe_client *cli,
				    TALLOC_CTX *mem_ctx,
				    uint16_t *hostname /* [in] [unique] */,
				    struct lsa_StringLarge *message /* [in] [unique] */,
				    uint32_t timeout /* [in]  */,
				    uint8_t force_apps /* [in]  */,
				    uint8_t do_reboot /* [in]  */,
				    uint32_t reason /* [in]  */,
				    WERROR *werror)
{
	struct initshutdown_InitEx r;
	NTSTATUS status;

	/* In parameters */
	r.in.hostname = hostname;
	r.in.message = message;
	r.in.timeout = timeout;
	r.in.force_apps = force_apps;
	r.in.do_reboot = do_reboot;
	r.in.reason = reason;

	status = cli->dispatch(cli,
				mem_ctx,
				&ndr_table_initshutdown,
				NDR_INITSHUTDOWN_INITEX,
				&r);

	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}

	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}

	/* Return variables */

	/* Return result */
	if (werror) {
		*werror = r.out.result;
	}

	return werror_to_ntstatus(r.out.result);
}
