/*
** Copyright (c) 2011 - 10^10^10, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from Spot-On without specific prior written permission.
**
** SPOT-ON IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** SPOT-ON, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef LIBSPOTON_H
#define LIBSPOTON_H

#define LIBSPOTON_VERSION 0x000101
#define LIBSPOTON_VERSION_STR "0.1.1"

#ifdef LIBSPOTON_OS_WINDOWS
#include "errno.h"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "gcrypt.h"
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include "pthread.h"
#include "sqlite3.h"
#else
#include <errno.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gcrypt.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include <pthread.h>
#include <sqlite3.h>
#endif
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
  {
    LIBSPOTON_ERROR_NONE = 0,
    LIBSPOTON_ERROR_CALLOC,
    LIBSPOTON_ERROR_GCRY_CALLOC,
    LIBSPOTON_ERROR_GCRY_CALLOC_SECURE,
    LIBSPOTON_ERROR_GCRY_CHECK_VERSION,
    LIBSPOTON_ERROR_GCRY_CIPHER_ENCRYPT,
    LIBSPOTON_ERROR_GCRY_CIPHER_GET_ALGO_BLKLEN,
    LIBSPOTON_ERROR_GCRY_CIPHER_GET_ALGO_KEYLEN,
    LIBSPOTON_ERROR_GCRY_CIPHER_MAP_NAME,
    LIBSPOTON_ERROR_GCRY_CIPHER_OPEN,
    LIBSPOTON_ERROR_GCRY_CIPHER_SETIV,
    LIBSPOTON_ERROR_GCRY_CIPHER_SETKEY,
    LIBSPOTON_ERROR_GCRY_CONTROL,
    LIBSPOTON_ERROR_GCRY_KDF_DERIVE,
    LIBSPOTON_ERROR_GCRY_MD_MAP_NAME,
    LIBSPOTON_ERROR_INVALID_LENGTH,
    LIBSPOTON_ERROR_INVALID_PARAMETER,
    LIBSPOTON_ERROR_KERNEL_PROCESS_ALREADY_REGISTERED,
    LIBSPOTON_ERROR_NOT_CONNECTED_TO_SQLITE_DATABASE,
    LIBSPOTON_ERROR_NULL_LIBSPOTON_HANDLE,
    LIBSPOTON_ERROR_NUMERIC_ERROR,
    LIBSPOTON_ERROR_SQLITE_BIND_BLOB_DESCRIPTION,
    LIBSPOTON_ERROR_SQLITE_BIND_BLOB_TITLE,
    LIBSPOTON_ERROR_SQLITE_BIND_BLOB_URL,
    LIBSPOTON_ERROR_SQLITE_BIND_INT,
    LIBSPOTON_ERROR_SQLITE_BIND_INT64,
    LIBSPOTON_ERROR_SQLITE_BIND_INT_ENCRYPT,
    LIBSPOTON_ERROR_SQLITE_CREATE_KERNEL_REGISTRATION_TABLE,
    LIBSPOTON_ERROR_SQLITE_CREATE_KERNEL_REGISTRATION_TRIGGER,
    LIBSPOTON_ERROR_SQLITE_CREATE_URLS_TABLE,
    LIBSPOTON_ERROR_SQLITE_DATABASE_LOCKED,
    LIBSPOTON_ERROR_SQLITE_OPEN_V2,
    LIBSPOTON_ERROR_SQLITE_PREPARE_V2,
    LIBSPOTON_ERROR_SQLITE_STEP
  }
libspoton_error_code_t;

struct libspoton_handle_struct_t
{
  char *m_key;
  int m_cipherAlgorithm;
  size_t m_keyLength;
  sqlite3 *m_sqliteHandle;
};

typedef libspoton_error_code_t libspoton_error_t;
typedef struct libspoton_handle_struct_t libspoton_handle_t;

/*
** Is a kernel process registered?
*/

bool libspoton_is_kernel_registered(libspoton_handle_t *libspotonHandle,
				    libspoton_error_t *error);

/*
** Return a user-friendly string representation of error.
*/

const char *libspoton_strerror(const libspoton_error_t error);

/*
** Delete URLs.
*/

libspoton_error_t libspoton_delete_urls(libspoton_handle_t *libspotonHandle,
					const int encrypted);

/*
** Deregister the kernel process.
*/

libspoton_error_t libspoton_deregister_kernel
(const pid_t pid, libspoton_handle_t *libspotonHandle);

/*
** Create shared.db. Initialize containers.
*/

libspoton_error_t libspoton_init_a(const char *databasePath,
				   const char *cipherType,
				   const char *key,
				   const size_t keyLength,
				   libspoton_handle_t *libspotonHandle,
				   const int secure_memory_pool_size);
libspoton_error_t libspoton_init_b(const char *databasePath,
				   const char *cipherType,
				   const char *hashType,
				   const char *passphrase,
				   const char *salt,
				   const size_t passphraseLength,
				   const size_t saltLength,
				   const unsigned long iterationCount,
				   libspoton_handle_t *libspotonHandle,
				   const int secure_memory_pool_size);

/*
** Register a kernel process.
*/

libspoton_error_t libspoton_register_kernel
(const pid_t pid,
 const bool forceRegistration,
 libspoton_handle_t *libspotonHandle);

/*
** Encode the description, title, and url via cipherType and place the
** encoded values into the urls table. The url field is required while
** title and description may be 0. Please note that the passphrase must
** agree with the passphrase understood by Spot-On.
*/

libspoton_error_t libspoton_save_url(const char *url,
				     const size_t urlSize,
				     const char *title,
				     const size_t titleSize,
				     const char *description,
				     const size_t descriptionSize,
				     libspoton_handle_t *libspotonHandle);

/*
** Retrieve the registered kernel's PID.
*/

pid_t libspoton_registered_kernel_pid(libspoton_handle_t *libspotonHandle,
				      libspoton_error_t *error);

/*
** Release resources and reset important containers to zero.
*/

void libspoton_close(libspoton_handle_t *libspotonHandle);
void libspoton_enable_sqlite_cache(void);

#ifdef __cplusplus
}
#endif
#endif
