#pragma once
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

#include <filesystem>
#include <dirent.h>	// Using 17 now so not necessary?


namespace FileUtils
{
	inline static std::string loadFileContents(const std::string& path)
	{
		std::ifstream t(path);
		std::stringstream buffer;
		buffer << t.rdbuf();

		return buffer.str();
	}



	inline static bool fileExists(const std::string& name)
	{
		std::ifstream f(name.c_str());
		return f.good();
	}



	static void printDirContents(const std::string& path)
	{
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			std::cout << entry.path().string() << std::endl;
		}
	}



	static bool getDirContentsAsStrings(const std::string& path, std::vector<std::string>& contents)
	{
		if (!std::filesystem::is_directory(path))
			return false;

		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			contents.push_back(entry.path().filename().string());
		}

		return true;
	}



	static bool findFile(const std::string& baseFolder, const std::string& fileName, std::filesystem::directory_entry& out)
	{
		std::filesystem::recursive_directory_iterator rdi(baseFolder);
		for (std::filesystem::directory_entry dirEntry : rdi)
		{
			if (dirEntry.path().filename() == fileName && dirEntry.is_regular_file())
			{
				out = dirEntry;
				return true;
			}
		}
		return false;
	}



	static std::vector<char> readAllBytes(char const* filename)
	{
		static_assert(sizeof(char) == 1);								// Am I paranoid? Yes I aaaam

		std::ifstream ifs(filename, std::ios::binary | std::ios::ate);	// Construct with cursor "At-The-End"
		std::ifstream::pos_type byteCount = ifs.tellg();				// Save file size

		std::vector<char> result(byteCount);	// Create a vector of matching size

		ifs.seekg(0, std::ios::beg);
		ifs.read(result.data(), byteCount);		// Read byteCount bytes into the result vector

		return result;
	}



	static void writeAllBytes(char const* filename, const void* content, const size_t size, int flags = std::ios::out | std::ios::binary)
	{
		std::ofstream writer(filename, flags);
		writer.write(static_cast<const char*>(content), size);
	}



	static bool getRecursiveIterator(const std::filesystem::path& dirPath, std::filesystem::recursive_directory_iterator& iter)
	{
		if (std::filesystem::is_directory(dirPath))
		{
			iter = std::filesystem::recursive_directory_iterator(dirPath);
		}
		return false;
	}



	static std::vector<std::filesystem::directory_entry> getFilesByExt
	(const std::filesystem::path& dir, const std::filesystem::path& ext)
	{
		std::vector<std::filesystem::directory_entry> files;
		std::filesystem::recursive_directory_iterator rdi;
		if(getRecursiveIterator(dir, rdi))
		{
			for (auto file : rdi)
			{
				if (file.path().extension() == ext)
					files.push_back(file);
			}
		}
		return files;
	}
}