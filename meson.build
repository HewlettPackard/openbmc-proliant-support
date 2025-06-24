project(
    'openbmc-proliant',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++20',
	'b_lto=true',
	'b_lto_mode=default',
        'b_lto_threads=0'
    ],
    license: 'Apache-2.0',
    version: '1.0',
)

cxx = meson.get_compiler('cpp')

add_project_arguments('-Wno-psabi', language: [ 'cpp' ])

cpp = meson.get_compiler('cpp')

executable('hpe-uefi-version',
           'src/hpe-uefi-version.cpp',
           'src/uefi.cpp',
        implicit_include_directories: false,
        include_directories: ['include'],
        install: true,
        install_dir: get_option('bindir'))

executable('hpe-platdef-extract',
           'src/hpe-platdef-extract.cpp',
        implicit_include_directories: false,
        include_directories: ['include'],
        install: true,
        install_dir: get_option('bindir'))

