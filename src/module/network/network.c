#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "network.h"
#include "macro.h"
#include "module.h"

void get_network_sty(struct wb_style_sec * sec, struct wb_style_main * msty);
int get_net_fd(struct wb_context * ctx);
void net_set(struct wb_context * ctx);
void network_get(struct wb_event * event, struct wb_context * ctx);
void network_render(struct wb_render * render, struct wb_data * data);

static struct nla_policy attr_policy[NL80211_ATTR_MAX + 1] = {
	[NL80211_ATTR_SSID] = {}
};

static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
	[NL80211_BSS_INFORMATION_ELEMENTS] = {}
};

static struct nl_state * nls = NULL;

static struct conn_state conn = {
	.connecting = 0,
	.connected = 0
};

static struct module_interface mod = {
	.module_name	= "network",
	.parse_sty		= get_network_sty,
	.get_fd			= get_net_fd,
	.set_up			= net_set,
	.handle_event	= network_get,
	.handle_update	= network_render,
	.clean_up		= NULL
};

struct module_interface * mod_init(int id, struct wb_public_api * api){

	/*
	 * size check at compile time
	 */
	_Static_assert(sizeof(struct network_state) <= 
					sizeof(((struct wb_data *)0)->str_val));
	
	printf("net id = %d\n", id);
	mod.id = id;
	mod.data = api;
	return &mod;
}

void network_render(struct wb_render * wrender, struct wb_data * data){
	struct network_state state;
	memcpy(&state, data->str_val, sizeof(struct network_state));

	if (state.conn & NET_CONNECT){
    	printf("Event Triggered network | Wifi Connected to  %s\n", state.ssid);
	} else {
    	printf("Event Triggered network | Wifi is Down\n");
	}
}

void get_network_sty(struct wb_style_sec * sec, struct wb_style_main * main_sty){
	struct wb_public_api * api = mod.data;
	struct network_style * net_sty = calloc(1, sizeof(struct network_style));

	api->style->get_base(&net_sty->base, sec, main_sty);
	char * format = api->style->get_str(sec, "format");

	strncpy(net_sty->format, format, WB_STYLE_STR_SIZE_MAX);
	mod.style = net_sty;
}

static struct nl_msg * forge_trigger_msg(int ifid, int cmd, int flag){
	struct nl_msg * msg = nlmsg_alloc();

	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ,
				nls->family, 0, flag,
				cmd, 0);
	if (nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifid) < 0){
		ON_ERR("trigger msg forge - nl module")
	}

	return msg;
}

void network_get(struct wb_event * event, struct wb_context * ctx){
	nl_recvmsgs_default(nls->sock);
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
	struct wb_public_api * api = mod.data;

	// this handle mlme group
	struct nlmsghdr * nlh = nlmsg_hdr(msg);
	struct genlmsghdr * gnlh = genlmsg_hdr(nlh);

	if (gnlh->cmd != NL80211_CMD_CONNECT && gnlh->cmd != NL80211_CMD_DISCONNECT){
		return NL_OK;
	}

	struct nlattr * attrs[NL80211_ATTR_MAX + 1];
	struct nlattr * bss[NL80211_BSS_MAX + 1];

	genlmsg_parse(nlh, 0, attrs, NL80211_ATTR_MAX, attr_policy);

	struct network_state network_data;
	struct wb_data wbdata;
	wbdata.id = mod.id;

	if (attrs[NL80211_ATTR_REQ_IE]){
		u8 ssid[IEEE80211_MAX_SSID_LEN + 1];

		int ssidlen = parse_ie_ssid(nla_len(attrs[NL80211_ATTR_REQ_IE]),
					  				nla_data(attrs[NL80211_ATTR_REQ_IE]),
					  				ssid);
		if (ssidlen < 0)
				return NL_OK;

		memcpy(network_data.ssid, ssid, sizeof(network_data.ssid));
		network_data.conn = NET_CONNECT;
		network_data.ssid[ssidlen] = 0;	
		memcpy(wbdata.str_val, &network_data, sizeof(struct network_state));

		api->mod->send_data(ctx, &wbdata);

	}
	else {
		network_data.conn = NET_DISCONNECT;
		memcpy(wbdata.str_val, &network_data, sizeof(struct network_state));

		api->mod->send_data(ctx, &wbdata);
	}

	return NL_OK;
}

