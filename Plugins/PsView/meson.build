PsDeps     = [ Deps, LibSpectre ]
PsHeaders  = [ 'PsDocument.hpp', 'PsPlugin.hpp', ]
PsSources  = [ 'PsDocument.cpp', 'PsPlugin.cpp', ]

PsMocs = Qt.compile_moc(
	headers : PsHeaders,
	dependencies: PsDeps,
	include_directories: [ join_paths( get_option( 'prefix' ), get_option( 'includedir' ) ), Includes ],
)

pluginPs = shared_module(
	'qdv-plugin-ps', [ PsSources, PsMocs ],
	dependencies: PsDeps,
	include_directories: [ join_paths( get_option( 'prefix' ), get_option( 'includedir' ) ), Includes ],
	link_with: qdocview,
	install: true,
	install_dir: join_paths( get_option( 'libdir' ), 'qdocumentview', 'plugins' )
)
