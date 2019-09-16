#pragma once

//
// Created by Richard Robinson on 2019-09-07.
//

#include <string>
#include <sstream>
#include <tuple>

namespace CSV::detail
{
	template <typename T>
	void convert(const std::string& s, T& out)
	{
		std::stringstream stream(s);
		stream >> out;
	}

	template <typename T>
	void convert(const std::string& s, std::string& out_s)
	{
		out_s = s;
	}

	inline void trim(std::string& str, const std::string& whitespace = " \t")
	{
		const auto strBegin = str.find_first_not_of(whitespace);
		if (strBegin == std::string::npos) return;

		const auto strEnd = str.find_last_not_of(whitespace);
		const auto strRange = strEnd - strBegin + 1;

		str = str.substr(strBegin, strRange);
	}

	template<std::size_t _Idx = 0, typename... _Ty>
	void output_helper(std::ostream& os, const std::tuple<_Ty...>& tuple)
	{
		os << std::get<_Idx>(tuple);

		if constexpr (sizeof...(_Ty) > _Idx + 1)
		{
			os << ", ";
			output_helper<_Idx + 1>(os, tuple);
		};
	}
}

namespace CSV::TupleHelper
{
	/**
	 * \brief Outputs a tuple with comma-delimited elements
	 */
	template<typename... Cols>
	std::ostream& operator<<(std::ostream& os, const std::tuple<Cols...>& tuple)
	{
		CSV::detail::output_helper<>(os, tuple);
		return os;
	}
}

namespace CSV
{
	template <typename ...Cols>
	class Reader
	{
		using value_type = std::tuple<Cols...>;
		class iterator;

	public:
		/**
		 * \brief Constructs a new Reader from the given stream using the optionally specified delimiter
		 * 
		 * \param csv the input stream of the CSV file
		 * \param delimiter the delimiting character (excluding whitespace) between columns (by default, ',')
		 */
		explicit Reader(std::istream& csv, const char delimiter = ',') : in_(csv), delimiter_(delimiter)
		{
			if (!in_.good())
			{
				throw std::out_of_range("Bad bit is set");
			}
		}

		/**
		 * \brief Skips and ignored the specified number of lines
		 * 
		 * \param numberOfLines the number of lines including invalid and comment lines to skip (by default, 1)
		 */
		void skip_lines(int numberOfLines = 1) const
		{
			in_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		}
		
		/**
		 * \brief Returns a \code std::tuple \endcode of \code Cols \endcode types
		 * 
		 * \return 
		 */
		value_type next_row()
		{
			value_type tuple;
			const auto next = next_valid_line();
			if (next.empty())
			{
				throw std::out_of_range("EOF");
			}

			std::stringstream temp(next);
			parse<>(temp, tuple);

			return tuple;
		}

		iterator begin() { return iterator(*this); }

		iterator end() { return iterator(); }

	private:
		std::istream& in_;
		const char delimiter_;

		template <std::size_t _Idx = 0>
		void parse(std::stringstream & line, value_type & out_tuple)
		{
			if constexpr (sizeof...(Cols) > _Idx)
			{
				std::string col;
				std::getline(line, col, delimiter_);
				detail::trim(col);

				detail::convert<decltype(std::get<_Idx>(out_tuple))>(col, std::get<_Idx>(out_tuple));
				parse<_Idx + 1>(line, out_tuple);
			}
		}

		[[nodiscard]] std::string next_valid_line() const
		{
			std::string line;
			while (std::getline(in_, line))
			{
				detail::trim(line);
				if (!line.empty() && line[0] != '#') return line;
			}

			return "";
		}
	};

	template <typename ...Cols>
	class Reader<Cols...>::iterator
	{
	public:
		using iterator_category = std::input_iterator_tag;
		using value_type		= typename Reader<Cols...>::value_type;
		using difference_type	= std::size_t;

		using pointer	= value_type *;
		using reference = value_type &;

		iterator() : reader_(nullptr) {}

		explicit iterator(Reader<Cols...>& other) : reader_(other.in_.good() ? &other : nullptr)
		{
			++(*this);
		}

		iterator& operator++()
		{
			if (reader_ != nullptr && reader_->in_.good())
			{
				try {
					row_ = reader_->next_row();
					return *this;
				}
				catch (...) {}
			}

			reader_ = nullptr;
			return *this;
		}

		iterator operator++(int i)
		{
			iterator tmp = *this;
			++(*this);
			return tmp;
		}

		value_type const& operator*() const { return row_; }

		value_type const& operator->() const { return &row_; }

		bool operator==(iterator const& other)
		{
			return (this == &other) || (reader_ == nullptr && other.reader_ == nullptr);
		}

		bool operator!=(iterator const& other)
		{
			return !(*this == other);
		}

	private:
		value_type row_;
		Reader<Cols...>* reader_;
	};
}