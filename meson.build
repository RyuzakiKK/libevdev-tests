project('libevdev-tests', 'c',
	version : '1',
	license : 'MIT/Expat',
	default_options : [ 'c_std=gnu99', 'warning_level=2' ])

cflags = ['-Wno-unused-parameter', '-Wmissing-prototypes', '-Wstrict-prototypes']
add_project_arguments(cflags, language : 'c')

dep_libinput = dependency('libinput')
dep_libudev = dependency('libudev')
dep_libevdev = dependency('libevdev')

libev_src = files('libevdev-tests.c')
libev_deps = [
	dep_libinput,
	dep_libudev,
	dep_libevdev,
]
executable('libevdev-tests',
	   libev_src,
	   dependencies: libev_deps,
	   install: true)