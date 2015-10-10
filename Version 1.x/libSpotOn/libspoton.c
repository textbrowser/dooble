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

#ifndef LIBSPOTON_OS_WINDOWS
#include <arpa/inet.h>
#endif
#include "libspoton.h"

static bool gcryctl_set_thread_cbs_set = false;
static char *libspoton_error_strings[] =
  {
    "LIBSPOTON_ERROR_NONE",
    "LIBSPOTON_ERROR_CALLOC",
    "LIBSPOTON_ERROR_CALLOC_SECURE",
    "LIBSPOTON_ERROR_GCRY_CALLOC",
    "LIBSPOTON_ERROR_GCRY_CHECK_VERSION",
    "LIBSPOTON_ERROR_GCRY_CIPHER_ENCRYPT",
    "LIBSPOTON_ERROR_GCRY_CIPHER_GET_ALGO_BLKLEN",
    "LIBSPOTON_ERROR_GCRY_CIPHER_GET_ALGO_KEYLEN",
    "LIBSPOTON_ERROR_GCRY_CIPHER_MAP_NAME",
    "LIBSPOTON_ERROR_GCRY_CIPHER_OPEN",
    "LIBSPOTON_ERROR_GCRY_CIPHER_SETIV",
    "LIBSPOTON_ERROR_GCRY_CIPHER_SETKEY",
    "LIBSPOTON_ERROR_GCRY_CONTROL",
    "LIBSPOTON_ERROR_GCRY_KDF_DERIVE",
    "LIBSPOTON_ERROR_GCRY_MD_MAP_NAME",
    "LIBSPOTON_ERROR_INVALID_LENGTH",
    "LIBSPOTON_ERROR_INVALID_PARAMETER",
    "LIBSPOTON_ERROR_KERNEL_PROCESS_ALREADY_REGISTERED",
    "LIBSPOTON_ERROR_NOT_CONNECTED_TO_SQLITE_DATABASE",
    "LIBSPOTON_ERROR_NULL_LIBSPOTON_HANDLE",
    "LIBSPOTON_ERROR_NUMERIC_ERROR",
    "LIBSPOTON_ERROR_SQLITE_BIND_BLOB_DESCRIPTION",
    "LIBSPOTON_ERROR_SQLITE_BIND_BLOB_TITLE",
    "LIBSPOTON_ERROR_SQLITE_BIND_BLOB_URL",
    "LIBSPOTON_ERROR_SQLITE_BIND_INT",
    "LIBSPOTON_ERROR_SQLITE_BIND_INT64",
    "LIBSPOTON_ERROR_SQLITE_BIND_INT_ENCRYPT",
    "LIBSPOTON_ERROR_SQLITE_CREATE_KERNEL_REGISTRATION_TABLE",
    "LIBSPOTON_ERROR_SQLITE_CREATE_KERNEL_REGISTRATION_TRIGGER",
    "LIBSPOTON_ERROR_SQLITE_CREATE_URLS_TABLE",
    "LIBSPOTON_ERROR_SQLITE_DATABASE_LOCKED",
    "LIBSPOTON_ERROR_SQLITE_OPEN_V2",
    "LIBSPOTON_ERROR_SQLITE_PREPARE_V2",
    "LIBSPOTON_ERROR_SQLITE_STEP"
  };
static libspoton_error_t libspoton_error_maximum_error =
  LIBSPOTON_ERROR_SQLITE_STEP;
static pthread_mutex_t sqlite_mutex = PTHREAD_MUTEX_INITIALIZER;

#if !defined(GCRYPT_VERSION_NUMBER) || GCRYPT_VERSION_NUMBER < 0x010600
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