static int handle_scan(struct nl_msg * msg, void * data){
	struct nlmsghdr * nlh = nlmsg_hdr(msg);
	struct genlmsghdr * gnlh = genlmsg_hdr(nlh);

	struct wb_public_api * api = mod.data;
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

	struct network_state network_data;
	struct wb_data wbdata;
	wbdata.id = mod.id;

	if (bss[NL80211_BSS_INFORMATION_ELEMENTS]){
		u8 ssid[IEEE80211_MAX_SSID_LEN + 1];
		int ssid_len = parse_ie_ssid(nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
									 nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
									 ssid);


		if (memcmp(conn.bssid, nla_data(bss[NL80211_BSS_BSSID]), ETH_ALEN)){
			goto bad_msg;
		}

		memcpy(network_data.ssid, ssid, sizeof(network_data.ssid));
		network_data.conn = NET_CONNECT;
		network_data.ssid[ssid_len] = 0;	
		memcpy(wbdata.str_val, &network_data, sizeof(struct network_state));

		api->mod->send_data(ctx, &wbdata);
	}


	return NL_OK;

	bad_msg:
		return NL_SKIP;
}

static int handle_station(struct nl_msg * msg, void * data){
	struct nlmsghdr * nlh = nlmsg_hdr(msg);
	struct genlmsghdr * gnlh = genlmsg_hdr(nlh);

	struct nlattr * attrs[NL80211_ATTR_MAX + 1];

	genlmsg_parse(nlh, 0, attrs, NL80211_ATTR_MAX, NULL);

	struct nl_msg * rmsg = forge_trigger_msg(nls->ifid, 
					NL80211_CMD_GET_SCAN, NLM_F_DUMP);


	memcpy(conn.bssid, nla_data(attrs[NL80211_ATTR_MAC]), ETH_ALEN);
	conn.connecting = 1;

	nl_send_auto(nls->sock, rmsg);
	nl_recvmsgs_default(nls->sock);	
	return NL_OK;
}

static int nl_recv_valid_cb(struct nl_msg * msg, void * data){
	struct wb_public_api * api = mod.data;
	struct wb_context * ctx = data;
	// this handle init information
	struct nlmsghdr * nlh = nlmsg_hdr(msg);
	struct genlmsghdr * gnlh = genlmsg_hdr(nlh);

	if (gnlh->cmd == NL80211_CMD_NEW_SCAN_RESULTS){
		return handle_scan(msg, data);		
	}

	if (gnlh->cmd == NL80211_CMD_NEW_STATION){
		return handle_station(msg, data);
	}

	if (gnlh->cmd != NL80211_CMD_NEW_INTERFACE)
		return NL_OK;

	struct nlattr * attrs[NL80211_ATTR_MAX + 1];
	struct nlattr * bss[NL80211_BSS_MAX + 1];

	genlmsg_parse(nlh, 0, attrs, NL80211_ATTR_MAX, attr_policy);

	if (attrs[NL80211_ATTR_CHANNEL_WIDTH]){
			struct nl_msg * rmsg = forge_trigger_msg(nls->ifid,
									NL80211_CMD_GET_STATION, NLM_F_DUMP);

			u8 bssid[ETH_ALEN];

			memcpy(bssid, nla_data(attrs[NL80211_ATTR_MAC]), ETH_ALEN);

			nla_put(rmsg, NL80211_ATTR_MAC,
						ETH_ALEN,
						bssid);
			if (nla_put_u32(rmsg, NL80211_ATTR_IFINDEX, nls->ifid) < 0){
				ON_ERR("trigger msg forge - nl module")
			}

			nl_send_auto(nls->sock, rmsg);
			nl_recvmsgs_default(nls->sock);	
	} else {
		struct network_state network_data;
		struct wb_data wbdata;
		wbdata.id = mod.id;

		network_data.conn = NET_DISCONNECT | NET_INIT;
		memcpy(wbdata.str_val, &network_data, sizeof(struct network_state));

		api->mod->send_data(ctx, &wbdata);
	}

	return NL_OK;
}

int get_net_fd(struct wb_context * ctx){
	nls = malloc(sizeof(struct nl_state));
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


void net_set(struct wb_context * ctx){

	struct nl_msg * msg = NULL;

	int ifid = if_nametoindex("wlp1s0");

	if (!ifid){
		ON_ERR("interface index - nl module")
	}

	nls->ifid = ifid;

	msg = forge_trigger_msg(ifid, NL80211_CMD_GET_INTERFACE, NLM_F_DUMP);

	nl_send_auto(nls->sock, msg);

}

