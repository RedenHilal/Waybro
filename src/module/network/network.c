#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "network.h"
#include "macro.h"
#include "module.h"
#include "widget.h"
#include "style.h"

void get_network_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base);

int get_net_fd(struct wb_context * ctx);

void * net_set(struct wb_context * ctx);

void network_get(struct wb_event * event, struct wb_context * ctx, void * data);

void network_render(struct wb_context * ctx, void * data);

static struct nla_policy attr_policy[NL80211_ATTR_MAX + 1] = {
	[NL80211_ATTR_SSID] = {}
};

static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
	[NL80211_BSS_INFORMATION_ELEMENTS] = {}
};


static struct module_interface mod = {
	.module_name	= "network",
	.parse_sty		= get_network_sty,
	.get_fd			= get_net_fd,
	.set_up			= net_set,
	.handle_event	= network_get,
	.emit_layout	= network_render,
	.clean_up		= NULL
};

struct module_interface * mod_init(int id, struct wb_public_api * api)
{
	return &mod;
}

static void
draw_text(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct network_state * state = data;

	struct wb_widget_text_data text = api->widget->default_text(ctx);
	if (state->cns->conn & NET_DISCONNECT) {
		text.string = "Disconnected";
	} else {
		char buffer[64];
		api->mod->sub_text(mod.base_style->format, "ssid", buffer, state->cns->ssid,
						WB_MOD_STRING, sizeof(buffer));
		text.string = buffer;
	}
	LOG_INFO("%s\n", text.string);

	api->widget->text(ctx, &text);
}

const struct wb_widget_callback net_cb = {

};

void network_render(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct network_state * state = data;

	static int id = -1;
	if (id < 0) {
		id = api->widget->allocate_id(ctx);
		api->widget->set_id(ctx, id, state, WB_POINTER_HOVER, &net_cb);
	}

	int events = api->widget->get_event(ctx, id);
	struct wb_widget_rect_special rect = {
		.rect = api->widget->default_rect(ctx, events)
	};

	rect.rect.child_cb = draw_text;
	rect.rect.data = state;

	api->widget->bind_id(ctx, id, &rect);
	api->widget->rect_special(ctx, &rect);
}

void get_network_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base)
{

}

static struct nl_msg * forge_trigger_msg(int ifid, int cmd, int flag){
	struct network_state * state = mod.data;
	struct nl_msg * msg = nlmsg_alloc();

	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ,
				state->nls->family, 0, flag,
				cmd, 0);
	if (nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifid) < 0){
		ON_ERR("trigger msg forge - nl module")
	}

	return msg;
}

void network_get(struct wb_event * event, struct wb_context * ctx, void * data)
{
	struct network_state * state = mod.data;
	nl_recvmsgs_default(state->nls->sock);
}

static int parse_ie_ssid(u16 len, u8 * start, u8 * data){
	u16 pos = 0, adv = 0;
	u8 * ieid, * iel, * ied;

	while(pos < len){
		ieid = start;
		iel = start + 1;
		ied = start + 2;

		pos += *iel;
		if (*ieid == WLAN_EID_SSID){
			memcpy(data, ied, *iel);
			return *iel;
		}
		adv = *iel + 2;
		// align
		adv = (adv + 3) & ~3;
		start += adv;
	}
	return -1;
}

