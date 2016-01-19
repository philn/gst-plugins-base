/* GStreamer HTTP library
 *
 * Copyright (C) 2007 Red Hat, Inc.
 * Copyright (C) 2016 Samsung Electronics. All rights reserved.
 *   Author: Thiago Santos <thiagoss@osg.samsung.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
/*
 * This code was copied from libsoup at commit
 * 45cf9db7d46ff6ecabd6bbd4e7ae99cfefbc1626
 * on 20/01/2016.
 *
 * libsoup is LGPL v2.
 */

#ifndef GST_HTTP_COOKIE_H
#define GST_HTTP_COOKIE_H 1

#include <gst/gst.h>

G_BEGIN_DECLS

struct _GstHttpCookie {
	char     *name;
	char     *value;
	char     *domain;
	char     *path;
	GDateTime *expires;
	gboolean  secure;
	gboolean  http_only;
};

typedef struct _GstHttpCookie GstHttpCookie;

GType gst_http_cookie_get_type (void);
#define GST_TYPE_HTTP_COOKIE (gst_http_cookie_get_type())

GstHttpCookie *gst_http_cookie_new              (const char  *name,
						 const char  *value,
						 const char  *domain,
						 const char  *path,
						 int          max_age);

GstHttpCookie *gst_http_cookie_copy             (GstHttpCookie  *cookie);


const char *gst_http_cookie_get_name            (GstHttpCookie  *cookie);

void        gst_http_cookie_set_name            (GstHttpCookie  *cookie,
						 const char  *name);

const char *gst_http_cookie_get_value           (GstHttpCookie  *cookie);

void        gst_http_cookie_set_value           (GstHttpCookie  *cookie,
						 const char  *value);

const char *gst_http_cookie_get_domain              (GstHttpCookie  *cookie);

void        gst_http_cookie_set_domain              (GstHttpCookie  *cookie,
						 const char  *domain);

const char *gst_http_cookie_get_path                (GstHttpCookie  *cookie);

void        gst_http_cookie_set_path                (GstHttpCookie  *cookie,
						 const char  *path);

void        gst_http_cookie_set_max_age             (GstHttpCookie  *cookie,
						 int          max_age);

GDateTime   *gst_http_cookie_get_expires             (GstHttpCookie  *cookie);

void        gst_http_cookie_set_expires             (GstHttpCookie  *cookie,
						 GDateTime    *expires);

gboolean    gst_http_cookie_get_secure              (GstHttpCookie  *cookie);

void        gst_http_cookie_set_secure              (GstHttpCookie  *cookie,
						 gboolean     secure);

gboolean    gst_http_cookie_get_http_only           (GstHttpCookie  *cookie);

void        gst_http_cookie_set_http_only           (GstHttpCookie  *cookie,
						 gboolean     http_only);


gboolean    gst_http_cookie_equal                   (GstHttpCookie  *cookie1,
						 GstHttpCookie  *cookie2);


void        gst_http_cookie_free                    (GstHttpCookie  *cookie);


void        gst_http_cookies_free                   (GSList      *cookies);

G_END_DECLS

#endif /* GST_HTTP_COOKIE_H */
