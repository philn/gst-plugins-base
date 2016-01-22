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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gst/gst.h>

#include "gsthttpcookiejar.h"

/**
 * SECTION:gst-http-cookie-jar
 * @short_description: Automatic cookie sharing
 *
 * A #GstHttpCookieJar stores #GstHttpCookie<!-- -->s.
 *
 * Note that the base #GstHttpCookieJar class does not support any form
 * of long-term cookie persistence.
 **/

G_DEFINE_TYPE (GstHttpCookieJar, gst_http_cookie_jar, GST_TYPE_OBJECT);

enum
{
  CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct
{
  gboolean constructed;
  GHashTable *domains, *serials;
  guint serial;

  /* no cookies are added/removed when an operation
   * is ongoing. The mutex and this boolean are to
   * ensure this */
  gboolean ongoing_operation;

  GRecMutex mutex;
} GstHttpCookieJarPrivate;
#define GST_HTTP_COOKIE_JAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_HTTP_COOKIE_JAR, GstHttpCookieJarPrivate))

#define GST_HTTP_COOKIE_JAR_GET_MUTEX(j) &(GST_HTTP_COOKIE_JAR_GET_PRIVATE (GST_HTTP_COOKIE_JAR_CAST (j))->mutex)
#define GST_HTTP_COOKIE_JAR_LOCK(j) g_rec_mutex_lock (GST_HTTP_COOKIE_JAR_GET_MUTEX(j))
#define GST_HTTP_COOKIE_JAR_UNLOCK(j) g_rec_mutex_unlock (GST_HTTP_COOKIE_JAR_GET_MUTEX(j))

static gboolean
gst_http_cookie_jar_check_start_operation (GstHttpCookieJar * jar)
{
  GstHttpCookieJarPrivate *priv = GST_HTTP_COOKIE_JAR_GET_PRIVATE (jar);
  if (priv->ongoing_operation)
    return FALSE;
  priv->ongoing_operation = TRUE;
  return TRUE;
}

static void
gst_http_cookie_jar_finish_operation (GstHttpCookieJar * jar)
{
  GstHttpCookieJarPrivate *priv = GST_HTTP_COOKIE_JAR_GET_PRIVATE (jar);
  g_return_if_fail (priv->ongoing_operation);
  priv->ongoing_operation = FALSE;
}

/**
 * gst_http_str_case_hash:
 * @key: ASCII string to hash
 *
 * Hashes @key in a case-insensitive manner.
 *
 * Return value: the hash code.
 **/
static guint
gst_http_str_case_hash (gconstpointer key)
{
  const char *p = key;
  guint h = g_ascii_toupper (*p);

  if (h)
    for (p += 1; *p != '\0'; p++)
      h = (h << 5) - h + g_ascii_toupper (*p);

  return h;
}

/**
 * gst_http_str_case_equal:
 * @v1: an ASCII string
 * @v2: another ASCII string
 *
 * Compares @v1 and @v2 in a case-insensitive manner
 *
 * Return value: %TRUE if they are equal (modulo case)
 **/
static gboolean
gst_http_str_case_equal (gconstpointer v1, gconstpointer v2)
{
  const char *string1 = v1;
  const char *string2 = v2;

  return g_ascii_strcasecmp (string1, string2) == 0;
}

static void
gst_http_cookie_jar_init (GstHttpCookieJar * jar)
{
  GstHttpCookieJarPrivate *priv = GST_HTTP_COOKIE_JAR_GET_PRIVATE (jar);

  priv->domains = g_hash_table_new_full (gst_http_str_case_hash,
      gst_http_str_case_equal, g_free, NULL);
  priv->serials = g_hash_table_new (NULL, NULL);
}

static void
gst_http_cookie_jar_constructed (GObject * object)
{
  GstHttpCookieJarPrivate *priv = GST_HTTP_COOKIE_JAR_GET_PRIVATE (object);

  priv->constructed = TRUE;
}

static void
gst_http_cookie_jar_finalize (GObject * object)
{
  GstHttpCookieJarPrivate *priv = GST_HTTP_COOKIE_JAR_GET_PRIVATE (object);
  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, priv->domains);
  while (g_hash_table_iter_next (&iter, &key, &value))
    gst_http_cookies_free (value);
  g_hash_table_destroy (priv->domains);
  g_hash_table_destroy (priv->serials);

  G_OBJECT_CLASS (gst_http_cookie_jar_parent_class)->finalize (object);
}

