{
  "targets": [
    {
      "target_name": "bubo",
      "sources": [
        "src/attrs-table.cc",
        "src/blob-store.cc",
        "src/bubo-types.cc",
        "src/utils.cc",
        "src/bubo.cc",
        "src/strings-table.cc",
        "src/test.cc"
      ],
      "include_dirs": [
            "<!(node -e \"require('nan')\")"
      ],
      'conditions': [
        [
          'OS=="mac"',
          {
            'xcode_settings': {
              'OTHER_CPLUSPLUSFLAGS' : ['-std=c++11','-stdlib=libc++'],
              'OTHER_LDFLAGS': ['-stdlib=libc++'],
              'MACOSX_DEPLOYMENT_TARGET': '10.7'
            },
            'libraries' : ['-lz']
          }
        ],[
          'OS=="linux"',
          {
            'cflags': ['-std=c++11'],
            'libraries' : ['-lz']
          }
        ]
      ]
    }
  ]
}
