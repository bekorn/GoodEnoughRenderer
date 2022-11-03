#pragma once

#include "core.hpp"

namespace GL
{
	struct Timer
	{
		u32 query_id[2];
		u64 time_elapsed_in_nanoseconds[2];

		f64 sample_window_begin = 0;
		f64 sample_window_interval = 0.5;

		u32 sample_count = 1; // to prevent division by zero
		u64 sample_sum;
		u64 average_in_nanoseconds;

		~Timer()
		{
			glDeleteQueries(2, query_id);
		}

		void create()
		{
			glGenQueries(2, query_id);
			glBeginQuery(GL_TIME_ELAPSED, query_id[0]);
			glEndQuery(GL_TIME_ELAPSED);
			glBeginQuery(GL_TIME_ELAPSED, query_id[1]);
			glEndQuery(GL_TIME_ELAPSED);
		}

		void begin(u32 frame_idx, f64 seconds_since_start)
		{
			bool idx = frame_idx % 2 == 0;
			glGetQueryObjectui64v(query_id[idx], GL_QUERY_RESULT_NO_WAIT, time_elapsed_in_nanoseconds + idx);

			if (seconds_since_start > sample_window_begin + sample_window_interval)
			{
				average_in_nanoseconds = sample_sum / sample_count;
				sample_sum = 0, sample_count = 0;
				sample_window_begin = seconds_since_start;
			}

			sample_sum += time_elapsed_in_nanoseconds[idx];
			sample_count += 1;

			glBeginQuery(GL_TIME_ELAPSED, query_id[not idx]);
		}

		void end()
		{
			glEndQuery(GL_TIME_ELAPSED);
		}
	};
}