static libspoton_error_t initialize_libgcrypt
(const int secure_memory_pool_size)
{
  /*
  ** Initialize the gcrypt library if it has not yet been
  ** initialized.
  */

  libspoton_error_t rerr = LIBSPOTON_ERROR_NONE;

  if(!gcryctl_set_thread_cbs_set)
    {
      gcry_error_t err = 0;

#if !defined(GCRYPT_VERSION_NUMBER) || GCRYPT_VERSION_NUMBER < 0x010600
      err = gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread, 0);
#endif

      if(err == 0)
	gcryctl_set_thread_cbs_set = true;
      else
	fprintf(stderr, "libspoton::initialize_libgcrypt(): "
		"error initializing threads. Proceeding.\n");
    }

  if(!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P))
    {
      gcry_control(GCRYCTL_ENABLE_M_GUARD);

      if(!gcry_check_version(GCRYPT_VERSION))
	rerr = LIBSPOTON_ERROR_GCRY_CHECK_VERSION;
      else
	{
	  gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
#ifdef LIBSPOTON_IGNORE_GCRY_CONTROL_GCRYCTL_INIT_SECMEM_RETURN_VALUE
	  gcry_control(GCRYCTL_INIT_SECMEM, secure_memory_pool_size, 0);
#else
	  if(gcry_control(GCRYCTL_INIT_SECMEM,
			  secure_memory_pool_size, 0) != 0)
	    rerr = LIBSPOTON_ERROR_GCRY_CONTROL;
#endif
	  gcry_control(GCRYCTL_RESUME_SECMEM_WARN);
	  gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
	}
    }

  return rerr;
}

bool libspoton_is_kernel_registered(libspoton_handle_t *libspotonHandle,
				    libspoton_error_t *error)
{
  return libspoton_registered_kernel_pid(libspotonHandle, error) > 0;
}

const char *libspoton_strerror(const libspoton_error_t error)
{
  if(error > libspoton_error_maximum_error)
    return "";
  else
    return libspoton_error_strings[error];
}

libspoton_error_t libspoton_create_urls_table
(libspoton_handle_t *libspotonHandle)
{
  int rv = 0;
  libspoton_error_t rerr = LIBSPOTON_ERROR_NONE;

  if(!libspotonHandle)
    {
      rerr = LIBSPOTON_ERROR_NULL_LIBSPOTON_HANDLE;
      goto error_label;
    }
  else if(!libspotonHandle->m_sqliteHandle)
    {
      rerr = LIBSPOTON_ERROR_NOT_CONNECTED_TO_SQLITE_DATABASE;
      goto error_label;
    }

  pthread_mutex_lock(&sqlite_mutex);
  rv = sqlite3_exec(libspotonHandle->m_sqliteHandle,
		    "CREATE TABLE IF NOT EXISTS urls ("
		    "url BLOB PRIMARY KEY NOT NULL, "
		    "title BLOB, "
		    "description BLOB, "
		    "encrypted INTEGER NOT NULL DEFAULT 0)",
		    0,
		    0,
		    0);
  pthread_mutex_unlock(&sqlite_mutex);

  if(rv != SQLITE_OK)
    {
      rerr = LIBSPOTON_ERROR_SQLITE_CREATE_URLS_TABLE;
      goto error_label;
    }

 error_label:
  return rerr;
}

libspoton_error_t libspoton_delete_urls(libspoton_handle_t *libspotonHandle,
					const int encrypted)
{
  const char *sql = "DELETE FROM urls WHERE encrypted = ?";
  int rv = 0;
  libspoton_error_t rerr = LIBSPOTON_ERROR_NONE;
  sqlite3_stmt *stmt = 0;

  if(!libspotonHandle)
    {
      rerr = LIBSPOTON_ERROR_NULL_LIBSPOTON_HANDLE;
      goto error_label;
    }
  else if(!libspotonHandle->m_sqliteHandle)
    {
      rerr = LIBSPOTON_ERROR_NOT_CONNECTED_TO_SQLITE_DATABASE;
      goto error_label;
    }

  pthread_mutex_lock(&sqlite_mutex);
  rv = sqlite3_prepare_v2(libspotonHandle->m_sqliteHandle,
			  sql,
			  (int) strlen(sql),
			  &stmt,
			  0);
  pthread_mutex_unlock(&sqlite_mutex);

  if(rv != SQLITE_OK)
    {
      rerr = LIBSPOTON_ERROR_SQLITE_PREPARE_V2;
      goto error_label;
    }

  if(sqlite3_bind_int(stmt, 1, encrypted) != SQLITE_OK)
    {
      rerr = LIBSPOTON_ERROR_SQLITE_BIND_INT;
      goto error_label;
    }

  rv = sqlite3_step(stmt);

  if(!(rv == SQLITE_OK || rv == SQLITE_DONE))
    {
      rerr = LIBSPOTON_ERROR_SQLITE_STEP;
      goto error_label;
    }

 error_label:
  sqlite3_finalize(stmt);
  return rerr;
}

