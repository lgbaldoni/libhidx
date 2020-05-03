if host_machine.system() != 'windows'
   install_headers(
      'src/args.hh',
      subdir: meson.project_name()
   )
endif

libhidx_server_daemon_sources = [
   'src/main.cc'
]

libhidx_server_daemon = executable(
    'hidx_server',
    sources: libhidx_server_daemon_sources,
    dependencies: libhidx_server_dep,
    install: (host_machine.system() != 'windows')
)
