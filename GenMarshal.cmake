find_package(PkgConfig)

# Due to a bug in the parser a hash mark is treated as a comment even if it 
# is in a string.
string(ASCII 35 HASH)

execute_process(COMMAND pkg-config --variable=glib_genmarshal glib-2.0 
	OUTPUT_VARIABLE GLIB_GENMARSHAL
	OUTPUT_STRIP_TRAILING_WHITESPACE)

macro(GENMARSHAL namespace)
	add_custom_command(OUTPUT ${namespace}-marshal.c
		COMMAND echo '${HASH}include \"${namespace}-marshal.h\"' > xgen-gmc
		COMMAND ${GLIB_GENMARSHAL} --prefix=_${namespace}_marshal ${namespace}-marshal.list --body >> xgen-gmc
		COMMAND cp xgen-gmc ${namespace}-marshal.c
		COMMAND rm xgen-gmc
		COMMENT "Generating ${namespace}-marshal.c"
		)
	add_custom_command(OUTPUT ${namespace}-marshal.h
		COMMAND ${GLIB_GENMARSHAL} --prefix=_${namespace}_marshal ${namespace}-marshal.list --header > ${namespace}-marshal.h
		COMMENT "Generating ${namespace}-marshal.h"
		)
endmacro(GENMARSHAL)