libspoton_error_t libspoton_deregister_kernel
(const pid_t pid, libspoton_handle_t *libspotonHandle)
{
  const char *sql = "DELETE FROM kernel_registration WHERE pid = ?";
  int rv = 0;
  libspoton_error_t rerr = LIBSPOTON_ERROR_NONE;
  sqlite3_stmt *stmt = 0;

  if(!libspotonHandle)
    {
      rerr = LIBSPOTON_ERROR_NULL_LIBSPOTON_HANDLE;
      goto error_label;
    }
  else if(!libspotonHandle->m_sqliteHandle)
    {
      rerr = LIBSPOTON_ERROR_NOT_CONNECTED_TO_SQLITE_DATABASE;
      goto error_label;
    }

  pthread_mutex_lock(&sqlite_mutex);
  rv = sqlite3_prepare_v2(libspotonHandle->m_sqliteHandle,
			  sql,
			  (int) strlen(sql),
			  &stmt,
			  0);
  pthread_mutex_unlock(&sqlite_mutex);

  if(rv != SQLITE_OK)
    {
      rerr = LIBSPOTON_ERROR_SQLITE_PREPARE_V2;
      goto error_label;
    }

  if(sqlite3_bind_int64(stmt, 1, pid) != SQLITE_OK)
    {
      rerr = LIBSPOTON_ERROR_SQLITE_BIND_INT64;
      goto error_label;
    }

  rv = sqlite3_step(stmt);

  if(!(rv == SQLITE_OK || rv == SQLITE_DONE))
    {
      rerr = LIBSPOTON_ERROR_SQLITE_STEP;
      goto error_label;
    }

 error_label:
  sqlite3_finalize(stmt);
  return rerr;
}

libspoton_error_t libspoton_init_a(const char *databasePath,
				   const char *cipherType,
				   const char *key,
				   const size_t keyLength,
				   libspoton_handle_t *libspotonHandle,
				   const int secure_memory_pool_size)
{
  int rv = 0;
  libspoton_error_t rerr = LIBSPOTON_ERROR_NONE;

  if(!libspotonHandle)
    {
      rerr = LIBSPOTON_ERROR_NULL_LIBSPOTON_HANDLE;
      goto error_label;
    }

  libspotonHandle->m_cipherAlgorithm = 0;
  libspotonHandle->m_key = 0;
  libspotonHandle->m_keyLength = 0;
  libspotonHandle->m_sqliteHandle = 0;

  /*
  ** Initialize libgcrypt.
  */

  if((rerr = initialize_libgcrypt(secure_memory_pool_size)) !=
     LIBSPOTON_ERROR_NONE)
    goto error_label;

  if(key && keyLength > 0)
    {
      if((libspotonHandle->m_cipherAlgorithm =
	  gcry_cipher_map_name(cipherType)) == 0)
	{
	  rerr = LIBSPOTON_ERROR_GCRY_CIPHER_MAP_NAME;
	  goto error_label;
	}

      libspotonHandle->m_keyLength = keyLength;

      if(!(libspotonHandle->m_key =
	   gcry_calloc_secure(libspotonHandle->m_keyLength,
			      sizeof(char))))
	{
	  rerr = LIBSPOTON_ERROR_GCRY_CALLOC_SECURE;
	  goto error_label;
	}
      else
	memcpy(libspotonHandle->m_key, key, keyLength);
    }

  /*
  ** Create the shared.db database.
  */

  pthread_mutex_lock(&sqlite_mutex);
  rv = sqlite3_open_v2(databasePath,
		       &libspotonHandle->m_sqliteHandle,
		       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
		       0);
  pthread_mutex_unlock(&sqlite_mutex);

  if(rv != SQLITE_OK)
    {
      rerr = LIBSPOTON_ERROR_SQLITE_OPEN_V2;
      goto error_label;
    }

 error_label:

  if(rerr != LIBSPOTON_ERROR_NONE)
    libspoton_close(libspotonHandle);

  return rerr;
}

