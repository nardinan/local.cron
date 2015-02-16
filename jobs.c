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
struct s_list *v_jobs;
int p_jobs_load_job(char *string, char separator, struct s_jobs_entry *entry) {
	enum e_jobs_timestamp current_timestamp = e_jobs_timestamp_minute;
	char *buffer, *pointer, *next;
	int result = d_false;
	size_t string_size;
	if ((string_size = f_string_strlen(string)) && (buffer = (char *) d_malloc(string_size+1))) {
		strncpy(buffer, string, string_size);
		pointer = buffer;
		while ((current_timestamp < e_jobs_timestamp_null) && (next = strchr(pointer, separator))) {
			*next = '\0';
			f_string_trim(pointer);
			if (f_string_strlen(pointer) > 0) {
				if (f_string_strcmp(pointer, "*") != 0)
					entry->timestamp[current_timestamp] = atoi(pointer);
				else
					entry->timestamp[current_timestamp] = -1;
				current_timestamp++;
			}
			pointer = (next+1);
		}
		if ((current_timestamp == e_jobs_timestamp_null) && (f_string_strlen(pointer) > 0)) {
			f_string_trim(pointer);
			strncpy(entry->action, pointer, d_string_buffer_size);
			result = d_true;
		}
		d_free(buffer);
	}
	return result;
}

int f_jobs_load(const char *file, char comment_separator, char job_separator) {
	FILE *stream;
	struct s_jobs_entry *entry = NULL;
	char buffer[d_string_buffer_size], *pointer;
	int result = d_false;
	if (!v_jobs)
		f_list_init(&v_jobs);
	if ((stream = fopen(file, "r"))) {
		while (!(feof(stream)))
			if ((fgets(buffer, d_string_buffer_size, stream)) > 0) {
				if ((pointer = strchr(buffer, comment_separator)))
					*pointer = '\0';
				f_string_trim(buffer);
				if (f_string_strlen(buffer) > 0)
					if ((entry) || (entry = (struct s_jobs_entry *) d_malloc(sizeof(struct s_jobs_entry)))) {
						if (p_jobs_load_job(buffer, job_separator, entry)) {
							f_list_append(v_jobs, (struct s_list_node *)entry, e_list_insert_head);
							entry = NULL;
						} else
							d_log(e_log_level_medium, "[configuration] - string '%s' has a wrong format", buffer);
					}
			}
		if (entry)
			d_free(entry);
		fclose(stream);
		result = d_true;
	} else
		d_err(e_log_level_medium, "[configuration] - crontab file %s not found", file);
	return result;
}

int p_jobs_run_execute(const char *command) {
	FILE *process;
	char buffer[d_string_buffer_size], complete_command[d_string_buffer_size];
	int result = d_false;
	snprintf(complete_command, d_string_buffer_size, "%s 2>&1", command);
	if ((process = popen(complete_command, "r"))) {
		while (fgets(buffer, d_string_buffer_size, process) != NULL)
			d_log(e_log_level_ever, "[output: %s] %s", command, f_string_trim(buffer));
		pclose(process);
		result = d_true;
	}
	return result;
}

int f_jobs_run(void) {
	struct s_jobs_entry *current_entry;
	time_t local_timestamp = time(NULL), starting, elapsed;
	struct tm *timestamp = localtime(&local_timestamp);
	int execute, ignorable, index, result = d_false, *timestamp_entries[] = {
		&(timestamp->tm_min),
		&(timestamp->tm_hour),
		&(timestamp->tm_mday),
		&(timestamp->tm_mon),
		&(timestamp->tm_year)
	};
	if (v_jobs) {
		d_foreach(v_jobs, current_entry, struct s_jobs_entry) {
			execute = d_true;
			for (index = 0; index < e_jobs_timestamp_null; ++index)
				if ((current_entry->timestamp[index] > -1) && (current_entry->timestamp[index] != *(timestamp_entries[index])))
					execute = d_false;
			if (execute) {
				for (index = (e_jobs_timestamp_null-1), ignorable = d_false, execute = d_false; index >= 0; --index) {
					if ((current_entry->timestamp[index] == -1) && (!ignorable)) {
						if (*(timestamp_entries[index]) != current_entry->last_timestamp[index]) {
							execute = d_true;
							current_entry->last_timestamp[index] = *(timestamp_entries[index]);
						}
					} else {
						ignorable = d_true;
						current_entry->last_timestamp[index] = -1;
					}
				}
				if (execute) {
					d_log(e_log_level_ever, "[execution] - {%d:%02d %02d/%02d/%04d} running '%s' ... ", timestamp->tm_hour,
							timestamp->tm_min, timestamp->tm_mday, timestamp->tm_mon, (timestamp->tm_year+1900),
							current_entry->action);
					starting = time(NULL);
					if (p_jobs_run_execute(current_entry->action)) {
						elapsed = time(NULL)-starting;
						d_log(e_log_level_ever, "[execution] \t... which required %zu seconds", elapsed);
					} else
						d_log(e_log_level_medium, "[execution] - p_job_run_execute(\"%s\") returns error", current_entry->action);

				}
			}
		}
		result = d_true;
	}
	return result;
}

void f_jobs_destroy(void) {
	struct s_jobs_entry *current;
	if (v_jobs) {
		while ((current = (struct s_jobs_entry *)v_jobs->head)) {
			f_list_delete(v_jobs, (struct s_list_node *)current);
			d_free(current);
		}
		f_list_destroy(&v_jobs);
	}
}
