#ifndef FILEINFO_HPP
#define FILEINFO_HPP

#include <vector>
#include <string>

struct FileInfo 
{
    	std::string filename;
    	
	std::vector<unsigned char>::const_iterator file_start;
    	
	std::vector<unsigned char>::const_iterator file_end;
};

#endif