libspoton_error_t libspoton_init_b(const char *databasePath,
				   const char *cipherType,
				   const char *hashType,
				   const char *passphrase,
				   const char *salt,
				   const size_t passphraseLength,
				   const size_t saltLength,
				   const unsigned long iterationCount,
				   libspoton_handle_t *libspotonHandle,
				   const int secure_memory_pool_size)
{
  int hashAlgorithm = 0;
  int rv = 0;
  libspoton_error_t rerr = LIBSPOTON_ERROR_NONE;

  if(!libspotonHandle)
    {
      rerr = LIBSPOTON_ERROR_NULL_LIBSPOTON_HANDLE;
      goto error_label;
    }

  libspotonHandle->m_cipherAlgorithm = 0;
  libspotonHandle->m_key = 0;
  libspotonHandle->m_keyLength = 0;
  libspotonHandle->m_sqliteHandle = 0;

  /*
  ** Initialize libgcrypt.
  */

  if((rerr =
      initialize_libgcrypt(secure_memory_pool_size)) != LIBSPOTON_ERROR_NONE)
    goto error_label;

  if(cipherType && hashType && iterationCount > 0 &&
     passphrase && salt && passphraseLength > 0 && saltLength > 0)
    {
      if((libspotonHandle->m_cipherAlgorithm =
	  gcry_cipher_map_name(cipherType)) == 0)
	{
	  rerr = LIBSPOTON_ERROR_GCRY_CIPHER_MAP_NAME;
	  goto error_label;
	}

      if((hashAlgorithm = gcry_md_map_name(hashType)) == 0)
	{
	  rerr = LIBSPOTON_ERROR_GCRY_MD_MAP_NAME;
	  goto error_label;
	}

      if((libspotonHandle->m_keyLength =
	  gcry_cipher_get_algo_keylen(libspotonHandle->
				      m_cipherAlgorithm)) == 0)
	{
	  rerr = LIBSPOTON_ERROR_GCRY_CIPHER_GET_ALGO_KEYLEN;
	  goto error_label;
	}

      if(!(libspotonHandle->m_key =
	   gcry_calloc_secure(libspotonHandle->m_keyLength,
			      sizeof(char))))
	{
	  rerr = LIBSPOTON_ERROR_GCRY_CALLOC_SECURE;
	  goto error_label;
	}

      gcry_fast_random_poll();

      if(gcry_kdf_derive((const void *) passphrase,
			 passphraseLength,
			 GCRY_KDF_PBKDF2,
			 hashAlgorithm,
			 (const void *) salt,
			 saltLength,
			 iterationCount,
			 libspotonHandle->m_keyLength,
			 (void *) libspotonHandle->m_key) != 0)
	{
	  rerr = LIBSPOTON_ERROR_GCRY_KDF_DERIVE;
	  goto error_label;
	}
    }

  /*
  ** Create the shared.db database.
  */

  pthread_mutex_lock(&sqlite_mutex);
  rv = sqlite3_open_v2(databasePath,
		       &libspotonHandle->m_sqliteHandle,
		       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
		       0);
  pthread_mutex_unlock(&sqlite_mutex);

  if(rv != SQLITE_OK)
    {
      rerr = LIBSPOTON_ERROR_SQLITE_OPEN_V2;
      goto error_label;
    }

 error_label:

  if(rerr != LIBSPOTON_ERROR_NONE)
    libspoton_close(libspotonHandle);

  return rerr;
}

