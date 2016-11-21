http_sources = [
  'gsthttpcookie.c',
  'gsthttpcookiejar.c'
]

http_headers = [
  'gsthttpcookie.h',
  'gsthttpcookiejar.h'
]
install_headers(http_headers, subdir : 'gstreamer-1.0/gst/http/')

gsthttp = library('gsthttp-@0@'.format(api_version),
  http_sources,
  c_args : gst_plugins_base_args,
  include_directories: [configinc, libsinc],
  version : libversion,
  soversion : soversion,
  install : true,
  dependencies : [gst_dep],
  vs_module_defs: vs_module_defs_dir + 'libgsthttp.def',
)

http_gen_sources  = []
if build_gir
  http_gen_sources += [gnome.generate_gir(gsthttp,
    sources : http_sources + http_headers,
    namespace : 'GstHttp',
    nsversion : api_version,
    identifier_prefix : 'Gst',
    symbol_prefix : 'gst',
    export_packages : 'gstreamer-http-1.0',
    includes : ['Gst-1.0'],
    install : true,
    extra_args : gir_init_section,
    dependencies : [gst_dep]
  )]
endif

http_dep = declare_dependency(link_with: gsthttp,
  include_directories : [libsinc],
  dependencies : [gst_dep],
  sources: http_gen_sources)
