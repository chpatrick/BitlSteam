/***************************************************************************\
*                                                                           *
*  BitlBee - An IRC to IM gateway                                           *
*  Simple module to facilitate twitter functionality.                       *
*                                                                           *
*  Copyright 2009 Geert Mulders <g.c.w.m.mulders@gmail.com>                 *
*                                                                           *
*  This library is free software; you can redistribute it and/or            *
*  modify it under the terms of the GNU Lesser General Public               *
*  License as published by the Free Software Foundation, version            *
*  2.1.                                                                     *
*                                                                           *
*  This library is distributed in the hope that it will be useful,          *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU        *
*  Lesser General Public License for more details.                          *
*                                                                           *
*  You should have received a copy of the GNU Lesser General Public License *
*  along with this library; if not, write to the Free Software Foundation,  *
*  Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA           *
*                                                                           *
****************************************************************************/

#include "nogaim.h"
#include "oauth.h"
#include "twitter.h"
#include "twitter_http.h"
#include "twitter_lib.h"
#include "url.h"

#define twitter_msg( ic, fmt... ) \
	do {                                            \
		struct twitter_data *td = ic->proto_data;   \
		if( td->timeline_gc )                       \
			imcb_chat_log( td->timeline_gc, fmt );  \
		else                                        \
			imcb_log( ic, fmt );                    \
	} while( 0 );

GSList *twitter_connections = NULL;

/**
 * Main loop function
 */
gboolean twitter_main_loop(gpointer data, gint fd, b_input_condition cond)
{
	struct im_connection *ic = data;

	// Check if we are still logged in...
	if (!g_slist_find(twitter_connections, ic))
		return 0;

	// Do stuff..
	twitter_get_timeline(ic, -1);

	// If we are still logged in run this function again after timeout.
	return (ic->flags & OPT_LOGGED_IN) == OPT_LOGGED_IN;
}

static void twitter_main_loop_start(struct im_connection *ic)
{
	struct twitter_data *td = ic->proto_data;

	imcb_log(ic, "Getting initial statuses");

	// Run this once. After this queue the main loop function.
	twitter_main_loop(ic, -1, 0);

	// Queue the main_loop
	// Save the return value, so we can remove the timeout on logout.
	td->main_loop_id =
	    b_timeout_add(set_getint(&ic->acc->set, "fetch_interval") * 1000, twitter_main_loop, ic);
}

static void twitter_oauth_start(struct im_connection *ic);

void twitter_login_finish(struct im_connection *ic)
{
	struct twitter_data *td = ic->proto_data;

	td->flags &= ~TWITTER_DOING_TIMELINE;

	if (set_getbool(&ic->acc->set, "oauth") && !td->oauth_info)
		twitter_oauth_start(ic);
	else if (g_strcasecmp(set_getstr(&ic->acc->set, "mode"), "one") != 0 &&
		 !(td->flags & TWITTER_HAVE_FRIENDS)) {
		imcb_log(ic, "Getting contact list");
		twitter_get_friends_ids(ic, -1);
		//twitter_get_statuses_friends(ic, -1);
	} else
		twitter_main_loop_start(ic);
}

static const struct oauth_service twitter_oauth = {
	"https://api.twitter.com/oauth/request_token",
	"https://api.twitter.com/oauth/access_token",
	"https://api.twitter.com/oauth/authorize",
	.consumer_key = "xsDNKJuNZYkZyMcu914uEA",
	.consumer_secret = "FCxqcr0pXKzsF9ajmP57S3VQ8V6Drk4o2QYtqMcOszo",
};

static const struct oauth_service identica_oauth = {
	"https://identi.ca/api/oauth/request_token",
	"https://identi.ca/api/oauth/access_token",
	"https://identi.ca/api/oauth/authorize",
	.consumer_key = "e147ff789fcbd8a5a07963afbb43f9da",
	.consumer_secret = "c596267f277457ec0ce1ab7bb788d828",
};

static gboolean twitter_oauth_callback(struct oauth_info *info);

static const struct oauth_service *get_oauth_service(struct im_connection *ic)
{
	struct twitter_data *td = ic->proto_data;

	if (strstr(td->url_host, "identi.ca"))
		return &identica_oauth;
	else
		return &twitter_oauth;

	/* Could add more services, or allow configuring your own base URL +
	   API keys. */
}

