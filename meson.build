project('uaxidmalib', 'cpp',
  version : '0.1.0',
  default_options : ['warning_level=3',
                     'cpp_std=gnu++20',
                     'default_library=both',
                     'buildtype=release'
                    ])

cc = meson.get_compiler('cpp')
get_id = cc.get_id()
cc_version = cc.version()
if get_id == 'gcc' and cc_version.version_compare('>10.0.0')
  message('Enabling gcc static analyzer')
  static_analyzer_flag = '-fanalyzer'
else
  static_analyzer_flag = []
endif


# ===========
# git version
# ===========
git = find_program('git', required: false)

if git.found()
  git_tag = run_command('git', 'describe', '--tags', check: false)
  if git_tag.stdout() != ''
    tag_info = git_tag.stdout().strip()
    lib_version = tag_info
  else
    warning('tag not found, using meson project version as current version')
    tag_info = ''
    lib_version = meson.project_version()
  endif
else
  warning('git not found, no information about current version')
  tag_info = ''
  lib_version = meson.project_version()
endif
message('Current version info: ' + lib_version)
data_git = configuration_data()
data_git.set('CURRENT_GIT_TAG', tag_info)


# ==========
# Libraries
# ==========

incdir = include_directories(['include', 'src'])

subdir('src')

uio_dep = declare_dependency(
  include_directories : [incdir],
  dependencies : [])

uio_lib = library('uio',
                  sources : [uio_sources],
                  dependencies : uio_dep,
                  version: lib_version,
                  soversion: lib_version.split('.')[0],
			            c_args: [static_analyzer_flag],
                  link_with: [],
			            install: true)

dma_dep = declare_dependency(
  include_directories : [incdir],
  dependencies : [])

dma_lib = library('uaxidma',
                  sources : [dma_sources],
                  dependencies : dma_dep,
                  version: lib_version,
                  soversion: lib_version.split('.')[0],
			            c_args: [static_analyzer_flag],
                  link_with: [],
			            install: true)

# ==========
# EXE
# ==========

#rt_dep = cc.find_library('rt', required : true)
#thread_dep = cc.find_library('pthread', required : true)
#gpiod_dep = cc.find_library('gpiod', required : true)

subdir('test')

cyclic_rx_demo = executable('cyclic_rx_demo',
                      cyclic_rx_demo_src,
                      include_directories : [incdir],
                      dependencies : [],
		                  c_args: [static_analyzer_flag],
                      link_with : [dma_lib],
		                  link_args: ['-Wl,--disable-new-dtags'],
                      install : false)

async_tx_demo = executable('async_tx_demo',
                      async_tx_demo_src,
                      include_directories : [incdir],
                      dependencies : [],
		                  c_args: [static_analyzer_flag],
                      link_with : [dma_lib],
		                  link_args: ['-Wl,--disable-new-dtags'],
                      install : false)

# ==========
# pkg-config
# ==========  
install_headers(['include' / 'uaxidma.h'])

pkg_mod = import('pkgconfig')
pkg_mod.generate(dma_lib,
                 version : lib_version,
                 name : 'uaxidma',
                 filebase : 'libuaxidma',
                 description : 'User space DMA library for transferring data between PL and PS')

pkg_mod.generate(uio_lib,
                 version : lib_version,
                 name : 'uio',
                 filebase : 'libuio',
                 description : 'User space IO wrapper library for managing UIO devices')

# ==========
# Doxygen
# ==========
if get_option('documentation_enabled')==true
  doxygen = find_program('doxygen', required : false)

  if doxygen.found()
    cdata = configuration_data()
    cdata.set('VERSION', meson.project_version())
    cdata.set('DOXYGEN_PROJECT_NAME', meson.project_name())
    cdata.set('DOXYGEN_PROJECT_BRIEF', '"User space library for scatter-gather DMA using Xilinx\'s AXI-DMA"')
    cdata.set('TOP_SRCDIR', meson.source_root())
    cdata.set('TOP_BUILDDIR', meson.current_build_dir())

    if find_program('dot', required : false).found()
      cdata.set('HAVE_DOT', 'YES')
    else
      cdata.set('HAVE_DOT', 'NO')
    endif

  doxyfile = configure_file(input: 'Doxyfile.in',
                            output: 'doxyfile',
                            configuration: cdata,
                            install: false)
  custom_target('docs',
                input: doxyfile,
                output: 'docs',
                command: [doxygen, doxyfile],
                install: false)

  else
    warning('Doxygen not installed')
  endif
endif