libspoton_error_t libspoton_register_kernel
(const pid_t pid,
 const bool forceRegistration,
 libspoton_handle_t *libspotonHandle)
{
  const char *sql = "INSERT OR REPLACE INTO kernel_registration (pid) "
    "VALUES (?)";
  int rv = 0;
  libspoton_error_t rerr = LIBSPOTON_ERROR_NONE;
  sqlite3_stmt *stmt = 0;

  if(!libspotonHandle)
    {
      rerr = LIBSPOTON_ERROR_NULL_LIBSPOTON_HANDLE;
      goto error_label;
    }
  else if(!libspotonHandle->m_sqliteHandle)
    {
      rerr = LIBSPOTON_ERROR_NOT_CONNECTED_TO_SQLITE_DATABASE;
      goto error_label;
    }

  pthread_mutex_lock(&sqlite_mutex);
  rv = sqlite3_exec(libspotonHandle->m_sqliteHandle,
		    "CREATE TABLE IF NOT EXISTS kernel_registration ("
		    "pid INTEGER PRIMARY KEY NOT NULL)",
		    0,
		    0,
		    0);
  pthread_mutex_unlock(&sqlite_mutex);

  if(rv != SQLITE_OK)
    {
      rerr = LIBSPOTON_ERROR_SQLITE_CREATE_KERNEL_REGISTRATION_TABLE;
      goto error_label;
    }

  /*
  ** The kernel_registration table must contain only one entry.
  */

  pthread_mutex_lock(&sqlite_mutex);
  rv = sqlite3_exec(libspotonHandle->m_sqliteHandle,
		    "CREATE TRIGGER IF NOT EXISTS kernel_registration_trigger "
		    "BEFORE INSERT ON kernel_registration "
		    "BEGIN "
		    "DELETE FROM kernel_registration; "
		    "END",
		    0,
		    0,
		    0);
  pthread_mutex_unlock(&sqlite_mutex);

  if(rv != SQLITE_OK)
    {
      rerr = LIBSPOTON_ERROR_SQLITE_CREATE_KERNEL_REGISTRATION_TRIGGER;
      goto error_label;
    }

  if(libspoton_registered_kernel_pid(libspotonHandle, &rerr) > 0)
    if(!forceRegistration)
      {
	rerr = LIBSPOTON_ERROR_KERNEL_PROCESS_ALREADY_REGISTERED;
	goto error_label;
      }

  if(rerr != LIBSPOTON_ERROR_NONE)
    goto error_label;

  pthread_mutex_lock(&sqlite_mutex);
  rv = sqlite3_prepare_v2(libspotonHandle->m_sqliteHandle,
			  sql,
			  (int) strlen(sql),
			  &stmt,
			  0);
  pthread_mutex_unlock(&sqlite_mutex);

  if(rv != SQLITE_OK)
    {
      rerr = LIBSPOTON_ERROR_SQLITE_PREPARE_V2;
      goto error_label;
    }

  if(sqlite3_bind_int64(stmt, 1, pid) != SQLITE_OK)
    {
      rerr = LIBSPOTON_ERROR_SQLITE_BIND_INT64;
      goto error_label;
    }

  rv = sqlite3_step(stmt);

  if(!(rv == SQLITE_OK || rv == SQLITE_DONE))
    {
      rerr = LIBSPOTON_ERROR_SQLITE_STEP;
      goto error_label;
    }

 error_label:
  sqlite3_finalize(stmt);
  return rerr;
}

