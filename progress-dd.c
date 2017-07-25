#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <math.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/time.h>

#include <err.h>

/* Может упасть с сообщением No space left. Хорошо бы сделать, чтобы это не было ошибкой */

void human(ssize_t bytes)
{
  if (bytes < 0){
    fprintf(stderr, "                                                      Negative");
  }else
    {
      fprintf(stderr, "%4zd Gib %4zd Mib %4zd Kib %4zd bytes (%16zd bytes)", bytes / (1024 * 1024 * 1024), bytes / (1024 * 1024) % 1024, bytes / 1024 % 1024, bytes % 1024, bytes);
    }
}

void human_time (long long usec)
{
  long long sec = (usec + 500000) / 1000000;

  if (sec < 0){
    fprintf(stderr, "                                                      Negative");
  }else
    {
      fprintf(stderr, "                                     %2lld h %2lld m %2lld s (%6lld s)", sec / 3600, sec / 60 % 60, sec % 60, sec);
    }
}

int main(int argc, char *argv[])
{
  const char *input_file = NULL;
  const char *output_file = NULL;

  size_t bs = 1;
  size_t fs = 0;
  ssize_t count = -1;

  for (int i = 1; argv[i] != NULL; ++i){
    if (strncmp(argv[i], "if=", 3) == 0){
      input_file = argv[i] + 3;
    }else if (strncmp(argv[i], "of=", 3) == 0){
      output_file = argv[i] + 3;
    }else if (strncmp(argv[i], "bs=", 3) == 0){
      char *endptr;

      if (argv[i][3] == '\0'){
        errx (EXIT_FAILURE, "no bs");
      }

      errno = 0;
      bs = (size_t)strtoll(argv[i] + 3, &endptr, 10);

      if (*endptr == '\0')
        ;
      else if (strcmp(endptr, "K") == 0){
        bs *= 1024;
      }else if (strcmp(endptr, "M") == 0){
        bs *= 1024 * 1024;
      }else if (strcmp(endptr, "G") == 0){
        bs *= 1024 * 1024 * 1024;
      }else{
        errx (EXIT_FAILURE, "wrong bs");
      }

      if (errno != 0){
        err (EXIT_FAILURE, "wrong bs");
      }
    }else if (strncmp(argv[i], "fs=", 3) == 0){
      char *endptr;

      if (argv[i][3] == '\0'){
        errx (EXIT_FAILURE, "no fs");
      }

      errno = 0;
      fs = (size_t)strtoll(argv[i] + 3, &endptr, 10);

      if (*endptr == '\0')
        ;
      else if (strcmp(endptr, "K") == 0){
        fs *= 1024;
      }else if (strcmp(endptr, "M") == 0){
        fs *= 1024 * 1024;
      }else if (strcmp(endptr, "G") == 0){
        fs *= 1024 * 1024 * 1024;
      }else{
        errx (EXIT_FAILURE, "wrong fs");
      }

      if (errno != 0){
        err (EXIT_FAILURE, "wrong fs");
      }
    }else if (strncmp(argv[i], "count=", 6) == 0){
      char *endptr;

      if (argv[i][6] == '\0'){
        errx (EXIT_FAILURE, "no count");
      }

      errno = 0;
      count = (ssize_t)strtoll(argv[i] + 6, &endptr, 10);

      if (*endptr == '\0')
        ;
      else{
        errx (EXIT_FAILURE, "wrong count");
      }

      if (errno != 0){
        err (EXIT_FAILURE, "wrong count");
      }

      fs = bs * count;
    }else
      {
        errx (EXIT_FAILURE, "%s: wrong argument", argv[1]);
      }
  }

  if (fs == 0)
    {
      errx (EXIT_FAILURE, "usage: %s [if=FILE] [of=FILE] [bs=BLOCK-SIZE] fs=FULL-SIZE|count=COUNT (use K, M, G)", argv[0]);
    }

  int src, dest;

  if (input_file == NULL){
    src = 0;
  }else{
    src = open(input_file, O_RDONLY);

    if (src == -1)
      {
        err (EXIT_FAILURE, "%s", input_file);
      }
  }

  if (output_file == NULL){
    dest = 1;
  }else{
    dest = creat(output_file, 0666);

    if (dest == -1)
      {
        err (EXIT_FAILURE, "%s", output_file);
      }
  }

  char *buf = (char *)malloc(bs);

  struct timeval start;
  gettimeofday(&start, NULL);

  /* Цикл скопирован из Mini */

  size_t data_done = 0;

  for (ssize_t i = 0; i != count; ++i){
    ssize_t size;
    ssize_t write_result;

    if (i % 1 == 0){
      struct timeval now;
      gettimeofday(&now, NULL);

      long long time_done = ((long long)now.tv_sec - start.tv_sec) * 1000000 + ((long long)now.tv_usec - start.tv_usec);

      fprintf(stderr, "\033[H\033[2J");
      fprintf(stderr, "     |                                                           Done |                                                           Left |                                                          Total\n");

      fprintf(stderr, "Data | ");
      human(data_done);
      fprintf(stderr, " | ");
      human(fs - data_done);
      fprintf(stderr, " | ");
      human(fs);
      fputc('\n', stderr);

      fprintf(stderr, "Time | ");
      human_time (time_done);
      fprintf(stderr, " | ");
      human_time (llround(((double)fs * time_done) / data_done - time_done));
      fprintf(stderr, " | ");
      human_time (llround(((double)fs * time_done) / data_done));
      fputc('\n', stderr);
    }

    size = read(src, buf, bs);

    if (size == 0)
      {
        break;
      }

    if (size == -1){
      perror("read(...) failed");
      return -1;
    }

    {
      char *left = buf;

      while (size > 0){
        write_result = write(dest, left, size);

        if (write_result == -1){
          perror("write(...) failed");
          return -1;
        }

        left += write_result;
        size -= write_result;
        data_done += write_result;
      }
    }
  }

  free(buf);

  if (close(src) == -1)
    {
      err (EXIT_FAILURE, "%s: cannot close", input_file);
    }

  if (close(dest) == -1)
    {
      err (EXIT_FAILURE, "%s: cannot close", output_file);
    }

  fprintf(stderr, "OK\n");

  exit (EXIT_SUCCESS);
}
