#pragma once
#include <chrono>
#include <atomic>
#include <functional>

namespace Utils
{
	using Clock = std::chrono::high_resolution_clock;

	template<typename TimeUnit = std::chrono::milliseconds>
	class Timer
	{
	public:
		explicit Timer(TimeUnit interval) :
			interval_(interval),
			previous_(Clock::now())
		{
		}

		explicit Timer(long long interval) : Timer(TimeUnit(interval))
		{
		}

		void Start()
		{
			start_ = true;
		}

		template<typename F>
		void Tick(F&& update)
		{
			if (!start_)
			{
				return;
			}

			auto duration = std::chrono::duration_cast<TimeUnit>(Clock::now() - previous_);

			if (duration >= interval_)
			{
				update();
				previous_ = Clock::now();
			}
		}


	private:
		TimeUnit interval_;
		std::chrono::time_point<Clock> previous_;
		bool start_ = false;
	};

	//**************************************************************************//
	//**************************************************************************//

	template<typename TimeUnit = std::chrono::milliseconds>
	class Stopwatch
	{
	public:
		explicit Stopwatch() : start_(Clock::now())
		{
		}

		void Restart()
		{
			start_ = Clock::now();
		}

		[[nodiscard]] TimeUnit Elapsed() const
		{
			return std::chrono::duration_cast<TimeUnit>(Clock::now() - start_);
		}

	private:
		std::chrono::time_point<Clock> start_;
	};
}