libspoton_error_t libspoton_save_url(const char *url,
				     const size_t urlSize,
				     const char *title,
				     const size_t titleSize,
				     const char *description,
				     const size_t descriptionSize,
				     libspoton_handle_t *libspotonHandle)
{
  bool encrypt = true;
  char *encodedBuffer = 0;
  char *encodedBufferAndIV = 0;
  char *iv = 0;
  char lengthArray[4];
  const char *buffer = "";
  const char *sql = "INSERT OR REPLACE INTO urls (url, title, "
    "description, encrypted) VALUES (?, ?, ?, ?)";
  gcry_cipher_hd_t cipherCtx = 0;
  int i = 0;
  int rv = 0;
  libspoton_error_t rerr = LIBSPOTON_ERROR_NONE;
  size_t blockLength = 0;
  size_t encodedBufferAndIVLength = 0;
  size_t encodedBufferLength = 0;
  size_t length = 0;
  size_t sizeofchar = sizeof(char);
  sqlite3_stmt *stmt = 0;

  if(!libspotonHandle)
    {
      rerr = LIBSPOTON_ERROR_NULL_LIBSPOTON_HANDLE;
      goto error_label;
    }
  else if(!libspotonHandle->m_sqliteHandle)
    {
      rerr = LIBSPOTON_ERROR_NOT_CONNECTED_TO_SQLITE_DATABASE;
      goto error_label;
    }

  if(!libspotonHandle->m_cipherAlgorithm ||
     !libspotonHandle->m_key || !libspotonHandle->m_keyLength)
    encrypt = false;

  if(encrypt)
    if(gcry_cipher_open(&cipherCtx, libspotonHandle->m_cipherAlgorithm,
			GCRY_CIPHER_MODE_CBC,
			GCRY_CIPHER_CBC_CTS |
			GCRY_CIPHER_SECURE) != 0 || !cipherCtx)
      {
	rerr = LIBSPOTON_ERROR_GCRY_CIPHER_OPEN;
	goto error_label;
      }

  if(encrypt)
    {
      if(gcry_cipher_setkey(cipherCtx,
			    libspotonHandle->m_key,
			    libspotonHandle->m_keyLength) != 0)
	{
	  rerr = LIBSPOTON_ERROR_GCRY_CIPHER_SETKEY;
	  goto error_label;
	}
    }

  if(encrypt)
    if((blockLength = gcry_cipher_get_algo_blklen(libspotonHandle->
						  m_cipherAlgorithm)) == 0)
      {
	rerr = LIBSPOTON_ERROR_GCRY_CIPHER_GET_ALGO_BLKLEN;
	goto error_label;
      }

  rv = libspoton_create_urls_table(libspotonHandle);

  if(rv != SQLITE_OK)
    {
      rerr = LIBSPOTON_ERROR_SQLITE_CREATE_URLS_TABLE;
      goto error_label;
    }

  pthread_mutex_lock(&sqlite_mutex);
  rv = sqlite3_prepare_v2(libspotonHandle->m_sqliteHandle,
			  sql,
			  (int) strlen(sql),
			  &stmt,
			  0);
  pthread_mutex_unlock(&sqlite_mutex);

  if(rv != SQLITE_OK)
    {
      rerr = LIBSPOTON_ERROR_SQLITE_PREPARE_V2;
      goto error_label;
    }

  for(i = 1; i <= 3; i++)
    {
      if(encrypt)
	{
	  iv = gcry_calloc(blockLength, sizeof(char));

	  if(iv)
	    {
	      gcry_cipher_reset(cipherCtx);
	      gcry_fast_random_poll();
	      gcry_create_nonce(iv, blockLength);

	      if(gcry_cipher_setiv(cipherCtx, iv, blockLength) != 0)
		{
		  rerr = LIBSPOTON_ERROR_GCRY_CIPHER_SETIV;
		  goto error_label;
		}
	    }
	  else
	    {
	      rerr = LIBSPOTON_ERROR_GCRY_CALLOC;
	      goto error_label;
	    }
	}

      if(i == 1)
	{
	  buffer = url;
	  length = urlSize;
	}
      else if(i == 2)
	{
	  buffer = title;
	  length = titleSize;
	}
      else
	{
	  buffer = description;
	  length = descriptionSize;
	}

      if(encrypt)
	{
	  if(length < blockLength)
	    encodedBufferLength = blockLength;
	  else
	    encodedBufferLength = length;

	  if(SIZE_MAX - encodedBufferLength >= 4)
	    encodedBufferLength += 4; /*
				      ** We'll append the length of the
				      ** original buffer.
				      */
	  else
	    {
	      rerr = LIBSPOTON_ERROR_NUMERIC_ERROR;
	      goto error_label;
	    }

	  if(sizeofchar > 0 && encodedBufferLength <= SIZE_MAX / sizeofchar)
	    encodedBuffer = calloc(encodedBufferLength, sizeof(char));
	  else
	    encodedBuffer = 0;

	  if(!encodedBuffer)
	    {
	      rerr = LIBSPOTON_ERROR_CALLOC;
	      goto error_label;
	    }
	  else
	    memcpy(encodedBuffer, buffer, length);

	  /*
	  ** Set the last four bytes to the length of the buffer. QDataStream
	  ** objects will retrieve the length of the original message.
	  */

#ifdef LIBSPOTON_OS_WINDOWS
	  lengthArray[3] = (char) (length & 0xff);
	  lengthArray[2] = (char) ((length >> 8)  & 0xff);
	  lengthArray[1] = (char) ((length >> 16) & 0xff);
	  lengthArray[0] = (char) ((length >> 24) & 0xff);
	  encodedBuffer[encodedBufferLength - 4] = lengthArray[0];
	  encodedBuffer[encodedBufferLength - 3] = lengthArray[1];
	  encodedBuffer[encodedBufferLength - 2] = lengthArray[2];
	  encodedBuffer[encodedBufferLength - 1] = lengthArray[3];
#else
	  length = (size_t) htonl((uint32_t) length);
	  memcpy(lengthArray, &length, 4);
	  memcpy(&encodedBuffer[encodedBufferLength - 4], lengthArray, 4);
#endif
	  gcry_fast_random_poll();

	  if(gcry_cipher_encrypt(cipherCtx,
				 encodedBuffer, encodedBufferLength,
				 0, 0) == 0)
	    {
	      if(SIZE_MAX - blockLength >= encodedBufferLength)
		encodedBufferAndIVLength = blockLength + encodedBufferLength;
	      else
		{
		  rerr = LIBSPOTON_ERROR_NUMERIC_ERROR;
		  goto error_label;
		}

	      if(sizeofchar > 0 &&
		 encodedBufferAndIVLength <= SIZE_MAX / sizeofchar)
		encodedBufferAndIV = calloc(encodedBufferAndIVLength,
					    sizeof(char));
	      else
		encodedBufferAndIV = 0;

	      if(encodedBufferAndIV)
		{
		  memcpy(encodedBufferAndIV, iv, blockLength);
		  memcpy(&encodedBufferAndIV[blockLength], encodedBuffer,
			 encodedBufferLength);
		}
	      else
		{
		  rerr = LIBSPOTON_ERROR_CALLOC;
		  goto error_label;
		}
	    }
	  else
	    {
	      rerr = LIBSPOTON_ERROR_GCRY_CIPHER_ENCRYPT;
	      goto error_label;
	    }
	}

      if(encrypt)
	{
	  if(sqlite3_bind_blob(stmt,
			       i,
			       encodedBufferAndIV,
			       (int) encodedBufferAndIVLength,
			       SQLITE_TRANSIENT) != SQLITE_OK)
	    {
	      if(i == 1)
		rerr = LIBSPOTON_ERROR_SQLITE_BIND_BLOB_URL;
	      else if(i == 2)
		rerr = LIBSPOTON_ERROR_SQLITE_BIND_BLOB_TITLE;
	      else
		rerr = LIBSPOTON_ERROR_SQLITE_BIND_BLOB_DESCRIPTION;

	      goto error_label;
	    }
	}
      else
	{
	  if(sqlite3_bind_blob(stmt,
			       i,
			       buffer,
			       (int) length,
			       SQLITE_TRANSIENT) != SQLITE_OK)
	    {
	      if(i == 1)
		rerr = LIBSPOTON_ERROR_SQLITE_BIND_BLOB_URL;
	      else if(i == 2)
		rerr = LIBSPOTON_ERROR_SQLITE_BIND_BLOB_TITLE;
	      else
		rerr = LIBSPOTON_ERROR_SQLITE_BIND_BLOB_DESCRIPTION;

	      goto error_label;
	    }
	}

      free(encodedBuffer);
      encodedBuffer = 0;
      free(encodedBufferAndIV);
      encodedBufferAndIV = 0;
      gcry_free(iv);
      iv = 0;
    }

  if(sqlite3_bind_int(stmt,
		      4,
		      encrypt) != SQLITE_OK)
    {
      rerr = LIBSPOTON_ERROR_SQLITE_BIND_INT_ENCRYPT;
      goto error_label;
    }

  rv = sqlite3_step(stmt);

  if(!(rv == SQLITE_OK || rv == SQLITE_DONE))
    {
      rerr = LIBSPOTON_ERROR_SQLITE_STEP;
      goto error_label;
    }

  sqlite3_finalize(stmt);
  stmt = 0;

 error_label:
  free(encodedBuffer);
  free(encodedBufferAndIV);
  gcry_cipher_close(cipherCtx);
  gcry_free(iv);
  sqlite3_finalize(stmt);
  return rerr;
}

