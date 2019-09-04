#pragma once

#include <tuple>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>

namespace Utils
{
	template<typename ...ColType>
	class CSV
	{
	public:
		using value_type = std::tuple<ColType...>;

		class iterator;

		explicit CSV(std::istream& in, const char delimiter = ',', const char comment_flag = '#')
			: in_(in),
			delimiter(delimiter),
			comment(comment_flag)
		{
		}

		[[nodiscard]] std::string get_line() const
		{
			std::string line;
			std::getline(in_, line);
			return line;
		}

		template<typename T>
		[[nodiscard]] T get_line() const
		{
			auto s = get_line();
			trim(s);

			std::stringstream tmp(s);
			T val;
			tmp >> val;

			return val;
		}

		std::optional<value_type> get_row()
		{
			auto line = next_valid_line();
			if (line.empty())
			{
				return std::optional<value_type>();
			}

			auto next = std::stringstream(line);
			std::tuple<ColType...> tuple;

			parse_row<>(next, tuple);
			return std::optional<value_type>(tuple);
		}

		[[nodiscard]] inline bool good() const { return in_.good(); }

		inline iterator begin() { return iterator(*this); }

		inline iterator end() { return iterator(); }

	private:
		std::istream& in_;
		const char delimiter = ',';
		const char comment = '#';

		[[nodiscard]] std::string next_valid_line() const
		{
			std::string line;
			while (std::getline(in_, line))
			{
				if (!line.empty() && !isspace(line.front()) && line.rfind(comment, 0) != 0)
				{
					return line;
				}
			}

			return "";
		}

		template<std::size_t Index = 0>
		void parse_row(std::stringstream & s, value_type & tuple)
		{
			if constexpr (sizeof...(ColType) > Index)
			{
				std::string col;
				std::getline(s, col, delimiter);
				trim(col);

				std::stringstream tmp(col);
				tmp >> std::get<Index>(tuple);

				parse_row<Index + 1>(s, tuple);
			}
		}

		static void trim(std::string& s)
		{
			const auto is_space = [](char c)
			{ return !std::isspace(c); };

			s.erase(s.begin(), std::find_if(s.begin(), s.end(), is_space));
			s.erase(std::find_if(s.rbegin(), s.rend(), is_space).base(), s.end());
		}
	};

	template<typename... ColType>
	class CSV<ColType...>::iterator
	{
	public:
		using iterator_category = std::input_iterator_tag;
		using value_type        = typename CSV::value_type;
		using difference_type   = std::size_t;

		using pointer   = typename CSV::value_type*;
		using reference = typename CSV::value_type &;

		iterator() : csv_(nullptr)
		{
		}

		explicit iterator(CSV& other) : csv_(other.good() ? &other : nullptr)
		{
			++(*this);
		}

		inline iterator& operator++()
		{
			if (csv_ != nullptr && csv_->good())
			{
				auto tmp = csv_->get_row();
				if (tmp.has_value())
				{
					row_ = tmp.value();
					return *this;
				}
			}

			csv_ = nullptr;
			return *this;
		}

		inline iterator operator++(int i)
		{
			iterator tmp = *this;
			++(*this);
			return tmp;
		}

		inline value_type const& operator*() const { return row_; }

		inline value_type const* operator->() const { return &row_; }

		inline bool operator==(iterator const& other)
		{
			return (this == &other) || (csv_ == nullptr && other.csv_ == nullptr);
		}

		inline bool operator!=(iterator const& other)
		{
			return !(*this == other);
		}

	private:
		typename CSV::value_type row_;
		CSV* csv_;
	};

}

