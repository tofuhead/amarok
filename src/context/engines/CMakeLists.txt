add_subdirectory( current )
add_subdirectory( info )
add_subdirectory( labels )
add_subdirectory( lyrics )
add_subdirectory( photos )
add_subdirectory( tabs )
add_subdirectory( wikipedia )

if(LIBLASTFM_FOUND)
    add_subdirectory( similarartists )
    add_subdirectory( upcomingevents )
endif(LIBLASTFM_FOUND)
