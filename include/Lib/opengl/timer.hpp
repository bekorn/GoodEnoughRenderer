#pragma once

#include "core.hpp"

namespace GL
{
	struct Timer
	{
		u32 query_id[2];
		u64 time_elapsed_in_nanoseconds[2];

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

		void begin(u32 frame_idx)
		{
			bool idx = frame_idx % 2 == 0;
			glGetQueryObjectui64v(query_id[idx], GL_QUERY_RESULT, time_elapsed_in_nanoseconds + idx);
			glBeginQuery(GL_TIME_ELAPSED, query_id[not idx]);
		}

		void end()
		{
			glEndQuery(GL_TIME_ELAPSED);
		}

		u64 get_time_elapsed_in_nanoseconds(u32 frame_idx)
		{
			bool idx = frame_idx % 2 == 0;
			return time_elapsed_in_nanoseconds[idx];
		}
	};
}