static void
gst_http_cookie_jar_class_init (GstHttpCookieJarClass * jar_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (jar_class);

  g_type_class_add_private (jar_class, sizeof (GstHttpCookieJarPrivate));

  object_class->constructed = gst_http_cookie_jar_constructed;
  object_class->finalize = gst_http_cookie_jar_finalize;

  /**
   * GstHttpCookieJar::changed:
   * @jar: the #GstHttpCookieJar
   * @author: a pointer to the author of the change
   * @old_cookie: the old #GstHttpCookie value
   * @new_cookie: the new #GstHttpCookie value
   *
   * Emitted when @jar changes. If a cookie has been added,
   * @new_cookie will contain the newly-added cookie and
   * @old_cookie will be %NULL. If a cookie has been deleted,
   * @old_cookie will contain the to-be-deleted cookie and
   * @new_cookie will be %NULL. If a cookie has been changed,
   * @old_cookie will contain its old value, and @new_cookie its
   * new value.
   *
   * Signal listeners should check the author field to verify
   * if the change was made by themselves and ignore that change
   **/
  signals[CHANGED] =
      g_signal_new ("changed",
      G_OBJECT_CLASS_TYPE (object_class),
      G_SIGNAL_RUN_FIRST,
      G_STRUCT_OFFSET (GstHttpCookieJarClass, changed),
      NULL, NULL,
      NULL,
      G_TYPE_NONE, 3,
      G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE,
      GST_TYPE_HTTP_COOKIE | G_SIGNAL_TYPE_STATIC_SCOPE,
      GST_TYPE_HTTP_COOKIE | G_SIGNAL_TYPE_STATIC_SCOPE);
}

/**
 * gst_http_cookie_jar_new:
 *
 * Creates a new #GstHttpCookieJar. The base #GstHttpCookieJar class does
 * not support persistent storage of cookies; use a subclass for that.
 *
 * Returns: a new #GstHttpCookieJar
 *
 * Since: 1.10
 **/
GstHttpCookieJar *
gst_http_cookie_jar_new (void)
{
  return g_object_new (GST_TYPE_HTTP_COOKIE_JAR, NULL);
}

static void
gst_http_cookie_jar_changed (GstHttpCookieJar * jar, gpointer author,
    GstHttpCookie * old, GstHttpCookie * new)
{
  GstHttpCookieJarPrivate *priv = GST_HTTP_COOKIE_JAR_GET_PRIVATE (jar);

  if (old && old != new)
    g_hash_table_remove (priv->serials, old);
  if (new) {
    priv->serial++;
    g_hash_table_insert (priv->serials, new, GUINT_TO_POINTER (priv->serial));
  }

  if (!priv->constructed)
    return;

  g_signal_emit (jar, signals[CHANGED], 0, author, old, new);
}

static gboolean
date_time_is_past (GDateTime * dt)
{
  GDateTime *now = g_date_time_new_now_utc ();
  GTimeSpan diff;

  diff = g_date_time_difference (now, dt);

  g_date_time_unref (now);

  return diff > 0;
}

/**
 * gst_http_cookie_jar_add_cookie:
 * @jar: a #GstHttpCookieJar
 * @cookie: (transfer full): a #GstHttpCookie
 *
 * Adds @cookie to @jar, emitting the 'changed' signal if we are modifying
 * an existing cookie or adding a valid new cookie ('valid' means
 * that the cookie's expire date is not in the past).
 *
 * @cookie will be 'stolen' by the jar, so don't free it afterwards.
 *
 * Since: 1.10
 **/
void
gst_http_cookie_jar_add_cookie (GstHttpCookieJar * jar, gpointer author,
    GstHttpCookie * cookie)
{
  GstHttpCookieJarPrivate *priv;
  GSList *old_cookies, *oc, *last = NULL;
  GstHttpCookie *old_cookie;

  g_return_if_fail (GST_HTTP_IS_COOKIE_JAR (jar));
  g_return_if_fail (cookie != NULL);

  GST_HTTP_COOKIE_JAR_LOCK (jar);
  if (!gst_http_cookie_jar_check_start_operation (jar)) {
    GST_HTTP_COOKIE_JAR_UNLOCK (jar);
    return;
  }
#if 0
  /* Never accept cookies for public domains. */
  if (!g_hostname_is_ip_address (cookie->domain) &&
      soup_tld_domain_is_public_suffix (cookie->domain)) {
    gst_http_cookie_free (cookie);
    return;
  }
#endif

  priv = GST_HTTP_COOKIE_JAR_GET_PRIVATE (jar);
  old_cookies = g_hash_table_lookup (priv->domains, cookie->domain);
  for (oc = old_cookies; oc; oc = oc->next) {
    old_cookie = oc->data;
    if (!strcmp (cookie->name, old_cookie->name) &&
        !g_strcmp0 (cookie->path, old_cookie->path)) {
      if (cookie->expires && date_time_is_past (cookie->expires)) {
        /* The new cookie has an expired date,
         * this is the way the the server has
         * of telling us that we have to
         * remove the cookie.
         */
        old_cookies = g_slist_delete_link (old_cookies, oc);
        g_hash_table_insert (priv->domains,
            g_strdup (cookie->domain), old_cookies);
        gst_http_cookie_jar_changed (jar, author, old_cookie, NULL);
        gst_http_cookie_free (old_cookie);
        gst_http_cookie_free (cookie);
      } else {
        oc->data = cookie;
        /* TODO something else might have changed */
        if (strcmp (cookie->value, old_cookie->value))
          gst_http_cookie_jar_changed (jar, author, old_cookie, cookie);
        gst_http_cookie_free (old_cookie);
      }

      gst_http_cookie_jar_finish_operation (jar);
      GST_HTTP_COOKIE_JAR_UNLOCK (jar);
      return;
    }
    last = oc;
  }

  /* The new cookie is... a new cookie */
  if (cookie->expires && date_time_is_past (cookie->expires)) {
    gst_http_cookie_free (cookie);
    gst_http_cookie_jar_finish_operation (jar);
    GST_HTTP_COOKIE_JAR_UNLOCK (jar);
    return;
  }

  if (last)
    last->next = g_slist_append (NULL, cookie);
  else {
    old_cookies = g_slist_append (NULL, cookie);
    g_hash_table_insert (priv->domains, g_strdup (cookie->domain), old_cookies);
  }

  gst_http_cookie_jar_changed (jar, author, NULL, cookie);
  gst_http_cookie_jar_finish_operation (jar);
  GST_HTTP_COOKIE_JAR_UNLOCK (jar);
}