static void twitter_oauth_start(struct im_connection *ic)
{
	struct twitter_data *td = ic->proto_data;

	imcb_log(ic, "Requesting OAuth request token");

	td->oauth_info = oauth_request_token(get_oauth_service(ic), twitter_oauth_callback, ic);

	/* We need help from the user to complete OAuth login, so don't time
	   out on this login. */
	ic->flags |= OPT_SLOW_LOGIN;
}

static gboolean twitter_oauth_callback(struct oauth_info *info)
{
	struct im_connection *ic = info->data;
	struct twitter_data *td;

	if (!g_slist_find(twitter_connections, ic))
		return FALSE;

	td = ic->proto_data;
	if (info->stage == OAUTH_REQUEST_TOKEN) {
		char name[strlen(ic->acc->user) + 9], *msg;

		if (info->request_token == NULL) {
			imcb_error(ic, "OAuth error: %s", twitter_parse_error(info->http));
			imc_logout(ic, TRUE);
			return FALSE;
		}

		sprintf(name, "%s_%s", td->prefix, ic->acc->user);
		msg = g_strdup_printf("To finish OAuth authentication, please visit "
				      "%s and respond with the resulting PIN code.",
				      info->auth_url);
		imcb_buddy_msg(ic, name, msg, 0, 0);
		g_free(msg);
	} else if (info->stage == OAUTH_ACCESS_TOKEN) {
		if (info->token == NULL || info->token_secret == NULL) {
			imcb_error(ic, "OAuth error: %s", twitter_parse_error(info->http));
			imc_logout(ic, TRUE);
			return FALSE;
		} else {
			const char *sn = oauth_params_get(&info->params, "screen_name");

			if (sn != NULL && ic->acc->prpl->handle_cmp(sn, ic->acc->user) != 0) {
				imcb_log(ic, "Warning: You logged in via OAuth as %s "
					 "instead of %s.", sn, ic->acc->user);
			}
			g_free(td->user);
			td->user = g_strdup(sn);
		}

		/* IM mods didn't do this so far and it's ugly but I should
		   be able to get away with it... */
		g_free(ic->acc->pass);
		ic->acc->pass = oauth_to_string(info);

		twitter_login_finish(ic);
	}

	return TRUE;
}


static char *set_eval_mode(set_t * set, char *value)
{
	if (g_strcasecmp(value, "one") == 0 ||
	    g_strcasecmp(value, "many") == 0 || g_strcasecmp(value, "chat") == 0)
		return value;
	else
		return NULL;
}

static gboolean twitter_length_check(struct im_connection *ic, gchar * msg)
{
	int max = set_getint(&ic->acc->set, "message_length"), len;

	if (max == 0 || (len = g_utf8_strlen(msg, -1)) <= max)
		return TRUE;

	imcb_error(ic, "Maximum message length exceeded: %d > %d", len, max);

	return FALSE;
}

static void twitter_init(account_t * acc)
{
	set_t *s;
	char *def_url;
	char *def_oauth;

	if (strcmp(acc->prpl->name, "twitter") == 0) {
		def_url = TWITTER_API_URL;
		def_oauth = "true";
	} else {		/* if( strcmp( acc->prpl->name, "identica" ) == 0 ) */
		def_url = IDENTICA_API_URL;
		def_oauth = "true";
	}

	s = set_add(&acc->set, "auto_reply_timeout", "10800", set_eval_int, acc);

	s = set_add(&acc->set, "base_url", def_url, NULL, acc);
	s->flags |= ACC_SET_OFFLINE_ONLY;

	s = set_add(&acc->set, "commands", "true", set_eval_bool, acc);

	s = set_add(&acc->set, "fetch_interval", "60", set_eval_int, acc);
	s->flags |= ACC_SET_OFFLINE_ONLY;

	s = set_add(&acc->set, "fetch_mentions", "true", set_eval_bool, acc);

	s = set_add(&acc->set, "message_length", "140", set_eval_int, acc);

	s = set_add(&acc->set, "mode", "chat", set_eval_mode, acc);
	s->flags |= ACC_SET_OFFLINE_ONLY;

	s = set_add(&acc->set, "oauth", def_oauth, set_eval_oauth, acc);

	s = set_add(&acc->set, "show_ids", "false", set_eval_bool, acc);
	s->flags |= ACC_SET_OFFLINE_ONLY;

	s = set_add(&acc->set, "show_old_mentions", "true", set_eval_bool, acc);
}

