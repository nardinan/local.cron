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
#ifndef local_cron_jobs_h
#define local_cron_jobs_h
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#define d_string_buffer_size 4096
#define d_jobs_stream_null -1
typedef enum e_jobs_timestamp {
  e_jobs_timestamp_minute = 0,
  e_jobs_timestamp_hour,
  e_jobs_timestamp_day,
  e_jobs_timestamp_month,
  e_jobs_timestamp_year,
  e_jobs_timestamp_null
} e_jobs_timestamp;
typedef struct s_jobs_entry {
  struct s_jobs_entry *next, *previous;
  char *action;
  int timestamp[e_jobs_timestamp_null], last_timestamp[e_jobs_timestamp_null];
} s_jobs_entry;
extern s_jobs_entry *v_jobs;
extern bool p_jobs_load_job(char *string, char separator, s_jobs_entry *entry);
extern int f_jobs_load(const char *file, char comment_separator, char job_separator);
extern int p_jobs_run_execute(const char *buffer);
extern int f_jobs_run(void);
extern void f_jobs_destroy(void);
#endif

