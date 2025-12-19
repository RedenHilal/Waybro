#include "core.h"
#include "fetcher.h"
#include "module/network.h"

static struct nl_state * nls = NULL;

static struct conn_state conn = {
	.connecting = 0,
	.connected = 0
};

static struct nla_policy attr_policy[NL80211_ATTR_MAX + 1] = {
	[NL80211_ATTR_SSID] = {}
};

static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
	[NL80211_BSS_INFORMATION_ELEMENTS] = {}
};

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

static void send_ev(char * ssid, int ev){
	char * passable = NULL;
	if (ssid != NULL){
		passable = strdup(ssid);
	}
	write(nls->pipe, &(Event){NETWORK, ev, 0 , passable}, sizeof(Event));
}

void * network_get(void * data){
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
	// this handle mlme group
	struct nlmsghdr * nlh = nlmsg_hdr(msg);
	struct genlmsghdr * gnlh = genlmsg_hdr(nlh);

	if (gnlh->cmd != NL80211_CMD_CONNECT && gnlh->cmd != NL80211_CMD_DISCONNECT){
		return NL_OK;
	}

	struct nlattr * attrs[NL80211_ATTR_MAX + 1];
	struct nlattr * bss[NL80211_BSS_MAX + 1];

	genlmsg_parse(nlh, 0, attrs, NL80211_ATTR_MAX, attr_policy);

	char ssid[IEEE80211_MAX_SSID_LEN + 1];

	if (attrs[NL80211_ATTR_REQ_IE]){

		int ssidlen = parse_ie_ssid(nla_len(attrs[NL80211_ATTR_REQ_IE]),
					  				nla_data(attrs[NL80211_ATTR_REQ_IE]),
					  				ssid);
		if (ssidlen < 0)
				return NL_OK;
		ssid[ssidlen] = 0;
		send_ev(ssid, NET_CONNECT);

	}
	else {
		send_ev(NULL, NET_DISCONNECT);
	}


	return NL_OK;
}

static int handle_scan(struct nl_msg * msg, void * data){
	struct nlmsghdr * nlh = nlmsg_hdr(msg);
	struct genlmsghdr * gnlh = genlmsg_hdr(nlh);


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

		ssid[ssid_len] = 0;

		if (memcmp(conn.bssid, nla_data(bss[NL80211_BSS_BSSID]), ETH_ALEN)){
			goto bad_msg;
		}

		send_ev(ssid, NET_INIT | NET_CONNECT);
		conn.connected = 1;
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
			send_ev(NULL, NET_INIT | NET_DISCONNECT);
	}

	return NL_OK;
}

int get_net_fd(){
	nls = malloc(sizeof(struct nl_state));
	nls->sock = nl_socket_alloc();

	if (nls->sock == NULL)
			ON_ERR("nl socket allocation failed")

	genl_connect(nls->sock);

	nl_socket_modify_cb(nls->sock, NL_CB_MSG_IN, NL_CB_CUSTOM, nl_recv_msg_cb, NULL);
	nl_socket_modify_cb(nls->sock, NL_CB_VALID, NL_CB_CUSTOM, nl_recv_valid_cb, NULL);

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


void net_set(void * data){

	struct fd_object * object = data;
	struct nl_msg * msg = NULL;
	
	nls->pipe = object->pipe;

	int ifid = if_nametoindex("wlp1s0");

	if (!ifid){
		ON_ERR("interface index - nl module")
	}

	nls->ifid = ifid;

	msg = forge_trigger_msg(ifid, NL80211_CMD_GET_INTERFACE, NLM_F_DUMP);

	nl_send_auto(nls->sock, msg);

}