/**
 * Login method. Since the twitter API works with seperate HTTP request we 
 * only save the user and pass to the twitter_data object.
 */
static void twitter_login(account_t * acc)
{
	struct im_connection *ic = imcb_new(acc);
	struct twitter_data *td;
	char name[strlen(acc->user) + 9];
	url_t url;
	char *s;
	
	if (!url_set(&url, set_getstr(&ic->acc->set, "base_url")) ||
	    (url.proto != PROTO_HTTP && url.proto != PROTO_HTTPS)) {
		imcb_error(ic, "Incorrect API base URL: %s", set_getstr(&ic->acc->set, "base_url"));
		imc_logout(ic, FALSE);
		return;
	}

	imcb_log(ic, "Connecting");

	twitter_connections = g_slist_append(twitter_connections, ic);
	td = g_new0(struct twitter_data, 1);
	ic->proto_data = td;
	td->user = g_strdup(acc->user);

	td->url_ssl = url.proto == PROTO_HTTPS;
	td->url_port = url.port;
	td->url_host = g_strdup(url.host);
	if (strcmp(url.file, "/") != 0)
		td->url_path = g_strdup(url.file);
	else {
		td->url_path = g_strdup("");
		if (g_str_has_suffix(url.host, "twitter.com"))
			/* May fire for people who turned on HTTPS. */
			imcb_error(ic, "Warning: Twitter requires a version number in API calls "
			               "now. Try resetting the base_url account setting.");
	}
	
	/* Hacky string mangling: Turn identi.ca into identi.ca and api.twitter.com
	   into twitter, and try to be sensible if we get anything else. */
	td->prefix = g_strdup(url.host);
	if (g_str_has_suffix(td->prefix, ".com"))
		td->prefix[strlen(url.host) - 4] = '\0';
	if ((s = strrchr(td->prefix, '.')) && strlen(s) > 4) {
		/* If we have at least 3 chars after the last dot, cut off the rest.
		   (mostly a www/api prefix or sth) */
		s = g_strdup(s + 1);
		g_free(td->prefix);
		td->prefix = s;
	}
	
	if (strstr(acc->pass, "oauth_token="))
		td->oauth_info = oauth_from_string(acc->pass, get_oauth_service(ic));

	sprintf(name, "%s_%s", td->prefix, acc->user);
	imcb_add_buddy(ic, name, NULL);
	imcb_buddy_status(ic, name, OPT_LOGGED_IN, NULL, NULL);

	if (set_getbool(&acc->set, "show_ids"))
		td->log = g_new0(struct twitter_log_data, TWITTER_LOG_LENGTH);

	twitter_login_finish(ic);
}

/**
 * Logout method. Just free the twitter_data.
 */
static void twitter_logout(struct im_connection *ic)
{
	struct twitter_data *td = ic->proto_data;

	// Set the status to logged out.
	ic->flags &= ~OPT_LOGGED_IN;

	// Remove the main_loop function from the function queue.
	b_event_remove(td->main_loop_id);

	if (td->timeline_gc)
		imcb_chat_free(td->timeline_gc);

	if (td) {
		oauth_info_free(td->oauth_info);
		g_free(td->user);
		g_free(td->prefix);
		g_free(td->url_host);
		g_free(td->url_path);
		g_free(td->log);
		g_free(td);
	}

	twitter_connections = g_slist_remove(twitter_connections, ic);
}

static void twitter_handle_command(struct im_connection *ic, char *message);

/**
 *
 */
static int twitter_buddy_msg(struct im_connection *ic, char *who, char *message, int away)
{
	struct twitter_data *td = ic->proto_data;
	int plen = strlen(td->prefix);

	if (g_strncasecmp(who, td->prefix, plen) == 0 && who[plen] == '_' &&
	    g_strcasecmp(who + plen + 1, ic->acc->user) == 0) {
		if (set_getbool(&ic->acc->set, "oauth") &&
		    td->oauth_info && td->oauth_info->token == NULL) {
			char pin[strlen(message) + 1], *s;

			strcpy(pin, message);
			for (s = pin + sizeof(pin) - 2; s > pin && isspace(*s); s--)
				*s = '\0';
			for (s = pin; *s && isspace(*s); s++) {
			}

			if (!oauth_access_token(s, td->oauth_info)) {
				imcb_error(ic, "OAuth error: %s",
					   "Failed to send access token request");
				imc_logout(ic, TRUE);
				return FALSE;
			}
		} else
			twitter_handle_command(ic, message);
	} else {
		twitter_direct_messages_new(ic, who, message);
	}
	return (0);
}