pid_t libspoton_registered_kernel_pid(libspoton_handle_t *libspotonHandle,
				      libspoton_error_t *error)
{
  int rv = 0;
  const char *sql = "SELECT pid FROM kernel_registration LIMIT 1";
  sqlite3_stmt *stmt = 0;
  sqlite3_int64 pid = 0;

  if(!libspotonHandle)
    goto error_label;
  else if(!libspotonHandle->m_sqliteHandle)
    goto error_label;

  pthread_mutex_lock(&sqlite_mutex);
  rv = sqlite3_prepare_v2(libspotonHandle->m_sqliteHandle, sql, -1, &stmt, 0);
  pthread_mutex_unlock(&sqlite_mutex);

  if(rv == SQLITE_OK)
    {
      if((rv = sqlite3_step(stmt)) == SQLITE_ROW)
	pid = sqlite3_column_int64(stmt, 0);
      else if(error)
	{
	  if(rv == SQLITE_BUSY || rv == SQLITE_LOCKED)
	    *error = LIBSPOTON_ERROR_SQLITE_DATABASE_LOCKED;
	  else if(!(rv == SQLITE_OK || rv == SQLITE_DONE))
	    *error = LIBSPOTON_ERROR_SQLITE_STEP;
	}
    }
  else if(error)
    {
      if(rv == SQLITE_BUSY || rv == SQLITE_LOCKED)
	*error = LIBSPOTON_ERROR_SQLITE_DATABASE_LOCKED;
      else
	*error = LIBSPOTON_ERROR_SQLITE_PREPARE_V2;
    }

  sqlite3_finalize(stmt);

 error_label:
  return (pid_t) pid;
}

void libspoton_close(libspoton_handle_t *libspotonHandle)
{
  if(libspotonHandle)
    {
      gcry_free(libspotonHandle->m_key);
      pthread_mutex_lock(&sqlite_mutex);
      sqlite3_close(libspotonHandle->m_sqliteHandle);
      pthread_mutex_unlock(&sqlite_mutex);
      libspotonHandle->m_cipherAlgorithm = 0;
      libspotonHandle->m_key = 0;
      libspotonHandle->m_keyLength = 0;
      libspotonHandle->m_sqliteHandle = 0;
    }
}

void libspoton_enable_sqlite_cache(void)
{
#ifndef LIBSPOTON_OS_MAC
#ifndef LIBSPOTON_OS_OS2
  sqlite3_enable_shared_cache(1);
#endif
#endif
}
