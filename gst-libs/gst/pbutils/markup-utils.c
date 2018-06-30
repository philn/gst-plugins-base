/* GStreamer base utils library text markup utility functions
 * Copyright (C) 2018 Philippe Normand <philn@igalia.com>
 *
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


/**
 * SECTION:gstpbutilsmarkuputils
 * @title: Markup utilities
 * @short_description: Miscellaneous text markup-specific utility functions
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pbutils.h"

#define ATTRIBUTE_REGEX "\\s?[a-zA-Z0-9\\. \t\\(\\)]*"
static const gchar *allowed_srt_tags[] = { "i", "b", "u", NULL };
static const gchar *allowed_vtt_tags[] =
    { "i", "b", "c", "u", "v", "ruby", "rt", NULL };

/* we want to escape text in general, but retain basic markup like
 * <i></i>, <u></u>, and <b></b>. The easiest and safest way is to
 * just unescape a white list of allowed markups again after
 * escaping everything (the text between these simple markers isn't
 * necessarily escaped, so it seems best to do it like this) */
static void
markup_utils_unescape_formatting (gchar * txt, gconstpointer allowed_tags_ptr,
    gboolean allows_tag_attributes)
{
  gchar *res;
  GRegex *tag_regex;
  gchar *allowed_tags_pattern, *search_pattern;
  const gchar *replace_pattern;

  /* No processing needed if no escaped tag marker found in the string. */
  if (strstr (txt, "&lt;") == NULL)
    return;

  /* Build a list of alternates for our regexp.
   * FIXME: Could be built once and stored */
  allowed_tags_pattern = g_strjoinv ("|", (gchar **) allowed_tags_ptr);
  /* Look for starting/ending escaped tags with optional attributes. */
  search_pattern = g_strdup_printf ("&lt;(/)?\\ *(%s)(%s)&gt;",
      allowed_tags_pattern, ATTRIBUTE_REGEX);
  /* And unescape appropriately */
  if (allows_tag_attributes) {
    replace_pattern = "<\\1\\2\\3>";
  } else {
    replace_pattern = "<\\1\\2>";
  }

  tag_regex = g_regex_new (search_pattern, 0, 0, NULL);
  res = g_regex_replace (tag_regex, txt, strlen (txt), 0,
      replace_pattern, 0, NULL);

  /* res will always be shorter than the input or identical, so this
   * copy is OK */
  strcpy (txt, res);

  g_free (res);
  g_free (search_pattern);
  g_free (allowed_tags_pattern);

  g_regex_unref (tag_regex);
}

/* we only allow a fixed set of tags like <i>, <u> and <b>, so let's
 * take a simple approach. This code assumes the input has been
 * escaped and subrip_unescape_formatting() has then been run over the
 * input! This function adds missing closing markup tags and removes
 * broken closing tags for tags that have never been opened. */
