#ifndef __CA_TEST_H__
#define __CA_TEST_H__
#include <array>
#include <vector>
#include <string>

#define  CA_TEST_NLOCKTIME     0
#define  CA_TEST_NTIME             1
#define  CA_TEST_PRINT              0
#define  CA_TEST_INFO                1


typedef enum emCaTestFormatType
{
	kCaTestFormatType_Print,
	kCaTestFormatType_File,
}CaTestFormatType;	

void blkprint(CaTestFormatType type, int fd, const char *format, ...);
void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c);

void printRocksdb(uint64_t start, uint64_t end);
std::string printBlocks(int num = 0);
#endif
