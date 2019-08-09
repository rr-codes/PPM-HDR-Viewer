#pragma once
#include <chrono>
#include <atomic>

namespace Utils {

	class Counter
	{
	public:

		Counter(double delta) : delta_(delta)
		{
		}

		void Tick() { duration_ += delta_; }
		void Reset() { duration_ = 0; }
		double Elapsed() const { return duration_; }

	private:
		double delta_;
		double duration_ = 0;
	};
}