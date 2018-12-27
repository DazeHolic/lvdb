{
	'targets': [
	{
         'target_name': 'lvdb',
         'type': 'static_library',
		 'dependencies':[ 		    
			'<(DEPTH)/third_party/timeout/timeout.gyp:timeout',
			'<(DEPTH)/third_party/leveldb/leveldb.gyp:leveldb',
			'<(DEPTH)/third_party/yaml-cpp/yaml-cpp.gyp:yaml-cpp',					
			'<(DEPTH)/toolkits/toolkits.gyp:toolkits',
		 ],
		 'conditions': [
           ['OS=="win"', {
		   	  'dependencies':[ 
				 '<(DEPTH)/third_party/pthreads/pthreads.gyp:pthreads-static',
		       ],
		     }
		   ],
		],
		 'direct_dependent_settings': {
            'include_dirs': [
				'include'
             ],
        },
		 'include_dirs': [
			'include', 'src',
		 ],
		 'sources': [
			'include/lvdb/auto.h',
			'include/lvdb/binlog.h',
			'include/lvdb/bytes.h',
			'include/lvdb/const.h',
			'include/lvdb/iterator.h',
			'include/lvdb/lvdb.h',
			'include/lvdb/options.h',
			'include/lvdb/strings.h',
			'include/lvdb/sync.h',
			'include/lvdb/t_hash.h',
			'include/lvdb/t_kv.h',
			'include/lvdb/t_meta.h',
			'include/lvdb/t_queue.h',
			'include/lvdb/t_zset.h',
			'src/auto.cpp',
			'src/binlog.cpp',
			'src/binlog_queue.h',
			'src/binlog_queue.cpp',
			'src/bytes.cpp',
			'src/iterator.cpp',
			'src/lvdb_impl.h',
			'src/lvdb_impl.cpp',
			'src/options.cpp',
			'src/sync.cpp',
			'src/t_hash.cpp',
			'src/t_kv.cpp',
			'src/t_meta.cpp',
			'src/t_queue.cpp',
			'src/t_zset.cpp',
		 ],
	},
	{
	    'target_name':'lvdbapi',
        'type':'static_library',
        'dependencies':[ 
		    '<(DEPTH)/third_party/protobuf/protobuf.gyp:libprotobuf-dll',
			'lvdb',
		],
		'sources':[
			'include/lvdb/http.h',
			'include/lvdb/pb.h',
			'src/dbapi.proto',
			'src/http.cpp',
			'src/pb.cpp',
		],
		'defines':[
			'LIBPROTOBUF_EXPORTS',
		],
            'variables': {
              'proto_in_dir': 'src',
              'proto_out_protected': 'pbcpp',
              'proto_out_dir': '<(proto_out_protected)',
			  'cc_generator_options': 'dllexport_decl=LIBPROTOBUF_EXPORT:',
            },
            'includes': ['../build/protoc.gypi',],		
	},	   
	{
	    'target_name':'lvdb_test',
        'type':'executable',
        'dependencies':[ 
			'lvdb',
			'<(DEPTH)/third_party/testing/gtest.gyp:gtest',	
		],
		'sources':['test/main.cpp']
	},
	]
}
