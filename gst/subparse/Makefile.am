plugin_LTLIBRARIES = libgstsubparse.la

SAMIPARSE_SOURCES = samiparse.c samiparse.h

libgstsubparse_la_SOURCES = \
	gstssaparse.c \
	gstssaparse.h \
	gstsubparse.c \
	gstsubparse.h \
	$(SAMIPARSE_SOURCES) \
	tmplayerparse.c \
	tmplayerparse.h \
	mpl2parse.c \
	mpl2parse.h \
        qttextparse.c \
        qttextparse.h

libgstsubparse_la_CFLAGS = $(GST_CFLAGS) $(GST_BASE_CFLAGS) $(GST_PLUGINS_BASE_CFLAGS)
libgstsubparse_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstsubparse_la_LIBADD = $(GST_LIBS) $(top_builddir)/gst-libs/gst/pbutils/libgstpbutils-$(GST_API_VERSION).la $(GST_BASE_LIBS) $(GST_PLUGINS_BASE_LIBS)

noinst_HEADERS = \
	gstssaparse.h \
	gstsubparse.h \
	samiparse.h \
	tmplayerparse.h \
	mpl2parse.h \
        qttextparse.h
