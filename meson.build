project('vcm', 'cpp',
  default_options : ['buildtype=release', 'b_ndebug=if-release', 'cpp_std=c++14'],
  meson_version : '>=0.64.0',
  version : '1.0.2'
)

cpp = meson.get_compiler('cpp')

# The plugin is a single translation unit: vcm.cpp includes all filter sources.
sources = ['vcm.cpp']

version_parts = meson.project_version().split('.')
add_project_arguments(
  '-DVCM_VERSION_MAJOR=' + version_parts[0],
  '-DVCM_VERSION_MINOR=' + version_parts[1],
  '-DSTATIC_FFTW3',  # link fftw3f directly instead of loading libfftw3f-3.dll at run time
  language : 'cpp')

# Prefer system VapourSynth headers, fall back to the vendored API 4 headers.
system_vs_dep = dependency('vapoursynth', version : '>=55', required : false)
if system_vs_dep.found()
  vapoursynth_dep = system_vs_dep.partial_dependency(compile_args : true, includes : true)
else
  vapoursynth_dep = declare_dependency(include_directories : include_directories('include/vapoursynth'))
endif

# fftw3f: either from an explicit prefix (e.g. a vcpkg triplet directory on Windows)
# or through pkg-config. -Dprefer_static=true links the static library when available.
fftw3_dir = get_option('fftw3_dir')
if fftw3_dir != ''
  fftw3f_lib = cpp.find_library('fftw3f', dirs : [fftw3_dir / 'lib'], static : get_option('prefer_static'))
  # compile_args instead of include_directories(): the prefix may be an absolute path (even inside the source tree)
  fftw3f_dep = declare_dependency(
    compile_args : ['-I' + fftw3_dir / 'include'],
    dependencies : fftw3f_lib)
else
  fftw3f_dep = dependency('fftw3f')
endif

deps = [vapoursynth_dep, fftw3f_dep]

if host_machine.system() != 'windows'
  # LoadLibrary/GetProcAddress, fopen_s, _mm_malloc, ... for the Unix build
  add_project_arguments('-include', 'compat.h', language : 'cpp')
endif

if host_machine.cpu_family().startswith('x86') and cpp.get_argument_syntax() == 'gcc'
  add_project_arguments('-mfpmath=sse', '-msse2', language : 'cpp')
endif

# VapourSynth only autoloads *.dylib on macOS, while Meson defaults to *.so for modules.
module_kwargs = {}
if host_machine.system() == 'darwin'
  module_kwargs += {'name_suffix' : 'dylib'}
endif

if get_option('python_package')
  # Same layout as the VapourSynth PyPI package autoloads from: site-packages/vapoursynth/plugins
  py = import('python').find_installation(pure : false)
  install_dir = join_paths(py.get_install_dir(), 'vapoursynth', 'plugins')
else
  libdir = join_paths(get_option('prefix'), get_option('libdir'))
  if system_vs_dep.found()
    libdir = system_vs_dep.get_variable(pkgconfig : 'libdir', default_value : libdir)
  endif
  install_dir = join_paths(libdir, 'vapoursynth')
endif

shared_module('vcm', sources,
  dependencies : deps,
  include_directories : include_directories('.'),
  install : true,
  install_dir : install_dir,
  gnu_symbol_visibility : 'hidden',
  kwargs : module_kwargs
)
