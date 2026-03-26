#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/param.h>
#include <sys/mount.h>
#endif


static void rstrip_slashes(char *s) {
  size_t n;
  if (s == NULL)
    return;
  n = strlen(s);
  while (n > 1 && s[n - 1] == '/') {
    s[n - 1] = '\0';
    n--;
  }
}

static char *normalize_path(const char *in) {
  char *rp;
  char tmp[PATH_MAX];

  if (in == NULL || in[0] == '\0')
    return NULL;

  rp = realpath(in, NULL);
  if (rp != NULL) {
    rstrip_slashes(rp);
    return rp;
  }

  /*
   * Fallback: build an absolute path without resolving symlinks.
   * This is less strict than realpath(), but avoids false negatives
   * when some components are not searchable.
   */
  if (in[0] == '/') {
    if (snprintf(tmp, sizeof(tmp), "%s", in) >= (int)sizeof(tmp))
      return NULL;
  } else {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
      return NULL;
    if (snprintf(tmp, sizeof(tmp), "%s/%s", cwd, in) >= (int)sizeof(tmp))
      return NULL;
  }

  rstrip_slashes(tmp);
  return strdup(tmp);
}

#if defined(__linux__)
static int octval(int c) {
  return (c >= '0' && c <= '7') ? (c - '0') : -1;
}

/*
 * Linux mountinfo escapes special chars as octal: '\040' for space, etc.
 * See proc(5).
 */
static int unescape_mountinfo(const char *in, char *out, size_t outsz) {
  size_t oi = 0;
  size_t i = 0;

  if (outsz == 0)
    return -1;

  while (in[i] != '\0') {
    if (oi + 1 >= outsz)
      return -1;

    if (in[i] == '\\') {
      int a, b, c;
      if (in[i + 1] == '\0')
        return -1;
      a = octval((unsigned char)in[i + 1]);
      b = octval((unsigned char)in[i + 2]);
      c = octval((unsigned char)in[i + 3]);
      if (a >= 0 && b >= 0 && c >= 0) {
        out[oi++] = (char)((a << 6) | (b << 3) | c);
        i += 4;
        continue;
      }
      /* Not an octal escape, keep backslash verbatim. */
    }

    out[oi++] = in[i++];
  }

  out[oi] = '\0';
  return 0;
}

static int is_mountpoint_linux(const char *path_norm) {
  FILE *fp;
  char *line = NULL;
  size_t cap = 0;
  int found = 0;

  fp = fopen("/proc/self/mountinfo", "r");
  if (fp == NULL)
    return 0;

  while (getline(&line, &cap, fp) != -1) {
    /*
     * mountinfo fields:
     * 1:mount_id 2:parent_id 3:major:minor 4:root 5:mount_point 6:options ...
     * We only need the 5th field.
     */
    char *p = line;
    char *fields[6];
    int fi = 0;

    while (fi < 6) {
      while (*p == ' ' || *p == '\t')
        p++;
      if (*p == '\0' || *p == '\n')
        break;
      fields[fi++] = p;
      while (*p != '\0' && *p != '\n' && *p != ' ' && *p != '\t')
        p++;
      if (*p == '\0' || *p == '\n')
        break;
      *p++ = '\0';
    }

    if (fi >= 5) {
      char mp[PATH_MAX];
      if (unescape_mountinfo(fields[4], mp, sizeof(mp)) == 0) {
        rstrip_slashes(mp);
        if (strcmp(mp, path_norm) == 0) {
          found = 1;
          break;
        }
      }
    }
  }

  free(line);
  fclose(fp);
  return found;
}
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
static int is_mountpoint_freebsd(const char *path_norm) {
  struct statfs *mnts = NULL;
  int n, i;

  n = getmntinfo(&mnts, MNT_NOWAIT);
  if (n <= 0 || mnts == NULL)
    return 0;

  for (i = 0; i < n; i++) {
    char mp[PATH_MAX];
    if (snprintf(mp, sizeof(mp), "%s", mnts[i].f_mntonname) >= (int)sizeof(mp))
      continue;
    rstrip_slashes(mp);
    if (strcmp(mp, path_norm) == 0)
      return 1;
  }
  return 0;
}
#endif

int main(int argc, char **argv) {
  struct stat st;
  char *path_norm;
  int is_mp = 0;

  if (argc != 2)
    return 1;

  path_norm = normalize_path(argv[1]);
  if (path_norm == NULL)
    return 1;

  if (lstat(path_norm, &st) != 0) {
    free(path_norm);
    return 1;
  }
  if (!S_ISDIR(st.st_mode)) {
    free(path_norm);
    return 1;
  }

#if defined(__linux__)
  is_mp = is_mountpoint_linux(path_norm);
#elif defined(__FreeBSD__) || defined(__DragonFly__)
  is_mp = is_mountpoint_freebsd(path_norm);
#else
  /* Unsupported OS: behave as "not a mountpoint". */
  is_mp = 0;
#endif

  free(path_norm);
  return is_mp ? 0 : 1;
}