static void
markup_utils_fix_up_markup (gchar ** p_txt, gconstpointer allowed_tags_ptr)
{
  gchar *cur, *next_tag;
  GPtrArray *open_tags = NULL;
  guint num_open_tags = 0;
  const gchar *iter_tag;
  guint offset = 0;
  guint index;
  gchar *cur_tag;
  gchar *end_tag;
  GRegex *tag_regex;
  GMatchInfo *match_info;
  gchar **allowed_tags = (gchar **) allowed_tags_ptr;

  g_assert (*p_txt != NULL);

  open_tags = g_ptr_array_new_with_free_func (g_free);
  cur = *p_txt;
  while (*cur != '\0') {
    next_tag = strchr (cur, '<');
    if (next_tag == NULL)
      break;
    offset = 0;
    index = 0;
    while (index < g_strv_length (allowed_tags)) {
      iter_tag = allowed_tags[index];
      /* Look for a white listed tag */
      cur_tag = g_strconcat ("<", iter_tag, ATTRIBUTE_REGEX, ">", NULL);
      tag_regex = g_regex_new (cur_tag, 0, 0, NULL);
      (void) g_regex_match (tag_regex, next_tag, 0, &match_info);

      if (g_match_info_matches (match_info)) {
        gint start_pos, end_pos;
        gchar *word = g_match_info_fetch (match_info, 0);
        g_match_info_fetch_pos (match_info, 0, &start_pos, &end_pos);
        if (start_pos == 0) {
          offset = strlen (word);
        }
        g_free (word);
      }
      g_match_info_free (match_info);
      g_regex_unref (tag_regex);
      g_free (cur_tag);
      index++;
      if (offset) {
        /* OK we found a tag, let's keep track of it */
        g_ptr_array_add (open_tags, g_ascii_strdown (iter_tag, -1));
        ++num_open_tags;
        break;
      }
    }

    if (offset) {
      next_tag += offset;
      cur = next_tag;
      continue;
    }

    if (*next_tag == '<' && *(next_tag + 1) == '/') {
      end_tag = strchr (cur, '>');
      if (end_tag) {
        const gchar *last = NULL;
        if (num_open_tags > 0)
          last = g_ptr_array_index (open_tags, num_open_tags - 1);
        if (num_open_tags == 0
            || g_ascii_strncasecmp (end_tag - 1, last, strlen (last))) {
          GST_LOG ("broken input, closing tag '%s' is not open", end_tag - 1);
          memmove (next_tag, end_tag + 1, strlen (end_tag) + 1);
          next_tag -= strlen (end_tag);
        } else {
          --num_open_tags;
          g_ptr_array_remove_index (open_tags, num_open_tags);
        }
      }
    }
    ++next_tag;
    cur = next_tag;
  }

  if (num_open_tags > 0) {
    GString *s;

    s = g_string_new (*p_txt);
    while (num_open_tags > 0) {
      GST_LOG ("adding missing closing tag '%s'",
          (char *) g_ptr_array_index (open_tags, num_open_tags - 1));
      g_string_append_c (s, '<');
      g_string_append_c (s, '/');
      g_string_append (s, g_ptr_array_index (open_tags, num_open_tags - 1));
      g_string_append_c (s, '>');
      --num_open_tags;
    }
    g_free (*p_txt);
    *p_txt = g_string_free (s, FALSE);
  }
  g_ptr_array_free (open_tags, TRUE);
}

static gboolean
subrip_remove_unhandled_tag (gchar * start, gchar * stop)
{
  gchar *tag, saved;

  tag = start + strlen ("&lt;");
  if (*tag == '/')
    ++tag;

  if (g_ascii_tolower (*tag) < 'a' || g_ascii_tolower (*tag) > 'z')
    return FALSE;

  saved = *stop;
  *stop = '\0';
  GST_LOG ("removing unhandled tag '%s'", start);
  *stop = saved;
  memmove (start, stop, strlen (stop) + 1);
  return TRUE;
}

/* remove tags we haven't explicitly allowed earlier on, like font tags
 * for example */
static void
subrip_remove_unhandled_tags (gchar * txt)
{
  gchar *pos, *gt;

  for (pos = txt; pos != NULL && *pos != '\0'; ++pos) {
    if (strncmp (pos, "&lt;", 4) == 0 && (gt = strstr (pos + 4, "&gt;"))) {
      if (subrip_remove_unhandled_tag (pos, gt + strlen ("&gt;")))
        --pos;
    }
  }
}

static void
strip_trailing_newlines (gchar * txt)
{
  if (txt) {
    guint len;

    len = strlen (txt);
    while (len > 1 && txt[len - 1] == '\n') {
      txt[len - 1] = '\0';
      --len;
    }
  }
}

/**
 * gst_markup_utils_sanitize_subrip_text:
 * @p_txt: Pointer to a UTF-8 string
 * @is_webvtt: Wether the data may contain WebVTT markup or not
 *
 * Ensure the SubRip cue text contains valid Pango markup data.
 * The @p_txt string is modified in-place.
 */
void
gst_markup_utils_sanitize_subrip_text (gchar ** p_txt, gboolean is_webvtt)
{
  gconstpointer allowed_tags = is_webvtt ? allowed_vtt_tags : allowed_srt_tags;
  markup_utils_unescape_formatting (*p_txt, allowed_tags, is_webvtt);
  subrip_remove_unhandled_tags (*p_txt);
  strip_trailing_newlines (*p_txt);
  markup_utils_fix_up_markup (p_txt, allowed_tags);
}
