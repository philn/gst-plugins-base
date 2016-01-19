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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "gsthttpcookie.h"

/**
 * SECTION:gst-http-cookie
 * @short_description: HTTP Cookies
 * @see_also: #GstHttpCookieJar
 *
 * #GstHttpCookie implements HTTP cookies, as described by <ulink
 * url="http://tools.ietf.org/html/rfc6265.txt">RFC 6265</ulink>.
 **/

/**
 * GstHttpCookie:
 * @name: the cookie name
 * @value: the cookie value
 * @domain: the "domain" attribute, or else the hostname that the
 * cookie came from.
 * @path: the "path" attribute, or %NULL
 * @expires: the cookie expiration time, or %NULL for a session cookie
 * @secure: %TRUE if the cookie should only be tranferred over SSL
 * @http_only: %TRUE if the cookie should not be exposed to scripts
 *
 * An HTTP cookie.
 *
 * @name and @value will be set for all cookies. If the cookie is
 * generated from a string that appears to have no name, then @name
 * will be the empty string.
 *
 * @domain and @path give the host or domain, and path within that
 * host/domain, to restrict this cookie to. If @domain starts with
 * ".", that indicates a domain (which matches the string after the
 * ".", or any hostname that has @domain as a suffix). Otherwise, it
 * is a hostname and must match exactly.
 *
 * @expires will be non-%NULL if the cookie uses either the original
 * "expires" attribute, or the newer "max-age" attribute. If @expires
 * is %NULL, it indicates that neither "expires" nor "max-age" was
 * specified, and the cookie expires at the end of the session.
 *
 * If @http_only is set, the cookie should not be exposed to untrusted
 * code (eg, javascript), so as to minimize the danger posed by
 * cross-site scripting attacks.
 *
 * Since: 1.10
 **/

G_DEFINE_BOXED_TYPE (GstHttpCookie, gst_http_cookie, gst_http_cookie_copy,
    gst_http_cookie_free);

/**
 * gst_http_cookie_copy:
 * @cookie: a #GstHttpCookie
 *
 * Copies @cookie.
 *
 * Return value: a copy of @cookie
 *
 * Since: 1.10
 **/
GstHttpCookie *
gst_http_cookie_copy (GstHttpCookie * cookie)
{
  GstHttpCookie *copy = g_slice_new0 (GstHttpCookie);

  copy->name = g_strdup (cookie->name);
  copy->value = g_strdup (cookie->value);
  copy->domain = g_strdup (cookie->domain);
  copy->path = g_strdup (cookie->path);
  if (cookie->expires)
    copy->expires = g_date_time_ref (cookie->expires);
  copy->secure = cookie->secure;
  copy->http_only = cookie->http_only;

  return copy;
}

static GstHttpCookie *
cookie_new_internal (const char *name, const char *value,
    const char *domain, const char *path, int max_age)
{
  GstHttpCookie *cookie;

  cookie = g_slice_new0 (GstHttpCookie);
  cookie->name = g_strdup (name);
  cookie->value = g_strdup (value);
  cookie->domain = g_strdup (domain);
  cookie->path = g_strdup (path);
  gst_http_cookie_set_max_age (cookie, max_age);

  return cookie;
}

/**
 * gst_http_cookie_new:
 * @name: cookie name
 * @value: cookie value
 * @domain: cookie domain or hostname
 * @path: cookie path, or %NULL
 * @max_age: max age of the cookie, or -1 for a session cookie
 *
 * Creates a new #GstHttpCookie with the given attributes. (Use
 * gst_http_cookie_set_secure() and gst_http_cookie_set_http_only() if you
 * need to set those attributes on the returned cookie.)
 *
 * If @domain starts with ".", that indicates a domain (which matches
 * the string after the ".", or any hostname that has @domain as a
 * suffix). Otherwise, it is a hostname and must match exactly.
 *
 * @max_age is used to set the "expires" attribute on the cookie; pass
 * -1 to not include the attribute (indicating that the cookie expires
 * with the current session), 0 for an already-expired cookie, or a
 * lifetime in seconds.
 *
 * Return value: a new #GstHttpCookie.
 *
 * Since: 1.10
 **/
