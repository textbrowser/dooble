#include <stdio.h>
#include <stdlib.h>

#include "libspoton.h"

int main(void)
{
  libspoton_error_t rc = LIBSPOTON_ERROR_NONE;
  libspoton_handle_t libspotonHandle;

  if((rc = libspoton_init_b("shared.db",
			    "aes256",
			    "sha512",
			    "123",
			    "456",
			    3,
			    3,
			    10000,
			    &libspotonHandle,
			    65536)) != LIBSPOTON_ERROR_NONE)
    printf("libspoton_init_b() error (%s).\n", libspoton_strerror(rc));

  if((rc = libspoton_register_kernel(100,
				     false,
				     &libspotonHandle)) !=
     LIBSPOTON_ERROR_NONE)
    printf("libspoton_register_kernel() error (%s).\n",
	   libspoton_strerror(rc));

  libspoton_close(&libspotonHandle);

  if((rc = libspoton_init_b("shared.db",
			    "aes256",
			    "sha512",
			    "123",
			    "456",
			    3,
			    3,
			    10000,
			    &libspotonHandle, 65536)) !=
     LIBSPOTON_ERROR_NONE)
    printf("libspoton_init_b() error (%s).\n", libspoton_strerror(rc));

  const char *description = "Dooble";
  const char *title = "Dooble Web Browser";
  const char *url = "http://dooble.sourceforge.net";

  if((rc = libspoton_save_url(url,
			      strlen(url),
			      title,
			      strlen(title),
			      description,
			      strlen(description),
			      &libspotonHandle)) !=
     LIBSPOTON_ERROR_NONE)
    printf("libspoton_save_url() error (%s).\n",
	   libspoton_strerror(rc));

  url = "http://spot-on.sourceforge.net";

  if((rc = libspoton_save_url(url,
			      strlen(url),
			      "",
			      0,
			      0,
			      0,
			      &libspotonHandle)) !=
     LIBSPOTON_ERROR_NONE)
    printf("libspoton_save_url() error (%s).\n",
	   libspoton_strerror(rc));

  libspoton_close(&libspotonHandle);

  if((rc = libspoton_init_a("shared.db",
			    0,
			    0,
			    0,
			    &libspotonHandle, 65536)) !=
     LIBSPOTON_ERROR_NONE)
    printf("libspoton_init_a() error (%s).\n", libspoton_strerror(rc));

  description = "knowledge, science";
  title = "Wikipedia";
  url = "http://www.wikipedia.org";

  if((rc = libspoton_save_url(url,
			      strlen(url),
			      title,
			      strlen(title),
			      description,
			      strlen(description),
			      &libspotonHandle)) !=
     LIBSPOTON_ERROR_NONE)
    printf("libspoton_save_url() error (%s).\n",
	   libspoton_strerror(rc));

  if((rc = libspoton_deregister_kernel(100, &libspotonHandle)) !=
     LIBSPOTON_ERROR_NONE)
    printf("libspoton_deregister_kernel() error (%s).\n",
	   libspoton_strerror(rc));

  libspoton_close(&libspotonHandle);
  return EXIT_SUCCESS;
}
