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
#include "jobs.h"
s_jobs_entry *v_jobs;
bool p_jobs_load_job(char *string, char separator, struct s_jobs_entry *entry) {
  enum e_jobs_timestamp current_timestamp = e_jobs_timestamp_minute;
  char *buffer, *pointer, *next, *trimmer;
  int result = false;
  size_t string_size, residual_size;
  if (((string_size = strlen(string)) > 0) && (buffer = (char *)malloc(string_size + 1))) {
    strcpy(buffer, string);
    buffer[string_size] = 0;
    pointer = buffer;
    while ((current_timestamp < e_jobs_timestamp_null) && (next = strchr(pointer, separator))) {
      trimmer = next;
      while ((trimmer > pointer) && (strchr("\n\t\r ", *trimmer))) {
        *trimmer = 0;
        --trimmer;
      }
      if (strlen(pointer) > 0) {
        if (strcmp(pointer, "*") != 0)
          entry->timestamp[current_timestamp] = atoi(pointer);
        else
          entry->timestamp[current_timestamp] = -1;
        current_timestamp++;
      }
      pointer = (next + 1);
    }
    if ((current_timestamp == e_jobs_timestamp_null) && ((residual_size = strlen(pointer)) > 0)) {
      trimmer = (pointer + residual_size);
      while ((trimmer > pointer) && (strchr("\n\t\r ", *trimmer))) {
        *trimmer = 0;
        --trimmer;
      }
      if ((residual_size = strlen(pointer)) > 0)
        if ((entry->action = (char *) malloc(residual_size + 1))) {
          strcpy(entry->action, pointer);
          entry->action[residual_size] = 0;
          result = true;
        }
    }
  }
  return result;
}
int f_jobs_load(const char *file, char comment_separator, char job_separator) {
  FILE *stream;
  struct s_jobs_entry *entry = NULL;
  char buffer[d_string_buffer_size], *pointer;
  int result = false;
  if ((stream = fopen(file, "r"))) {
    while (!(feof(stream))) {
      memset(buffer, 0, d_string_buffer_size);
      if ((fgets(buffer, d_string_buffer_size, stream)) > 0) {
        if ((pointer = strchr(buffer, comment_separator)))
          *pointer = 0;
        if (strlen(buffer) > 0)
          if ((entry) || (entry = (struct s_jobs_entry *) malloc(sizeof(struct s_jobs_entry)))) {
            if (p_jobs_load_job(buffer, job_separator, entry)) {
              if ((entry->next = v_jobs))
                v_jobs->previous = entry;
              v_jobs = entry;
              entry = NULL;
              syslog(LOG_INFO, "[configuration] - job '%s' has been included", v_jobs->action);
            } else
              syslog(LOG_WARNING, "[configuration] - string '%s' has a wrong format", buffer);
          }
      }
    }
    if (entry)
      free(entry);
    fclose(stream);
    result = true;
  } else
    syslog(LOG_ERR, "[configuration] - crontab file %s not found", file);
  return result;
}
int p_jobs_run_execute(const char *command) {
  FILE *process;
  char buffer[d_string_buffer_size], complete_command[d_string_buffer_size], *trimmer;
  int result = false;
  size_t residual_size;
  snprintf(complete_command, d_string_buffer_size, "%s 2>&1", command);
  if ((process = popen(complete_command, "r"))) {
    while (fgets(buffer, d_string_buffer_size, process) != NULL) {
      if ((residual_size = strlen(buffer)) > 0) {
        trimmer = (buffer + residual_size);
        while ((trimmer > buffer) && (strchr("\n\t\r ", *trimmer))) {
          *trimmer = 0;
          --trimmer;
          --residual_size;
        }
      }
      syslog(LOG_INFO, "[output: %s] %s", command, buffer);
    }
    pclose(process);
    result = true;
  }
  return result;
}
int f_jobs_run(void) {
  struct s_jobs_entry *current_entry;
  time_t local_timestamp = time(NULL), starting;
  struct tm *timestamp = localtime(&local_timestamp);
  int jolly_marker_different, defined_markers_matching, index, result = false, *timestamp_entries[] = {
    &(timestamp->tm_min),
    &(timestamp->tm_hour),
    &(timestamp->tm_mday),
    &(timestamp->tm_mon),
    &(timestamp->tm_year)
  };
  if (v_jobs) {
    current_entry = v_jobs;
    while (current_entry) {
      defined_markers_matching = true;
      jolly_marker_different = false;
      /* to know if an entry has to be launched we need the following two conditions to be true:
       * - every defined marker (different from -1) has to match with the current timestamp
       * - at least one of the last_timestamp values of the jolly markers (the onest set to -1) is different from the current timestamp
       */
      for (index = 0; (index < e_jobs_timestamp_null) && (defined_markers_matching); ++index) {
        if (current_entry->timestamp[index] == -1) {
          if (current_entry->last_timestamp[index] != *(timestamp_entries[index]))
            jolly_marker_different = true;
        } else if (current_entry->timestamp[index] != *(timestamp_entries[index]))
          defined_markers_matching = false;
      }
      if ((defined_markers_matching) && (jolly_marker_different)) {
        for (index = 0; index < e_jobs_timestamp_null; ++index)
          current_entry->last_timestamp[index] = *(timestamp_entries[index]);
        syslog(LOG_INFO, "[execution] - {%d:%02d %02d/%02d/%04d} running '%s' ... ", timestamp->tm_hour,
            timestamp->tm_min, timestamp->tm_mday, timestamp->tm_mon, (timestamp->tm_year+1900),
            current_entry->action);
        starting = time(NULL);
        if (p_jobs_run_execute(current_entry->action))
          syslog(LOG_INFO, "[execution] \t... which required %zu seconds", (time(NULL) - starting));
        else
          syslog(LOG_WARNING, "[execution] - p_job_run_execute(\"%s\") returns error", current_entry->action);
      }
      current_entry = current_entry->next;
    }
    result = true;
  }
  return result;
}
void f_jobs_destroy(void) {
  struct s_jobs_entry *current;
  if (v_jobs) {
    while ((current = v_jobs)) {
      v_jobs = current->next;
      if (current->action)
        free(current->action);
      free(current);
    }
    v_jobs = NULL;
  }
}
