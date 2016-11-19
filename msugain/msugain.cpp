#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#endif

#include <cstdlib>
#include <cstdint>
#include <vector>
#include <iostream>
#include <fstream>

#define ENDIAN_SWAP(n) if(0x01020304 == htonl(0x01020304)) n = ((n << 8) & 0xFF00) | ((n >> 8) & 0x00FF)

int main(int argc, char **argv)
{
	unsigned files;
	double gain;

	if (argc < 2)
	{
		std::cout << "Usage: ./msugain [files] [gain]";
		return 1;
	}

	if ((gain = atof(argv[argc - 1])) != 0.0)
	{
		files = argc - 2;
	}
	else
	{
		gain = 0.65;
		files = argc - 1;
	}

	std::fstream f;
	std::vector<char> buf;
	for (unsigned i = 0; i < files; ++i)
	{
		unsigned clip_count = 0;
		buf.clear();

		f.open(argv[i + 1], std::ios::in | std::ios::binary);
		if (f.good())
		{

			for (unsigned j = 0; j < 8; ++j)
			{
				buf.push_back(f.get());
			}

			if (buf[0] != 'M')
				break;
			if (buf[1] != 'S')
				break;
			if (buf[2] != 'U')
				break;
			if (buf[3] != '1')
				break;

			int16_t sample;
			while (f.read((char *)&sample, 2).good())
			{
				ENDIAN_SWAP(sample);
					
				if (gain > 1.0)
				{
					int32_t tmp = int32_t(sample * gain);
					if (tmp > SHRT_MAX)
					{
						sample = SHRT_MAX;
						++clip_count;
					}
					else if (tmp < SHRT_MIN)
					{
						sample = SHRT_MIN;
						++clip_count;
					}
				}
				else
				{
					sample = int16_t(sample * gain);
				}

				ENDIAN_SWAP(sample);

				buf.push_back(sample & 0xFF);
				buf.push_back(sample >> 8);
			}
			f.close();

			bool write_good = true;

			if (clip_count > 0)
			{
				std::cout << "WARNING: " << clip_count << " samples clipped";
				if (files > 1)
				{
					std::cout << "in file " << argv[i + 1];
				}
				std::cout << " consider lowering the gain level." << std::endl;
				char in;
				do {
					std::cout << "Overwrite anyway [y/n]?: ";
					std::cin >> in;
					write_good = (in == 'y');
				} while (in != 'y' && in != 'n');
			}

			if (write_good)
			{
				f.open(argv[i + 1], std::ios::out | std::ios::binary | std::ios::trunc);
				if(!(f.bad() | f.fail()))
					std::copy(buf.begin(), buf.end(), std::ostreambuf_iterator<char>(f));
				f.close();
			}
		}
	}

	return 0;
}