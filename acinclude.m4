dnl
dnl  $Id$
dnl

AC_DEFUN([AC_PREFIX_BLUEZ], [
	AC_PREFIX_DEFAULT(/usr)

	if (test "${prefix}" = "NONE"); then
		dnl no prefix and no sysconfdir, so default to /etc
		if test "$sysconfdir" = '${prefix}/etc'; then
			AC_SUBST([sysconfdir], ['/etc'])
		fi

		dnl no prefix and no mandir, so use ${prefix}/share/man as default
		if test "$mandir" = '${prefix}/man'; then
			AC_SUBST([mandir], ['${prefix}/share/man'])
		fi

		prefix="${ac_default_prefix}"
	fi

	if (test "${libdir}" = "\${exec_prefix}/lib"); then
		libdir="${prefix}/lib"
	fi
])

AC_DEFUN([AC_PATH_BLUEZ], [
	AC_ARG_WITH(bluez, [  --with-bluez=DIR        BlueZ library is installed in DIR], [
		if (test "${withval}" = "yes"); then
			bluez_prefix=${prefix}
		else
			bluez_prefix=${withval}
		fi
	])

	ac_save_CPPFLAGS=$CPPFLAGS
	ac_save_LDFLAGS=$LDFLAGS

	BLUEZ_CFLAGS=""
	test -d "${bluez_prefix}/include" && BLUEZ_CFLAGS="$BLUEZ_CFLAGS -I${bluez_prefix}/include"

	CPPFLAGS="$CPPFLAGS $BLUEZ_CFLAGS"
	AC_CHECK_HEADER(bluetooth/bluetooth.h,, AC_MSG_ERROR(Bluetooth header files not found))

	BLUEZ_LIBS=""
	if (test "${prefix}" = "${bluez_prefix}"); then
		test -d "${libdir}" && BLUEZ_LIBS="$BLUEZ_LIBS -L${libdir}"
	else
		test -d "${bluez_prefix}/lib64" && BLUEZ_LIBS="$BLUEZ_LIBS -L${bluez_prefix}/lib64"
		test -d "${bluez_prefix}/lib" && BLUEZ_LIBS="$BLUEZ_LIBS -L${bluez_prefix}/lib"
	fi

	LDFLAGS="$LDFLAGS $BLUEZ_LIBS"
	AC_CHECK_LIB(bluetooth, hci_open_dev, BLUEZ_LIBS="$BLUEZ_LIBS -lbluetooth", AC_MSG_ERROR(Bluetooth library not found))
	AC_CHECK_LIB(sdp, sdp_connect, BLUEZ_LIBS="$BLUEZ_LIBS -lsdp")

	CPPFLAGS=$ac_save_CPPFLAGS
	LDFLAGS=$ac_save_LDFLAGS

	AC_SUBST(BLUEZ_CFLAGS)
	AC_SUBST(BLUEZ_LIBS)
])
