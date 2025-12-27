#ifndef CONFIG_H_CMAKE
#define CONFIG_H_CMAKE

#cmakedefine DARWIN
#cmakedefine HAVE_ASSERT_H
#cmakedefine HAVE_ATEXIT
#cmakedefine HAVE_DLFCN_H
#cmakedefine HAVE_FCNTL_H
#cmakedefine HAVE_GETTIMEOFDAY
#cmakedefine HAVE_INTTYPES_H
#cmakedefine HAVE_MEMSET
#cmakedefine HAVE_NETDB_H
#cmakedefine HAVE_NEW_LIBLO
#cmakedefine HAVE_SELECT
#cmakedefine HAVE_SOCKET
#cmakedefine HAVE_STDBOOL_H
#cmakedefine HAVE_STDINT_H
#cmakedefine HAVE_STDIO_H
#cmakedefine HAVE_STDLIB_H
#cmakedefine HAVE_STRERROR
#cmakedefine HAVE_STRINGS_H
#cmakedefine HAVE_STRING_H
#cmakedefine HAVE_SYS_IOCTL_H
#cmakedefine HAVE_SYS_SOCKET_H
#cmakedefine HAVE_SYS_STAT_H
#cmakedefine HAVE_SYS_TYPES_H
#cmakedefine HAVE_UNISTD_H
#cmakedefine HAVE__BOOL
#cmakedefine LIBV4L2
#cmakedefine V4L2

#define PACKAGE "@PROJECT_NAME@"
#define PACKAGE_BUGREPORT "@PACKAGE_BUGREPORT@"
#define PACKAGE_NAME "@PACKAGE_NAME@"
#define PACKAGE_STRING "@PACKAGE_NAME@ @PROJECT_VERSION@"
#define PACKAGE_TARNAME "@PACKAGE_TARNAME@"
#define PACKAGE_URL "@PACKAGE_URL@"
#define PACKAGE_VERSION "@PROJECT_VERSION@"
#define VERSION "@PROJECT_VERSION@"

#endif