/**
 *
 */
static void twitter_set_my_name(struct im_connection *ic, char *info)
{
}

static void twitter_get_info(struct im_connection *ic, char *who)
{
}

static void twitter_add_buddy(struct im_connection *ic, char *who, char *group)
{
	twitter_friendships_create_destroy(ic, who, 1);
}

static void twitter_remove_buddy(struct im_connection *ic, char *who, char *group)
{
	twitter_friendships_create_destroy(ic, who, 0);
}

static void twitter_chat_msg(struct groupchat *c, char *message, int flags)
{
	if (c && message)
		twitter_handle_command(c->ic, message);
}

static void twitter_chat_invite(struct groupchat *c, char *who, char *message)
{
}

static void twitter_chat_leave(struct groupchat *c)
{
	struct twitter_data *td = c->ic->proto_data;

	if (c != td->timeline_gc)
		return;		/* WTF? */

	/* If the user leaves the channel: Fine. Rejoin him/her once new
	   tweets come in. */
	imcb_chat_free(td->timeline_gc);
	td->timeline_gc = NULL;
}

static void twitter_keepalive(struct im_connection *ic)
{
}

static void twitter_add_permit(struct im_connection *ic, char *who)
{
}

static void twitter_rem_permit(struct im_connection *ic, char *who)
{
}

static void twitter_add_deny(struct im_connection *ic, char *who)
{
}

static void twitter_rem_deny(struct im_connection *ic, char *who)
{
}

//static char *twitter_set_display_name( set_t *set, char *value )
//{
//      return value;
//}

static void twitter_buddy_data_add(struct bee_user *bu)
{
	bu->data = g_new0(struct twitter_user_data, 1);
}

static void twitter_buddy_data_free(struct bee_user *bu)
{
	g_free(bu->data);
}

