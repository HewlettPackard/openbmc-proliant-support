project(
    'openbmc-proliant',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++23',
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

# Dependencies
sdbusplus_dep = dependency('sdbusplus', fallback: ['sdbusplus', 'sdbusplus_dep'])
systemd_dep = dependency('systemd')

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

executable('host_state_monitor',
           'src/host_state_monitor.cpp',
           dependencies: [
               sdbusplus_dep,
               systemd_dep,
           ],
           implicit_include_directories: false,
           include_directories: ['include'],
           install: true,
           install_dir: get_option('bindir'))

executable('gsc_fru_service',
           'src/gsc_fru_service.cpp',
           dependencies: [
               sdbusplus_dep,
               systemd_dep,
           ],
           implicit_include_directories: false,
           include_directories: ['include'],
           install: true,
           install_dir: get_option('bindir'))

# Install systemd service file
systemd_system_unit_dir = systemd_dep.get_variable('systemdsystemunitdir')
configure_file(
    input: 'service_files/host-state-monitor.service',
    output: 'host-state-monitor.service',
    configuration: {
        'bindir': get_option('prefix') / get_option('bindir')
    },
    install: true,
    install_dir: systemd_system_unit_dir
)
configure_file(
    input: 'service_files/gsc-fru-service.service',
    output: 'gsc-fru-service.service',
    configuration: {
        'bindir': get_option('prefix') / get_option('bindir')
    },
    install: true,
    install_dir: systemd_system_unit_dir
)


