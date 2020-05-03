buffer_gen = custom_target(
  'Buffer.pb.[cch]',
  output: ['Buffer.pb.cc', 'Buffer.pb.h'],
  input: 'Buffer.proto',
  command: [protoc,
     '--proto_path=@CURRENT_SOURCE_DIR@',
     '--cpp_out=@OUTDIR@',
     '@INPUT@'
  ]
)

buffer_gen_dep = declare_dependency(
   sources: buffer_gen,
   include_directories: include_directories('.')
)
