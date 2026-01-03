/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config-util.h>
#include <config.h>

#include <holy/misc.h>
#include <unistd.h>
#include <holy/emu/exec.h>
#include <holy/emu/hostdisk.h>
#include <holy/emu/getroot.h>
#include <holy/util/misc.h>
#include <holy/disk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

int
holy_util_exec_redirect_all (const char *const *argv, const char *stdin_file,
			     const char *stdout_file, const char *stderr_file)
{
  pid_t pid;
  int status = -1;
  char *str, *pstr;
  const char *const *ptr;
  holy_size_t strl = 0;
  for (ptr = argv; *ptr; ptr++)
    strl += holy_strlen (*ptr) + 1;
  if (stdin_file)
    strl += holy_strlen (stdin_file) + 2;
  if (stdout_file)
    strl += holy_strlen (stdout_file) + 2;
  if (stderr_file)
    strl += holy_strlen (stderr_file) + 3;

  pstr = str = xmalloc (strl);
  for (ptr = argv; *ptr; ptr++)
    {
      pstr = holy_stpcpy (pstr, *ptr);
      *pstr++ = ' ';
    }
  if (stdin_file)
    {
      *pstr++ = '<';
      pstr = holy_stpcpy (pstr, stdin_file);
      *pstr++ = ' ';
    }
  if (stdout_file)
    {
      *pstr++ = '>';
      pstr = holy_stpcpy (pstr, stdout_file);
      *pstr++ = ' ';
    }
  if (stderr_file)
    {
      *pstr++ = '2';
      *pstr++ = '>';
      pstr = holy_stpcpy (pstr, stderr_file);
      pstr++;
    }
  *--pstr = '\0';

  holy_util_info ("executing %s", str);
  holy_free (str);

  pid = fork ();
  if (pid < 0)
    holy_util_error (_("Unable to fork: %s"), strerror (errno));
  else if (pid == 0)
    {
      int fd;
      /* Child.  */
      
      /* Close fd's.  */
#ifdef holy_UTIL
      holy_util_devmapper_cleanup ();
      holy_diskfilter_fini ();
#endif

      if (stdin_file)
	{
	  fd = open (stdin_file, O_RDONLY);
	  if (fd < 0)
	    exit (127);
	  dup2 (fd, STDIN_FILENO);
	  close (fd);
	}

      if (stdout_file)
	{
	  fd = open (stdout_file, O_WRONLY | O_CREAT, 0700);
	  if (fd < 0)
	    exit (127);
	  dup2 (fd, STDOUT_FILENO);
	  close (fd);
	}

      if (stderr_file)
	{
	  fd = open (stderr_file, O_WRONLY | O_CREAT, 0700);
	  if (fd < 0)
	    exit (127);
	  dup2 (fd, STDERR_FILENO);
	  close (fd);
	}

      /* Ensure child is not localised.  */
      setenv ("LC_ALL", "C", 1);

      execvp ((char *) argv[0], (char **) argv);
      exit (127);
    }
  waitpid (pid, &status, 0);
  if (!WIFEXITED (status))
    return -1;
  return WEXITSTATUS (status);
}

int
holy_util_exec (const char *const *argv)
{
  return holy_util_exec_redirect_all (argv, NULL, NULL, NULL);
}

int
holy_util_exec_redirect (const char *const *argv, const char *stdin_file,
			 const char *stdout_file)
{
  return holy_util_exec_redirect_all (argv, stdin_file, stdout_file, NULL);
}

int
holy_util_exec_redirect_null (const char *const *argv)
{
  return holy_util_exec_redirect_all (argv, "/dev/null", "/dev/null", NULL);
}

pid_t
holy_util_exec_pipe (const char *const *argv, int *fd)
{
  int pipe_fd[2];
  pid_t pid;

  *fd = 0;

  if (pipe (pipe_fd) < 0)
    {
      holy_util_warn (_("Unable to create pipe: %s"),
		      strerror (errno));
      return 0;
    }
  pid = fork ();
  if (pid < 0)
    holy_util_error (_("Unable to fork: %s"), strerror (errno));
  else if (pid == 0)
    {
      /* Child.  */

      /* Close fd's.  */
#ifdef holy_UTIL
      holy_util_devmapper_cleanup ();
      holy_diskfilter_fini ();
#endif

      /* Ensure child is not localised.  */
      setenv ("LC_ALL", "C", 1);

      close (pipe_fd[0]);
      dup2 (pipe_fd[1], STDOUT_FILENO);
      close (pipe_fd[1]);

      execvp ((char *) argv[0], (char **) argv);
      exit (127);
    }
  else
    {
      close (pipe_fd[1]);
      *fd = pipe_fd[0];
      return pid;
    }
}

pid_t
holy_util_exec_pipe_stderr (const char *const *argv, int *fd)
{
  int pipe_fd[2];
  pid_t pid;

  *fd = 0;

  if (pipe (pipe_fd) < 0)
    {
      holy_util_warn (_("Unable to create pipe: %s"),
		      strerror (errno));
      return 0;
    }
  pid = fork ();
  if (pid < 0)
    holy_util_error (_("Unable to fork: %s"), strerror (errno));
  else if (pid == 0)
    {
      /* Child.  */

      /* Close fd's.  */
#ifdef holy_UTIL
      holy_util_devmapper_cleanup ();
      holy_diskfilter_fini ();
#endif

      /* Ensure child is not localised.  */
      setenv ("LC_ALL", "C", 1);

      close (pipe_fd[0]);
      dup2 (pipe_fd[1], STDOUT_FILENO);
      dup2 (pipe_fd[1], STDERR_FILENO);
      close (pipe_fd[1]);

      execvp ((char *) argv[0], (char **) argv);
      exit (127);
    }
  else
    {
      close (pipe_fd[1]);
      *fd = pipe_fd[0];
      return pid;
    }
}
