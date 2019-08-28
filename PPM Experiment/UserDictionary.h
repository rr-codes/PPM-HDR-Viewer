#pragma once

#include <string>
#include <optional>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <utility>

namespace Utils
{
	class UserDictionary
	{

	public:
		explicit UserDictionary(std::filesystem::path path) : path_(std::move(path))
		{
			if (exists(path_))
			{
				std::ifstream in(path_);
				in >> map_;
				in.close();
			}
		}

		[[nodiscard]] int size() const
		{
			return map_.size();
		}

		template<typename ValT>
		[[nodiscard]] std::optional<ValT> at(const std::string& key)
		{
			return map_.contains(key) ? map_[key].get<ValT>() : std::optional<ValT>();
		}

		template<typename ValT>
		void insert(const std::string& key, ValT value)
		{
			map_[key] = value;
			save();
		}

		void erase(const std::string& key)
		{
			map_.erase(key);
			save();
		}

	private:
		std::filesystem::path path_;
		nlohmann::json map_;

		void save() const
		{
			std::ofstream out(path_, std::ios::out | std::ios::trunc);

			out << map_;
			out.close();
		}
	};

}