static int nl_recv_msg_cb(struct nl_msg * msg, void * data){
	struct wb_context * ctx = data;
	const struct wb_public_api * api = mod.api;
	struct network_state * state = mod.data;

	// this handle mlme group
	struct nlmsghdr * nlh = nlmsg_hdr(msg);
	struct genlmsghdr * gnlh = genlmsg_hdr(nlh);

	if (gnlh->cmd != NL80211_CMD_CONNECT && gnlh->cmd != NL80211_CMD_DISCONNECT){
		return NL_OK;
	}

	struct nlattr * attrs[NL80211_ATTR_MAX + 1];
	struct nlattr * bss[NL80211_BSS_MAX + 1];

	genlmsg_parse(nlh, 0, attrs, NL80211_ATTR_MAX, attr_policy);

	if (attrs[NL80211_ATTR_REQ_IE]){
		u8 ssid[IEEE80211_MAX_SSID_LEN + 1];

		int ssidlen = parse_ie_ssid(nla_len(attrs[NL80211_ATTR_REQ_IE]),
					  				nla_data(attrs[NL80211_ATTR_REQ_IE]),
					  				ssid);
		if (ssidlen < 0)
				return NL_OK;

		memcpy(state->cns->ssid, ssid, IEEE80211_MAX_SSID_LEN);
		state->cns->conn = NET_CONNECT;
		state->cns->ssid[ssidlen] = 0;	
	}
	else {
		state->cns->conn = NET_DISCONNECT;
		state->cns->ssid[0] = 0;
	}

	api->mod->trigger_update(ctx);
	return NL_OK;
}

static int handle_scan(struct nl_msg * msg, void * data){
	struct nlmsghdr * nlh = nlmsg_hdr(msg);
	struct genlmsghdr * gnlh = genlmsg_hdr(nlh);

	const struct wb_public_api * api = mod.api;
	struct network_state * state = mod.data;
	struct wb_context * ctx = data;

	struct nlattr * attr[NL80211_ATTR_MAX + 1];
	struct nlattr * bss[NL80211_BSS_MAX + 1];

	if (genlmsg_parse(nlh, 0, attr, NL80211_ATTR_MAX, NULL) < 0){
		goto bad_msg;
	}

	if (!attr[NL80211_ATTR_BSS]){
		goto bad_msg;
	}

	if (nla_parse_nested(bss, NL80211_BSS_MAX, attr[NL80211_ATTR_BSS], NULL) < 0){
		goto bad_msg;
	}

	if (bss[NL80211_BSS_INFORMATION_ELEMENTS]){
		u8 ssid[IEEE80211_MAX_SSID_LEN + 1];
		int ssid_len = parse_ie_ssid(nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
									 nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
									 ssid);


		if (memcmp(state->cns->bssid, nla_data(bss[NL80211_BSS_BSSID]), ETH_ALEN)){
			goto bad_msg;
		}

		memcpy(state->cns->ssid, ssid, IEEE80211_MAX_SSID_LEN);
		state->cns->conn = NET_CONNECT;
		state->cns->ssid[ssid_len] = 0;	
		api->mod->trigger_update(ctx);
	}

	return NL_OK;

	bad_msg:
		return NL_SKIP;
}

static int handle_station(struct nl_msg * msg, void * data){
	struct nlmsghdr * nlh = nlmsg_hdr(msg);
	struct genlmsghdr * gnlh = genlmsg_hdr(nlh);

	const struct wb_public_api * api = mod.api;
	struct network_state * state = mod.data;

	struct nlattr * attrs[NL80211_ATTR_MAX + 1];

	genlmsg_parse(nlh, 0, attrs, NL80211_ATTR_MAX, NULL);

	struct nl_msg * rmsg = forge_trigger_msg(state->nls->ifid, 
					NL80211_CMD_GET_SCAN, NLM_F_DUMP);


	memcpy(state->cns->bssid, nla_data(attrs[NL80211_ATTR_MAC]), ETH_ALEN);
	state->cns->connecting = 1;

	nl_send_auto(state->nls->sock, rmsg);
	nl_recvmsgs_default(state->nls->sock);	
	return NL_OK;
}