static void twitter_handle_command(struct im_connection *ic, char *message)
{
	struct twitter_data *td = ic->proto_data;
	char *cmds, **cmd, *new = NULL;
	guint64 in_reply_to = 0;

	cmds = g_strdup(message);
	cmd = split_command_parts(cmds);

	if (cmd[0] == NULL) {
		g_free(cmds);
		return;
	} else if (!set_getbool(&ic->acc->set, "commands")) {
		/* Not supporting commands. */
	} else if (g_strcasecmp(cmd[0], "undo") == 0) {
		guint64 id;

		if (cmd[1] == NULL)
			twitter_status_destroy(ic, td->last_status_id);
		else if (sscanf(cmd[1], "%" G_GUINT64_FORMAT, &id) == 1) {
			if (id < TWITTER_LOG_LENGTH && td->log)
				id = td->log[id].id;
			
			twitter_status_destroy(ic, id);
		} else
			twitter_msg(ic, "Could not undo last action");

		g_free(cmds);
		return;
	} else if (g_strcasecmp(cmd[0], "follow") == 0 && cmd[1]) {
		twitter_add_buddy(ic, cmd[1], NULL);
		g_free(cmds);
		return;
	} else if (g_strcasecmp(cmd[0], "unfollow") == 0 && cmd[1]) {
		twitter_remove_buddy(ic, cmd[1], NULL);
		g_free(cmds);
		return;
	} else if (g_strcasecmp(cmd[0], "rt") == 0 && cmd[1]) {
		struct twitter_user_data *tud;
		bee_user_t *bu;
		guint64 id;

		if (g_str_has_prefix(cmd[1], "#") &&
		    sscanf(cmd[1] + 1, "%" G_GUINT64_FORMAT, &id) == 1) {
			if (id < TWITTER_LOG_LENGTH && td->log)
				id = td->log[id].id;
		} else if ((bu = bee_user_by_handle(ic->bee, ic, cmd[1])) &&
		    (tud = bu->data) && tud->last_id)
			id = tud->last_id;
		else if (sscanf(cmd[1], "%" G_GUINT64_FORMAT, &id) == 1){
			if (id < TWITTER_LOG_LENGTH && td->log)
				id = td->log[id].id;
		}

		td->last_status_id = 0;
		if (id)
			twitter_status_retweet(ic, id);
		else
			twitter_msg(ic, "User `%s' does not exist or didn't "
				    "post any statuses recently", cmd[1]);

		g_free(cmds);
		return;
	} else if (g_strcasecmp(cmd[0], "reply") == 0 && cmd[1] && cmd[2]) {
		struct twitter_user_data *tud;
		bee_user_t *bu = NULL;
		guint64 id = 0;

		if (g_str_has_prefix(cmd[1], "#") &&
		    sscanf(cmd[1] + 1, "%" G_GUINT64_FORMAT, &id) == 1 &&
		    (id < TWITTER_LOG_LENGTH) && td->log) {
			bu = td->log[id].bu;
			if (g_slist_find(ic->bee->users, bu))
				id = td->log[id].id;
			else
				bu = NULL;
		} else if ((bu = bee_user_by_handle(ic->bee, ic, cmd[1])) &&
		    (tud = bu->data) && tud->last_id) {
			id = tud->last_id;
		} else if (sscanf(cmd[1], "%" G_GUINT64_FORMAT, &id) == 1 &&
		           (id < TWITTER_LOG_LENGTH) && td->log) {
			bu = td->log[id].bu;
			if (g_slist_find(ic->bee->users, bu))
				id = td->log[id].id;
			else
				bu = NULL;
		}

		if (!id || !bu) {
			twitter_msg(ic, "User `%s' does not exist or didn't "
				    "post any statuses recently", cmd[1]);
			return;
		}
		message = new = g_strdup_printf("@%s %s", bu->handle, message + (cmd[2] - cmd[0]));
		in_reply_to = id;
	} else if (g_strcasecmp(cmd[0], "post") == 0) {
		message += 5;
	}

	{
		char *s;
		bee_user_t *bu;

		if (!twitter_length_check(ic, message)) {
			g_free(new);
			g_free(cmds);
			return;
		}

		s = cmd[0] + strlen(cmd[0]) - 1;
		if (!new && s > cmd[0] && (*s == ':' || *s == ',')) {
			*s = '\0';

			if ((bu = bee_user_by_handle(ic->bee, ic, cmd[0]))) {
				struct twitter_user_data *tud = bu->data;

				new = g_strdup_printf("@%s %s", bu->handle,
						      message + (s - cmd[0]) + 2);
				message = new;

				if (time(NULL) < tud->last_time +
				    set_getint(&ic->acc->set, "auto_reply_timeout"))
					in_reply_to = tud->last_id;
			}
		}

		/* If the user runs undo between this request and its response
		   this would delete the second-last Tweet. Prevent that. */
		td->last_status_id = 0;
		twitter_post_status(ic, message, in_reply_to);
		g_free(new);
	}
	g_free(cmds);
}

void twitter_initmodule()
{
	struct prpl *ret = g_new0(struct prpl, 1);

	ret->options = OPT_NOOTR;
	ret->name = "twitter";
	ret->login = twitter_login;
	ret->init = twitter_init;
	ret->logout = twitter_logout;
	ret->buddy_msg = twitter_buddy_msg;
	ret->get_info = twitter_get_info;
	ret->set_my_name = twitter_set_my_name;
	ret->add_buddy = twitter_add_buddy;
	ret->remove_buddy = twitter_remove_buddy;
	ret->chat_msg = twitter_chat_msg;
	ret->chat_invite = twitter_chat_invite;
	ret->chat_leave = twitter_chat_leave;
	ret->keepalive = twitter_keepalive;
	ret->add_permit = twitter_add_permit;
	ret->rem_permit = twitter_rem_permit;
	ret->add_deny = twitter_add_deny;
	ret->rem_deny = twitter_rem_deny;
	ret->buddy_data_add = twitter_buddy_data_add;
	ret->buddy_data_free = twitter_buddy_data_free;
	ret->handle_cmp = g_strcasecmp;

	register_protocol(ret);

	/* And an identi.ca variant: */
	ret = g_memdup(ret, sizeof(struct prpl));
	ret->name = "identica";
	register_protocol(ret);
}