GstHttpCookie *
gst_http_cookie_new (const char *name, const char *value,
    const char *domain, const char *path, int max_age)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (value != NULL, NULL);

  /* We ought to return if domain is NULL too, but this used to
   * do be incorrectly documented as legal, and it wouldn't
   * break anything as long as you called
   * gst_http_cookie_set_domain() immediately after. So we warn but
   * don't return, to discourage that behavior but not actually
   * break anyone doing it.
   */
  g_warn_if_fail (domain != NULL);

  return cookie_new_internal (name, value, domain, path, max_age);
}

/**
 * gst_http_cookie_get_name:
 * @cookie: a #GstHttpCookie
 *
 * Gets @cookie's name
 *
 * Return value: @cookie's name
 *
 * Since: 1.10
 **/
const char *
gst_http_cookie_get_name (GstHttpCookie * cookie)
{
  return cookie->name;
}

/**
 * gst_http_cookie_set_name:
 * @cookie: a #GstHttpCookie
 * @name: the new name
 *
 * Sets @cookie's name to @name
 *
 * Since: 1.10
 **/
void
gst_http_cookie_set_name (GstHttpCookie * cookie, const char *name)
{
  g_free (cookie->name);
  cookie->name = g_strdup (name);
}

/**
 * gst_http_cookie_get_value:
 * @cookie: a #GstHttpCookie
 *
 * Gets @cookie's value
 *
 * Return value: @cookie's value
 *
 * Since: 1.10
 **/
const char *
gst_http_cookie_get_value (GstHttpCookie * cookie)
{
  return cookie->value;
}

/**
 * gst_http_cookie_set_value:
 * @cookie: a #GstHttpCookie
 * @value: the new value
 *
 * Sets @cookie's value to @value
 *
 * Since: 1.10
 **/
void
gst_http_cookie_set_value (GstHttpCookie * cookie, const char *value)
{
  g_free (cookie->value);
  cookie->value = g_strdup (value);
}

/**
 * gst_http_cookie_get_domain:
 * @cookie: a #GstHttpCookie
 *
 * Gets @cookie's domain
 *
 * Return value: @cookie's domain
 *
 * Since: 1.10
 **/
const char *
gst_http_cookie_get_domain (GstHttpCookie * cookie)
{
  return cookie->domain;
}

/**
 * gst_http_cookie_set_domain:
 * @cookie: a #GstHttpCookie
 * @domain: the new domain
 *
 * Sets @cookie's domain to @domain
 *
 * Since: 1.10
 **/
void
gst_http_cookie_set_domain (GstHttpCookie * cookie, const char *domain)
{
  g_free (cookie->domain);
  cookie->domain = g_strdup (domain);
}

/**
 * gst_http_cookie_get_path:
 * @cookie: a #GstHttpCookie
 *
 * Gets @cookie's path
 *
 * Return value: @cookie's path
 *
 * Since: 1.10
 **/
const char *
gst_http_cookie_get_path (GstHttpCookie * cookie)
{
  return cookie->path;
}

/**
 * gst_http_cookie_set_path:
 * @cookie: a #GstHttpCookie
 * @path: the new path
 *
 * Sets @cookie's path to @path
 *
 * Since: 1.10
 **/
void
gst_http_cookie_set_path (GstHttpCookie * cookie, const char *path)
{
  g_free (cookie->path);
  cookie->path = g_strdup (path);
}

/**
 * gst_http_cookie_set_max_age:
 * @cookie: a #GstHttpCookie
 * @max_age: the new max age
 *
 * Sets @cookie's max age to @max_age. If @max_age is -1, the cookie
 * is a session cookie, and will expire at the end of the client's
 * session. Otherwise, it is the number of seconds until the cookie
 * expires. (A value of 0 indicates that the cookie should be
 * considered already-expired.)
 *
 * (This sets the same property as gst_http_cookie_set_expires().)
 *
 * Since: 1.10
 **/
void
gst_http_cookie_set_max_age (GstHttpCookie * cookie, int max_age)
{
  if (cookie->expires)
    g_date_time_unref (cookie->expires);

  if (max_age == -1)
    cookie->expires = NULL;
  else if (max_age == 0) {
    /* Use a date way in the past, to protect against
     * clock skew.
     */
    cookie->expires = g_date_time_new (NULL, 1970, 1, 1, 0, 0, 0);
  } else {
    GDateTime *now = g_date_time_new_now_local ();
    cookie->expires = g_date_time_add_seconds (now, max_age);
    g_date_time_unref (now);
  }
}

/**
 * gst_http_cookie_get_expires:
 * @cookie: a #GstHttpCookie
 *
 * Gets @cookie's expiration time
 *
 * Return value: (transfer none): @cookie's expiration time, which is
 * owned by @cookie and should not be modified or freed.
 *
 * Since: 1.10
 **/
