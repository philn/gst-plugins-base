/* GStreamer HTTP library
 *
 * Copyright (C) 2008 Red Hat, Inc.
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

#ifndef GST_HTTP_COOKIE_JAR_H
#define GST_HTTP_COOKIE_JAR_H 1

#include <gst/gst.h>
#include "gsthttpcookie.h"

G_BEGIN_DECLS

#define GST_TYPE_HTTP_COOKIE_JAR            (gst_http_cookie_jar_get_type ())
#define GST_HTTP_COOKIE_JAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_HTTP_COOKIE_JAR, GstHttpCookieJar))
#define GST_HTTP_COOKIE_JAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_HTTP_COOKIE_JAR, GstHttpCookieJarClass))
#define GST_HTTP_IS_COOKIE_JAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_HTTP_COOKIE_JAR))
#define GST_HTTP_IS_COOKIE_JAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), GST_TYPE_HTTP_COOKIE_JAR))
#define GST_HTTP_COOKIE_JAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_HTTP_COOKIE_JAR, GstHttpCookieJarClass))
#define GST_HTTP_COOKIE_JAR_CAST(obj)       ((GstHttpCookieJar *) (obj))

typedef struct _GstHttpCookieJar {
  GstObject parent;

  /*< private >*/
  void             *padding[GST_PADDING_LARGE];
} GstHttpCookieJar;

typedef struct {
  GstObjectClass parent_class;

  /* signals */
  void (*changed) (GstHttpCookieJar *jar,
                   gpointer author,
                   GstHttpCookie    *old_cookie,
                   GstHttpCookie    *new_cookie);

  /*< private >*/
  void             *padding[GST_PADDING_LARGE];
} GstHttpCookieJarClass;

GType                     gst_http_cookie_jar_get_type                (void);

GstHttpCookieJar *        gst_http_cookie_jar_new                     (void);

void                      gst_http_cookie_jar_add_cookie              (GstHttpCookieJar             *jar,
                                                                       gpointer author,
								       GstHttpCookie                *cookie);

void                      gst_http_cookie_jar_delete_cookie           (GstHttpCookieJar             *jar,
                                                                       gpointer author,
								       GstHttpCookie                *cookie);

GSList *                  gst_http_cookie_jar_all_cookies             (GstHttpCookieJar             *jar);

G_END_DECLS

#endif /* GST_HTTP_COOKIE_JAR_H */
