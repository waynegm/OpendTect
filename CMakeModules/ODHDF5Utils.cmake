#_______________________Pmake___________________________________________________
#
#       CopyRight:	dGB Beheer B.V.
#       July 2018	Bert/Arnaud
#_______________________________________________________________________________

macro( HDF5CLEANUP )
  unset( HDF5_DIFF_EXECUTABLE CACHE )
  unset( HDF5_DIR CACHE )
  unset( HDF5_IS_PARALLEL CACHE )
  list(APPEND COMPS C CXX )
  list(APPEND LIBSUFFIXES dl hdf5 hdf5_cpp m pthread z)
  foreach( COMP IN LISTS COMPS )
    unset( HDF5_${COMP}_COMPILER_EXECUTABLE CACHE )
    unset( HDF5_${COMP}_COMPILER_NO_INTERROGATE CACHE )
    unset( HDF5_${COMP}_INCLUDE_DIR CACHE )
    foreach( LIBSUFFIX IN LISTS LIBSUFFIXES )
      unset( HDF5_${COMP}_LIBRARY_${LIBSUFFIX} CACHE )
      unset( HDF5_${LIBSUFFIX}_LIBRARY_DEBUG CACHE )
      unset( HDF5_${LIBSUFFIX}_LIBRARY_RELEASE CACHE )
    endforeach()
  endforeach()
endmacro( HDF5CLEANUP )

macro( CHECKTARGETTYPE )
  set( LIBISTARGET TRUE )
  if ( WIN32 )
    foreach( LIB IN LISTS HDF5_CXX_LIBRARIES )
      if ( EXISTS "${LIB}" )
        set( LIBISTARGET FALSE )
      endif()
    endforeach()
  endif()
endmacro()

macro( GETHDF5COMPDEF )
  CHECKTARGETTYPE()
  if ( LIBISTARGET )
    get_target_property( HDF5_COMPILEDEF ${HDF5_CXX_LIBRARIES} INTERFACE_COMPILE_DEFINITIONS )
  endif()
  if ( NOT DEFINED HDF5_COMPILEDEF OR NOT HDF5_COMPILEDEF )
    set( HDF5_COMPILEDEF "H5_BUILT_AS_DYNAMIC_LIB" )
  endif()
endmacro( GETHDF5COMPDEF )

macro( GET_HDF5_ROOT )
  if ( HDF5_DIFF_EXECUTABLE )
    get_filename_component( hdf5_path ${HDF5_DIFF_EXECUTABLE} DIRECTORY )
    get_filename_component( hdf5_path ${hdf5_path} DIRECTORY )
  elseif ( HDF5_C_INCLUDE_DIR )
    get_filename_component( hdf5_path ${HDF5_C_INCLUDE_DIR} DIRECTORY )
  elseif( WIN32 )
    message( FATAL_ERROR "Cannot locate pre-defined HDF5 installation" )
  else()
    set( hdf5_path "/usr" )
  endif()
endmacro()

macro( SETHDF5DIR )
  if ( DEFINED HDF5_ROOT )
    if ( WIN32 AND EXISTS "${HDF5_ROOT}/cmake/hdf5" )
      set( HDF5_DIR "${HDF5_ROOT}/cmake/hdf5" )
    elseif( NOT WIN32 AND EXISTS "${HDF5_ROOT}/share/cmake/hdf5" )
      set( HDF5_DIR "${HDF5_ROOT}/share/cmake/hdf5" )
    endif()
  endif()
endmacro()

macro( OD_SETUP_HDF5 )

  if ( NOT DEFINED HDF5_ROOT )
    set( HDF5_ROOT "" CACHE PATH "HDF5 Location" )
    HDF5CLEANUP()
  elseif ( DEFINED HDF5_DIFF_EXECUTABLE OR DEFINED HDF5_C_INCLUDE_DIR )
    GET_HDF5_ROOT()
    if ( "${HDF5_ROOT}" STREQUAL "" )
      set( HDF5_ROOT "${hdf5_path}" CACHE PATH "HDF5 Location" FORCE )
      set( HDF5_FOUND TRUE )
    elseif ( NOT "${HDF5_ROOT}" STREQUAL "${hdf5_path}" )
      HDF5CLEANUP()
    endif()
  endif()

  if ( NOT HDF5_FOUND )
    SETHDF5DIR()
    find_package( HDF5 1.10 QUIET COMPONENTS C CXX )
    if ( HDF5_FOUND )
      GET_HDF5_ROOT()
      set( HDF5_ROOT "${hdf5_path}" CACHE PATH "HDF5 Location" FORCE )
      if ( NOT HDF5_DIR )
	SETHDF5DIR()
        HDF5CLEANUP()
	find_package( HDF5 1.10 QUIET COMPONENTS C CXX )
      endif()
      string( TOUPPER ${CMAKE_BUILD_TYPE} HDF5_BUILD_TYPE )
      if ( WIN32 )
	CHECKTARGETTYPE()
	if ( LIBISTARGET )
          get_target_property( HDF5_CXXLIB ${HDF5_CXX_LIBRARIES} IMPORTED_LOCATION_${HDF5_BUILD_TYPE} )
	  if ( HDF5_CXXLIB )
            get_filename_component( HDF5_RUNTIMEDIR ${HDF5_CXXLIB} DIRECTORY )
            list( APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH ${HDF5_RUNTIMEDIR} )
	  endif()
	elseif( HDF5_ROOT )
	  list( APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH "${HDF5_ROOT}/bin" )
	endif()
      endif()
      foreach( HDF5_LIB IN LISTS HDF5_LIBRARIES )
	if ( WIN32 )
          get_target_property( HDF5_LIB ${HDF5_LIB} IMPORTED_LOCATION_${HDF5_BUILD_TYPE} )
	  if ( NOT HDF5_LIB )
	    get_filename_component( LIBNM ${HDF5_LIB} NAME_WE )
	    set( HDF5_LIB "${HDF5_ROOT}/bin/${LIBNM}.dll" )
	  endif()
	endif()
	if ( HDF5_LIB AND EXISTS ${HDF5_LIB} )
	  list( APPEND OD_${OD_MODULE_NAME}_PLUGIN_EXTERNAL_DLL ${HDF5_LIB} )
	  if ( OD_INSTALL_DEPENDENT_LIBS )
	    OD_INSTALL_LIBRARY( ${HDF5_LIB} ${CMAKE_BUILD_TYPE} )
	  endif(OD_INSTALL_DEPENDENT_LIBS)
	endif()
      endforeach()
    else()
      HDF5CLEANUP()
    endif()
    if ( NOT HDF5_DIR )
      unset( HDF5_DIR CACHE )
    endif()
  endif()

endmacro( OD_SETUP_HDF5 )