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

add_project_arguments( '-DPROJECT_VERSION="v@0@"'.format( meson.project_version() ), language : 'cpp' )
add_project_arguments( '-DQDV_PLUGIN_PATH="@0@"'.format( join_paths( get_option( 'prefix' ), get_option( 'libdir' ), 'qdocumentview', 'plugins' ) ), language : 'cpp' )

add_project_link_arguments( ['-rdynamic','-fPIC'], language:'cpp' )

if get_option('use_qt_version') == 'qt5'
	Qt = import( 'qt5' )
	QtBase     = dependency( 'qt5', modules: [ 'Core', 'Gui', 'Widgets' ] )
    QtPrint    = dependency( 'qt5', modules: [ 'PrintSupport' ] )
    Poppler    = dependency( 'poppler-qt5' )

    libName    = 'Qt5DocumentView'
    subdirname = 'qdocumentview/qt5'

elif get_option('use_qt_version') == 'qt6'
	Qt = import( 'qt6' )
	QtBase     = dependency( 'qt6', modules: [ 'Core', 'Gui', 'Widgets' ] )
    QtPrint    = dependency( 'qt6', modules: [ 'PrintSupport' ] )
    Poppler    = dependency( 'poppler-qt6' )

    libName    = 'Qt6DocumentView'
    subdirname = 'qdocumentview/qt6'

endif

DjVuLibre  = dependency( 'ddjvuapi', required: false )
LibSpectre = dependency( 'libspectre', required: false )

Cups       = dependency( 'cups', required: false )
add_project_arguments( '-DHAVE_CUPS=@0@'.format( Cups.found() ), language : 'cpp' )

Deps     = [ QtBase, QtPrint, Poppler ]
Requires = [ Poppler, ]

Headers = [
    'includes/qdocumentview/QDocument.hpp',
    'includes/qdocumentview/QDocumentNavigation.hpp',
    'includes/qdocumentview/QDocumentRenderer.hpp',
    'includes/qdocumentview/QDocumentRenderOptions.hpp',
    'includes/qdocumentview/QDocumentPrintOptions.hpp',
    'includes/qdocumentview/QDocumentSearch.hpp',
    'includes/qdocumentview/QDocumentView.hpp',
    'includes/qdocumentview/PopplerDocument.hpp',
    'includes/qdocumentview/QDocumentPluginInterface.hpp',
]

ImplHeaders = [
	'Document/QDocumentNavigationImpl.hpp',
	'Document/RendererImpl.hpp',
	'View/ViewImpl.hpp',
	'View/ViewToolbar.hpp'
]

Sources = [
    'Document/QDocument.cpp',
    'Document/QDocumentNavigation.cpp',
    'Document/QDocumentRenderer.cpp',
    'Document/QDocumentSearch.cpp',
    'PdfView/PopplerDocument.cpp',
    'View/QDocumentView.cpp',
    'View/ViewImpl.cpp',
    'View/ViewToolbar.cpp',
]

if Cups.found()
	Deps     += [ Cups, ]
	Requires += [ Cups, ]
endif

Mocs = Qt.compile_moc(
	headers : [ Headers, ImplHeaders ],
	dependencies: Deps
)
Includes = include_directories( [ 'includes', 'includes/qdocumentview', 'Document', 'View' ] )

qdocview = shared_library(
    libName, [ Sources, Mocs ],
	version: meson.project_version(),
	include_directories: [ Includes ],
    dependencies: Deps,
    install: true,
    install_dir: join_paths( get_option( 'libdir' ) )
)

subdir( 'Plugins' )

install_headers( Headers, subdir: subdirname )

## PkgConfig Section
pkgconfig = import( 'pkgconfig' )
pkgconfig.generate(
	qdocview,
	name: libName,
	subdirs: subdirname,
	version: meson.project_version(),
	filebase: libName,
	description: 'A Multi-page Document Rendering class for Qt',
	requires: Requires,
)