GDateTime *
gst_http_cookie_get_expires (GstHttpCookie * cookie)
{
  return cookie->expires;
}

/**
 * gst_http_cookie_set_expires:
 * @cookie: a #GstHttpCookie
 * @expires: the new expiration time, or %NULL
 *
 * Sets @cookie's expiration time to @expires. If @expires is %NULL,
 * @cookie will be a session cookie and will expire at the end of the
 * client's session.
 *
 * (This sets the same property as gst_http_cookie_set_max_age().)
 *
 * Since: 1.10
 **/
void
gst_http_cookie_set_expires (GstHttpCookie * cookie, GDateTime * expires)
{
  if (cookie->expires)
    g_date_time_unref (cookie->expires);

  if (expires)
    cookie->expires = g_date_time_ref (expires);
  else
    cookie->expires = NULL;
}

/**
 * gst_http_cookie_get_secure:
 * @cookie: a #GstHttpCookie
 *
 * Gets @cookie's secure attribute
 *
 * Return value: @cookie's secure attribute
 *
 * Since: 1.10
 **/
gboolean
gst_http_cookie_get_secure (GstHttpCookie * cookie)
{
  return cookie->secure;
}

/**
 * gst_http_cookie_set_secure:
 * @cookie: a #GstHttpCookie
 * @secure: the new value for the secure attribute
 *
 * Sets @cookie's secure attribute to @secure. If %TRUE, @cookie will
 * only be transmitted from the client to the server over secure
 * (https) connections.
 *
 * Since: 1.10
 **/
void
gst_http_cookie_set_secure (GstHttpCookie * cookie, gboolean secure)
{
  cookie->secure = secure;
}

/**
 * gst_http_cookie_get_http_only:
 * @cookie: a #GstHttpCookie
 *
 * Gets @cookie's HttpOnly attribute
 *
 * Return value: @cookie's HttpOnly attribute
 *
 * Since: 1.10
 **/
gboolean
gst_http_cookie_get_http_only (GstHttpCookie * cookie)
{
  return cookie->http_only;
}

/**
 * gst_http_cookie_set_http_only:
 * @cookie: a #GstHttpCookie
 * @http_only: the new value for the HttpOnly attribute
 *
 * Sets @cookie's HttpOnly attribute to @http_only. If %TRUE, @cookie
 * will be marked as "http only", meaning it should not be exposed to
 * web page scripts or other untrusted code.
 *
 * Since: 1.10
 **/
void
gst_http_cookie_set_http_only (GstHttpCookie * cookie, gboolean http_only)
{
  cookie->http_only = http_only;
}

/**
 * gst_http_cookie_free:
 * @cookie: a #GstHttpCookie
 *
 * Frees @cookie
 *
 * Since: 1.10
 **/
void
gst_http_cookie_free (GstHttpCookie * cookie)
{
  g_return_if_fail (cookie != NULL);

  g_free (cookie->name);
  g_free (cookie->value);
  g_free (cookie->domain);
  g_free (cookie->path);
  if (cookie->expires)
    g_date_time_unref (cookie->expires);

  g_slice_free (GstHttpCookie, cookie);
}

/**
 * gst_http_cookies_free: (skip)
 * @cookies: (element-type GstHttpCookie): a #GSList of #GstHttpCookie
 *
 * Frees @cookies.
 *
 * Since: 1.10
 **/
void
gst_http_cookies_free (GSList * cookies)
{
  g_slist_free_full (cookies, (GDestroyNotify) gst_http_cookie_free);
}

/**
 * gst_http_cookie_equal:
 * @cookie1: a #GstHttpCookie
 * @cookie2: a #GstHttpCookie
 *
 * Tests if @cookie1 and @cookie2 are equal.
 *
 * Note that currently, this does not check that the cookie domains
 * match. This may change in the future.
 *
 * Return value: whether the cookies are equal.
 *
 * Since: 1.10
 */
gboolean
gst_http_cookie_equal (GstHttpCookie * cookie1, GstHttpCookie * cookie2)
{
  g_return_val_if_fail (cookie1, FALSE);
  g_return_val_if_fail (cookie2, FALSE);

  return (!strcmp (cookie1->name, cookie2->name) &&
      !strcmp (cookie1->value, cookie2->value) &&
      !strcmp (cookie1->path, cookie2->path));
}
