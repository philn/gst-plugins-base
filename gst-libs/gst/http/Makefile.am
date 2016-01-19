lib_LTLIBRARIES = libgsthttp-@GST_API_VERSION@.la

libgsthttp_@GST_API_VERSION@_la_SOURCES = \
	gsthttpcookie.c gsthttpcookiejar.c

libgsthttp_@GST_API_VERSION@includedir = \
	$(includedir)/gstreamer-@GST_API_VERSION@/gst/http

libgsthttp_@GST_API_VERSION@include_HEADERS = \
	gsthttpcookie.h gsthttpcookiejar.h

noinst_HEADERS =

libgsthttp_@GST_API_VERSION@_la_CFLAGS = \
	$(GST_PLUGINS_BAD_CFLAGS) \
	-DGST_USE_UNSTABLE_API \
	$(GST_CFLAGS)

libgsthttp_@GST_API_VERSION@_la_LIBADD = \
	$(GST_BASE_LIBS) \
	$(GST_LIBS)

libgsthttp_@GST_API_VERSION@_la_LDFLAGS = \
	$(GST_LIB_LDFLAGS) \
	$(GST_ALL_LDFLAGS) \
	$(GST_LT_LDFLAGS)
