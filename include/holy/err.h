/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ERR_HEADER
#define holy_ERR_HEADER	1

#include <holy/symbol.h>

#define holy_MAX_ERRMSG		256

typedef enum
  {
    holy_ERR_NONE = 0,
    holy_ERR_TEST_FAILURE,
    holy_ERR_BAD_MODULE,
    holy_ERR_OUT_OF_MEMORY,
    holy_ERR_BAD_FILE_TYPE,
    holy_ERR_FILE_NOT_FOUND,
    holy_ERR_FILE_READ_ERROR,
    holy_ERR_BAD_FILENAME,
    holy_ERR_UNKNOWN_FS,
    holy_ERR_BAD_FS,
    holy_ERR_BAD_NUMBER,
    holy_ERR_OUT_OF_RANGE,
    holy_ERR_UNKNOWN_DEVICE,
    holy_ERR_BAD_DEVICE,
    holy_ERR_READ_ERROR,
    holy_ERR_WRITE_ERROR,
    holy_ERR_UNKNOWN_COMMAND,
    holy_ERR_INVALID_COMMAND,
    holy_ERR_BAD_ARGUMENT,
    holy_ERR_BAD_PART_TABLE,
    holy_ERR_UNKNOWN_OS,
    holy_ERR_BAD_OS,
    holy_ERR_NO_KERNEL,
    holy_ERR_BAD_FONT,
    holy_ERR_NOT_IMPLEMENTED_YET,
    holy_ERR_SYMLINK_LOOP,
    holy_ERR_BAD_COMPRESSED_DATA,
    holy_ERR_MENU,
    holy_ERR_TIMEOUT,
    holy_ERR_IO,
    holy_ERR_ACCESS_DENIED,
    holy_ERR_EXTRACTOR,
    holy_ERR_NET_BAD_ADDRESS,
    holy_ERR_NET_ROUTE_LOOP,
    holy_ERR_NET_NO_ROUTE,
    holy_ERR_NET_NO_ANSWER,
    holy_ERR_NET_NO_CARD,
    holy_ERR_WAIT,
    holy_ERR_BUG,
    holy_ERR_NET_PORT_CLOSED,
    holy_ERR_NET_INVALID_RESPONSE,
    holy_ERR_NET_UNKNOWN_ERROR,
    holy_ERR_NET_PACKET_TOO_BIG,
    holy_ERR_NET_NO_DOMAIN,
    holy_ERR_EOF,
    holy_ERR_BAD_SIGNATURE
  }
holy_err_t;

struct holy_error_saved
{
  holy_err_t holy_errno;
  char errmsg[holy_MAX_ERRMSG];
};

extern holy_err_t EXPORT_VAR(holy_errno);
extern char EXPORT_VAR(holy_errmsg)[holy_MAX_ERRMSG];

holy_err_t EXPORT_FUNC(holy_error) (holy_err_t n, const char *fmt, ...);
void EXPORT_FUNC(holy_fatal) (const char *fmt, ...) __attribute__ ((noreturn));
void EXPORT_FUNC(holy_error_push) (void);
int EXPORT_FUNC(holy_error_pop) (void);
void EXPORT_FUNC(holy_print_error) (void);
extern int EXPORT_VAR(holy_err_printed_errors);
int holy_err_printf (const char *fmt, ...)
     __attribute__ ((format (__printf__, 1, 2)));

#endif /* ! holy_ERR_HEADER */
