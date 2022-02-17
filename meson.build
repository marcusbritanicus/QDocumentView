project(
		'QDocumentView',
		'cpp',
		version: '1.0.0',
		license: 'GPLv3',
		meson_version: '>=0.59.0',
		default_options: [
				'cpp_std=c++17',
				'c_std=c11',
				'warning_level=2',
				'werror=false',
		],
)

add_global_arguments( '-DPROJECT_VERSION=' + meson.project_version(), language : 'cpp' )
add_project_link_arguments( ['-rdynamic','-fPIC'], language:'cpp' )

if get_option('use_qt_version') == 'qt5'
		Qt = import( 'qt5' )
		QtCore    = dependency( 'qt5', modules: [ 'Core' ], private_headers: [ 'Core' ] )
    QtGui     = dependency( 'qt5', modules: [ 'Gui' ], private_headers: [ 'Gui' ] )
    QtWidgets = dependency( 'qt5', modules: [ 'Widgets' ], private_headers: [ 'Widgets' ] )
    QtNetwork = dependency( 'qt5', modules: [ 'Network' ] )
    QtPrint   = dependency( 'qt5', modules: [ 'PrintSupport' ] )
    Poppler   = dependency( 'poppler-qt5' )

    libName   = 'Qt5DocumentView'

elif get_option('use_qt_version') == 'qt6'
		Qt = import( 'qt6' )
		QtCore    = dependency( 'qt6', modules: [ 'Core' ], private_headers: [ 'Core' ] )
    QtGui     = dependency( 'qt6', modules: [ 'Gui' ], private_headers: [ 'Gui' ] )
    QtWidgets = dependency( 'qt6', modules: [ 'Widgets' ], private_headers: [ 'Widgets' ] )
    QtNetwork = dependency( 'qt6', modules: [ 'Network' ] )
    QtPrint   = dependency( 'qt6', modules: [ 'PrintSupport' ] )
    Poppler   = dependency( 'poppler-qt6' )

    libName   = 'Qt6DocumentView'

endif

Deps = [ QtCore, QtGui, QtWidgets, QtNetwork, QtPrint, Poppler ]

Headers = [
    'includes/qdocumentview/QDocument.hpp',
    'includes/qdocumentview/QDocumentNavigation.hpp',
    'includes/qdocumentview/QDocumentRenderer.hpp',
    'includes/qdocumentview/QDocumentRenderOptions.hpp',
    'includes/qdocumentview/QDocumentSearch.hpp',
    'includes/qdocumentview/QDocumentView.hpp',
]

Sources = [
    'Document/QDocument.cpp',
    'Document/QDocumentNavigation.cpp',
    'Document/QDocumentRenderer.cpp',
    'Document/QDocumentSearch.cpp',
    'View/QDocumentView.cpp',
    'View/ViewImpl.cpp',
    'View/ViewWidgets.cpp',
]

Mocs = Qt.compile_moc(
 		headers : [ Headers, 'Document/QDocumentRendererImpl.hpp', 'View/ViewImpl.hpp', 'View/ViewWidgets.hpp' ],
	 	dependencies: Deps
)
includes = [ 'includes/qdocumentview', 'Document', 'View' ]

# configure_file(
# 		input : 'config.h.in',
#     output : 'desq-config.h',
# 		install: true,
#     install_dir: join_paths( 'include', 'desq' ),
#     configuration : conf_data
# )

qdocview = shared_library(
    libName, [ Sources, Mocs ],
		version: meson.project_version(),
		include_directories: [ includes ],
    dependencies: Deps,
    install: true,
    install_dir: join_paths( get_option( 'libdir' ) )
)

install_subdir(
		'includes/qdocumentview',
		install_dir: get_option( 'includedir' )
)

summary = [
	'',
	'-------------------',
	'QDocumentView @0@'.format( meson.project_version() ),
	'-------------------',
	''
]
message( '\n'.join( summary ) )

## PkgConfig Section
pkgconfig = import( 'pkgconfig' )
pkgconfig.generate(
		qdocview,
		name: libName,
		version: meson.project_version(),
		filebase: libName,
		description: 'A Multi-page Document Rendering class for Qt',
)
