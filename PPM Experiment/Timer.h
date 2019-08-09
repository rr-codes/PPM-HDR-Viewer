#pragma once
#include <chrono>

namespace Experiment {

	class Stopwatch
	{
		std::chrono::steady_clock::time_point m_start;

	public:
		Stopwatch()
		{
			Reset();
		}

		void Reset()
		{
			m_start = std::chrono::high_resolution_clock::now();
		}

		long long ElapsedMilliseconds() const
		{
			const auto t = std::chrono::high_resolution_clock::now();
			const auto t0 = m_start;

			const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t - t0).count();
			return duration;
		}
	};

}

