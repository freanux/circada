AC_INIT([circada], [0.4.1], [pw@earthwave.ch])

AM_INIT_AUTOMAKE
LT_INIT

AC_LANG(C++)
AC_PROG_CXX

AC_CONFIG_MACRO_DIR([m4])

CXXFLAGS="${CXXFLAGS} -pedantic"

# --- check for header file ---
AC_HEADER_DIRENT
AC_HEADER_STDC

# --- check for libraries ---
AC_CHECK_LIB([pthread], [pthread_create], [], [missing_libraries="$missing_libraries libpthread"])
AC_CHECK_LIB([ncursesw], [refresh], [], [missing_libraries="$missing_libraries libncursesw"])
AC_CHECK_LIB([gnutls], [gnutls_global_init], [], [missing_libraries="$missing_libraries libgnutls"])

AC_CHECK_HEADER([gnutls/gnutls.h], [], [$missing_headers gnutls/gnutls.h])
AC_CHECK_HEADER([gnutls/gnutlsxx.h], [], [$missing_headers gnutls/gnutlsxx.h])

# --- compile args ---
AC_ARG_ENABLE([build-library], [  --enable-build-library  Enable build and installing of circada shared library], [BUILD_LIBRARY="yes"], [])
AM_CONDITIONAL([BUILD_LIBRARY], [test "x$BUILD_LIBRARY" = xyes])

# --- bail out on errors ---
if test ! -z "$missing_libraries"; then
	AC_MSG_ERROR([missing libraries: $missing_libraries])
fi

if test ! -z "$missing_headers"; then
	AC_MSG_ERROR([missing header files: $missing_headers])
fi

# --- ready ---
AC_CONFIG_FILES([Makefile src/libcircada/Makefile src/libcircada/include/Makefile src/circada/Makefile])
AC_OUTPUT

# --- summary ---
AC_MSG_NOTICE([])
AC_MSG_NOTICE([--- SUMMARY ---])
if test "x$BUILD_LIBRARY" = "xyes"; then
    AC_MSG_NOTICE([ * circada library will be installed])
    AC_MSG_NOTICE([   NOTICES: - run ldconfig after installation])
    AC_MSG_NOTICE([            - you will find your header files in eg. /usr/local/include])
else
    AC_MSG_NOTICE([ * circada library will NOT be installed, linking statically])
fi
AC_MSG_NOTICE([])