/**
 * gst_http_cookie_jar_all_cookies:
 * @jar: a #GstHttpCookieJar
 *
 * Constructs a #GSList with every cookie inside the @jar.
 * The cookies in the list are a copy of the original, so
 * you have to free them when you are done with them.
 *
 * Return value: (transfer full) (element-type Soup.Cookie): a #GSList
 * with all the cookies in the @jar.
 *
 * Since: 1.10
 **/
GSList *
gst_http_cookie_jar_all_cookies (GstHttpCookieJar * jar)
{
  GstHttpCookieJarPrivate *priv;
  GHashTableIter iter;
  GSList *l = NULL;
  gpointer key, value;

  g_return_val_if_fail (GST_HTTP_IS_COOKIE_JAR (jar), NULL);

  priv = GST_HTTP_COOKIE_JAR_GET_PRIVATE (jar);

  GST_HTTP_COOKIE_JAR_LOCK (jar);

  g_hash_table_iter_init (&iter, priv->domains);

  while (g_hash_table_iter_next (&iter, &key, &value)) {
    GSList *p, *cookies = value;
    for (p = cookies; p; p = p->next)
      l = g_slist_prepend (l, gst_http_cookie_copy (p->data));
  }

  GST_HTTP_COOKIE_JAR_UNLOCK (jar);
  return l;
}

/**
 * gst_http_cookie_jar_delete_cookie:
 * @jar: a #GstHttpCookieJar
 * @cookie: a #GstHttpCookie
 *
 * Deletes @cookie from @jar, emitting the 'changed' signal.
 *
 * Since: 1.10
 **/
void
gst_http_cookie_jar_delete_cookie (GstHttpCookieJar * jar, gpointer author,
    GstHttpCookie * cookie)
{
  GstHttpCookieJarPrivate *priv;
  GSList *cookies, *p;

  g_return_if_fail (GST_HTTP_IS_COOKIE_JAR (jar));
  g_return_if_fail (cookie != NULL);

  priv = GST_HTTP_COOKIE_JAR_GET_PRIVATE (jar);

  GST_HTTP_COOKIE_JAR_LOCK (jar);
  if (gst_http_cookie_jar_check_start_operation (jar)) {
    GST_HTTP_COOKIE_JAR_UNLOCK (jar);
    return;
  }

  cookies = g_hash_table_lookup (priv->domains, cookie->domain);
  if (cookies == NULL) {
    gst_http_cookie_jar_finish_operation (jar);
    GST_HTTP_COOKIE_JAR_UNLOCK (jar);
    return;
  }

  for (p = cookies; p; p = p->next) {
    GstHttpCookie *c = (GstHttpCookie *) p->data;
    if (gst_http_cookie_equal (cookie, c)) {
      cookies = g_slist_delete_link (cookies, p);
      g_hash_table_insert (priv->domains, g_strdup (cookie->domain), cookies);
      gst_http_cookie_jar_changed (jar, author, c, NULL);
      gst_http_cookie_free (c);
      gst_http_cookie_jar_finish_operation (jar);
      GST_HTTP_COOKIE_JAR_UNLOCK (jar);
      return;
    }
  }
  gst_http_cookie_jar_finish_operation (jar);
  GST_HTTP_COOKIE_JAR_UNLOCK (jar);
}
