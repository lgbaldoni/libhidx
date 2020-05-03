threads_dep = dependency('threads', required: true)

if host_machine.system() != 'windows'
   install_headers (
      'include/libhidx/LibHidx.hh',
      'include/libhidx/Device.hh',
      'include/libhidx/InterfaceHandle.hh',
      'include/libhidx/Interface.hh',
      'include/libhidx/Parser.hh',
      'include/libhidx/hid/Item.hh',
      'include/libhidx/hid/Collection.hh',
      'include/libhidx/hid/Control.hh',
      'include/libhidx/Usages.hh',
      'include/libhidx/hid/Usage.hh',
      'include/libhidx/DeviceStrings.hh'
      'src/Connector.hh',
      subdir: meson.project_name()
endif

libhidx_sources = [
   'src/LibHidx.cc',
   'src/Device.cc',
   'src/InterfaceHandle.cc',
   'src/Interface.cc',
   'src/Parser.cc',
   'src/hid/Item.cc',
   'src/hid/Collection.cc',
   'src/hid/Control.cc',
   'src/Usages.cc',
   'src/hid/Usage.cc',
   'src/subprocess.hh',
   'src/Connector.cc'
]

libhidx = library(
   'hidx',
   version: meson.project_version(),
   sources: libhidx_sources,
   include_directories: [
      include_directories('include'),
      include_directories('src')
   ],
   dependencies: [
      threads_dep,
      libhidx_server_dep,
      protobuf_dep,
      buffer_gen_dep
   ],
   install: (host_machine.system() != 'windows')
)

libhidx_dep = declare_dependency(
   include_directories: include_directories('include'),
   link_with: libhidx
)
