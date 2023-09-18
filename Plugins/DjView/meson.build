DjDeps     = [ Deps, DjVuLibre ]
DjHeaders  = [ 'DjVuDocument.hpp', 'DjVuPlugin.hpp', ]
DjSources  = [ 'DjVuDocument.cpp', 'DjVuPlugin.cpp', ]

DjMocs = Qt.compile_moc(
	headers : DjHeaders,
	dependencies: DjDeps,
	include_directories: [ join_paths( get_option( 'prefix' ), get_option( 'includedir' ) ), Includes ],
)

pluginDj = shared_module(
	'qdv-plugin-djvu', [ DjSources, DjMocs ],
	dependencies: DjDeps,
	include_directories: [ join_paths( get_option( 'prefix' ), get_option( 'includedir' ) ), Includes ],
	link_with: qdocview,
	install: true,
	install_dir: join_paths( get_option( 'libdir' ), 'qdocumentview', 'plugins' )
)
