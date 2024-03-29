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


#ifndef _TWITTER_LIB_H
#define _TWITTER_LIB_H

#include "nogaim.h"
#include "twitter_http.h"

#define TWITTER_API_URL "http://api.twitter.com/1"
#define IDENTICA_API_URL "https://identi.ca/api"

/* Status URLs */
#define TWITTER_STATUS_UPDATE_URL "/statuses/update.xml"
#define TWITTER_STATUS_SHOW_URL "/statuses/show/"
#define TWITTER_STATUS_DESTROY_URL "/statuses/destroy/"
#define TWITTER_STATUS_RETWEET_URL "/statuses/retweet/"

/* Timeline URLs */
#define TWITTER_PUBLIC_TIMELINE_URL "/statuses/public_timeline.xml"
#define TWITTER_FEATURED_USERS_URL "/statuses/featured.xml"
#define TWITTER_FRIENDS_TIMELINE_URL "/statuses/friends_timeline.xml"
#define TWITTER_HOME_TIMELINE_URL "/statuses/home_timeline.xml"
#define TWITTER_MENTIONS_URL "/statuses/mentions.xml"
#define TWITTER_USER_TIMELINE_URL "/statuses/user_timeline.xml"

/* Users URLs */
#define TWITTER_USERS_LOOKUP_URL "/users/lookup.xml"

/* Direct messages URLs */
#define TWITTER_DIRECT_MESSAGES_URL "/direct_messages.xml"
#define TWITTER_DIRECT_MESSAGES_NEW_URL "/direct_messages/new.xml"
#define TWITTER_DIRECT_MESSAGES_SENT_URL "/direct_messages/sent.xml"
#define TWITTER_DIRECT_MESSAGES_DESTROY_URL "/direct_messages/destroy/"

/* Friendships URLs */
#define TWITTER_FRIENDSHIPS_CREATE_URL "/friendships/create.xml"
#define TWITTER_FRIENDSHIPS_DESTROY_URL "/friendships/destroy.xml"
#define TWITTER_FRIENDSHIPS_SHOW_URL "/friendships/show.xml"

/* Social graphs URLs */
#define TWITTER_FRIENDS_IDS_URL "/friends/ids.xml"
#define TWITTER_FOLLOWERS_IDS_URL "/followers/ids.xml"

/* Account URLs */
#define TWITTER_ACCOUNT_RATE_LIMIT_URL "/account/rate_limit_status.xml"

/* Favorites URLs */
#define TWITTER_FAVORITES_GET_URL "/favorites.xml"
#define TWITTER_FAVORITE_CREATE_URL "/favorites/create/"
#define TWITTER_FAVORITE_DESTROY_URL "/favorites/destroy/"

/* Block URLs */
#define TWITTER_BLOCKS_CREATE_URL "/blocks/create/"
#define TWITTER_BLOCKS_DESTROY_URL "/blocks/destroy/"

void twitter_get_timeline(struct im_connection *ic, gint64 next_cursor);
void twitter_get_friends_ids(struct im_connection *ic, gint64 next_cursor);
void twitter_get_home_timeline(struct im_connection *ic, gint64 next_cursor);
void twitter_get_mentions(struct im_connection *ic, gint64 next_cursor);
void twitter_get_statuses_friends(struct im_connection *ic, gint64 next_cursor);

void twitter_post_status(struct im_connection *ic, char *msg, guint64 in_reply_to);
void twitter_direct_messages_new(struct im_connection *ic, char *who, char *message);
void twitter_friendships_create_destroy(struct im_connection *ic, char *who, int create);
void twitter_status_destroy(struct im_connection *ic, guint64 id);
void twitter_status_retweet(struct im_connection *ic, guint64 id);

#endif //_TWITTER_LIB_H

