/*
 * local.cron
 * Copyright (C) 2015 Andrea Nardinocchi (andrea@nardinan.it)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include "jobs.h"
#define d_local_cron_comment_separator '#'
#define d_local_cron_job_separator ' '
#define d_local_cron_crontab "./local.crontab"
#define d_local_cron_sleep_seconds 1
int v_local_cron_interrupt = false, v_local_cron_timestamp = 0;
void p_local_cron_sigpipe(int signal) {
  /* just ignore */
}
void p_local_cron_sigint(int signal) {
  v_local_cron_interrupt = true;
}
int f_local_cron_analyze_file(char *file) {
  struct stat file_attribute;
  int result = false;
  stat(file, &file_attribute);
  if (v_local_cron_timestamp < file_attribute.st_ctime) {
    v_local_cron_timestamp = file_attribute.st_ctime;
    result = true;
  }
  return result;
}
int main (int argc, char *argv[]) {
  char *current_crontab = d_local_cron_crontab;
  openlog("local_cron", (LOG_PID | LOG_CONS), LOG_USER);
  if (argc >= 2)
    current_crontab = argv[1];
  if (f_jobs_load(current_crontab, d_local_cron_comment_separator, d_local_cron_job_separator)) {
    signal(SIGPIPE, p_local_cron_sigpipe);
    signal(SIGINT, p_local_cron_sigint);
    while (!v_local_cron_interrupt) {
      if (f_local_cron_analyze_file(current_crontab)) {
        f_jobs_destroy();
        if (f_jobs_load(current_crontab, d_local_cron_comment_separator, d_local_cron_job_separator))
          syslog(LOG_INFO, "crontab %s has been reloaded", current_crontab);
        else {
          syslog(LOG_ERR, "it's impossible to initialize & load crontab %s", current_crontab);
          break;
        }
      }
      if (!f_jobs_run())
        syslog(LOG_ERR, "[main log] - f_jobs_run fails");
      sleep(d_local_cron_sleep_seconds);
    }
  } else
    syslog(LOG_ERR, "it's impossible to initialize & load crontab %s", current_crontab);
  f_jobs_destroy();
  closelog();
  return 0;
}
