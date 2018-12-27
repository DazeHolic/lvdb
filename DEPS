use_relative_paths = True

vars = {
    'gitcode': 'git+http://code.rd.nd/scm/git'
}

deps = {
    'build': Var("gitcode") + "/build",
	'third_party/gflags': Var("gitcode") + "/gflags.gyp",
    'third_party/gflags/src': Var("gitcode") + "/gflags",
    'toolkits': Var("gitcode") + "/toolkits",
	#toolkits deps
    'third_party/libevent': Var("gitcode") + "/libevent.gyp",
    'third_party/libevent/src': Var("gitcode") + "/libevent",
    'third_party/libwebsockets': Var("gitcode") + "/libwebsockets.gyp",
    'third_party/libwebsockets/src': Var("gitcode") + "/libwebsockets",
    'third_party/zlib': Var("gitcode") + "/zlib",
    'third_party/openssl': Var("gitcode") + "/openssl",
    'third_party/modp_b64': Var("gitcode") + "/modp_b64",
    'third_party/breakpad': Var("gitcode") + "/breakpad.gyp",
    'third_party/breakpad/src': Var("gitcode") + "/breakpad",
    'third_party/testing': Var("gitcode") + "/testing",
    #toolkits deps	
    'third_party/protobuf': Var("gitcode") + "/protobuf.gyp",
    'third_party/protobuf/src': Var("gitcode") + "/protobuf",
	#leveldb 
    'third_party/leveldb': Var("gitcode") + "/leveldb.gyp",
    'third_party/leveldb/leveldb': Var("gitcode") + "/leveldb",
    'third_party/leveldb/snappy': Var("gitcode") + "/snappy.gyp",
    'third_party/leveldb/snappy/snappy': Var("gitcode") + "/snappy",	
    'third_party/timeout': Var("gitcode") + "/timeout.gyp",
    'third_party/timeout/src': Var("gitcode") + "/timeout",
	#yaml
	'third_party/yaml-cpp': Var("gitcode") + "/yaml-cpp",
	'third_party/pthreads': Var("gitcode") + "/pthreads",
}