static int
handle_interface(struct nl_msg * msg, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct network_state * state = mod.data;
	struct wb_context * ctx = data;

	struct nlmsghdr * nlh = nlmsg_hdr(msg);
	struct genlmsghdr * gnlh = genlmsg_hdr(nlh);

	struct nlattr * attrs[NL80211_ATTR_MAX + 1];
	struct nlattr * bss[NL80211_BSS_MAX + 1];

	genlmsg_parse(nlh, 0, attrs, NL80211_ATTR_MAX, attr_policy);

	if (attrs[NL80211_ATTR_CHANNEL_WIDTH]){
			struct nl_msg * rmsg = forge_trigger_msg(state->nls->ifid,
									NL80211_CMD_GET_STATION, NLM_F_DUMP);

			u8 bssid[ETH_ALEN];

			memcpy(state->cns->bssid, nla_data(attrs[NL80211_ATTR_MAC]), ETH_ALEN);

			nla_put(rmsg, NL80211_ATTR_MAC,
						ETH_ALEN,
						state->cns->bssid);
			if (nla_put_u32(rmsg, NL80211_ATTR_IFINDEX, state->nls->ifid) < 0){
				ON_ERR("trigger msg forge - nl module")
			}

			nl_send_auto(state->nls->sock, rmsg);
			nl_recvmsgs_default(state->nls->sock);	
	} 

	else {
	}
}

static int nl_recv_valid_cb(struct nl_msg * msg, void * data){

	// recv all msg and routes handling
	struct nlmsghdr * nlh = nlmsg_hdr(msg);
	struct genlmsghdr * gnlh = genlmsg_hdr(nlh);

	if (gnlh->cmd == NL80211_CMD_NEW_SCAN_RESULTS){
		return handle_scan(msg, data);		
	}

	else if (gnlh->cmd == NL80211_CMD_NEW_STATION){
		return handle_station(msg, data);
	}

	else if (gnlh->cmd == NL80211_CMD_NEW_INTERFACE)
		return handle_interface(msg, data);

	return NL_OK;
}

int get_net_fd(struct wb_context * ctx){
	struct network_state * state = malloc(sizeof(struct network_state));
	struct nl_state * nls = malloc(sizeof(struct nl_state));
	state->nls = nls;
	mod.data = state;

	nls->sock = nl_socket_alloc();
	if (nls->sock == NULL)
			ON_ERR("nl socket allocation failed")

	genl_connect(nls->sock);

	nl_socket_modify_cb(nls->sock, NL_CB_MSG_IN, NL_CB_CUSTOM, nl_recv_msg_cb, ctx);
	nl_socket_modify_cb(nls->sock, NL_CB_VALID, NL_CB_CUSTOM, nl_recv_valid_cb, ctx);

	nl_socket_disable_seq_check(nls->sock);
	nl_socket_set_nonblocking(nls->sock);

	int family = genl_ctrl_resolve(nls->sock, "nl80211");
	int mgid = genl_ctrl_resolve_grp(nls->sock, "nl80211", "mlme");
	int sgid = genl_ctrl_resolve_grp(nls->sock, "nl80211", "scan");

	if (mgid < 0)
			ON_ERR("failed to resolve group id - nl module")

	nls->family = family;
	nls->group = mgid;

	if (nl_socket_add_membership(nls->sock, mgid) < 0)
			ON_ERR("add membership failed - nl module")
	
	if (nl_socket_add_membership(nls->sock, sgid) < 0)
			ON_ERR("add membership failed - nl module")
	
	int fd = nl_socket_get_fd(nls->sock);

	if (fd < 0)
			ON_ERR("fd unavailable - nl module")

	return fd;
}


void *
net_set(struct wb_context * ctx)
{
	struct conn_state * cns = calloc(1, sizeof(struct conn_state));
	struct network_state * state = mod.data;
	struct nl_state * nls = state->nls;
	state->cns = cns;

	struct nl_msg * msg = NULL;

	int ifid = if_nametoindex("wlp1s0");

	if (!ifid){
		ON_ERR("interface index - nl module")
	}

	nls->ifid = ifid;
	msg = forge_trigger_msg(ifid, NL80211_CMD_GET_INTERFACE, NLM_F_DUMP);
	nl_send_auto(state->nls->sock, msg);

	return state;
}

