#include <er2/unity2/dumpsdk/writers/cli/UnityCliWriter.h>

#include <fstream>

namespace er2::UnityCli
{
	namespace
	{
		uint32_t AlignUp(uint32_t value, uint32_t alignment)
		{
			return (value + alignment - 1) / alignment * alignment;
		}

		void WriteDataDirectory(ByteWriter& writer, uint32_t rva, uint32_t size)
		{
			writer.WriteU32(rva);
			writer.WriteU32(size);
		}
	}

	bool WritePeImage(const std::string& path, const std::vector<uint8_t>& metadata, const std::vector<uint8_t>& methodBodies)
	{
		constexpr uint32_t PeHeaderOffset = 0x80;
		constexpr uint32_t FileAlignment = 0x200;
		constexpr uint32_t SectionAlignment = 0x2000;
		constexpr uint32_t SectionRva = 0x2000;
		constexpr uint32_t CliHeaderSize = 72;
		constexpr uint32_t HeadersSize = 0x200;

		const uint32_t methodBodiesSize = AlignUp(static_cast<uint32_t>(methodBodies.size()), 4);
		const uint32_t metadataRva = SectionRva + CliHeaderSize + methodBodiesSize;
		const uint32_t sectionVirtualSize = CliHeaderSize + methodBodiesSize + static_cast<uint32_t>(metadata.size());
		const uint32_t sectionRawSize = AlignUp(sectionVirtualSize, FileAlignment);
		const uint32_t sizeOfImage = AlignUp(SectionRva + sectionVirtualSize, SectionAlignment);

		ByteWriter writer;
		writer.WriteU8('M');
		writer.WriteU8('Z');
		writer.WriteZero(0x3A);
		writer.WriteU32(PeHeaderOffset);
		writer.WriteZero(PeHeaderOffset - writer.Size());

		writer.WriteU32(0x00004550);
		writer.WriteU16(0x014C);
		writer.WriteU16(1);
		writer.WriteU32(0);
		writer.WriteU32(0);
		writer.WriteU32(0);
		writer.WriteU16(0xE0);
		writer.WriteU16(0x2102);

		writer.WriteU16(0x10B);
		writer.WriteU8(14);
		writer.WriteU8(0);
		writer.WriteU32(sectionRawSize);
		writer.WriteU32(0);
		writer.WriteU32(0);
		writer.WriteU32(0);
		writer.WriteU32(SectionRva);
		writer.WriteU32(0);
		writer.WriteU32(0x00400000);
		writer.WriteU32(SectionAlignment);
		writer.WriteU32(FileAlignment);
		writer.WriteU16(4);
		writer.WriteU16(0);
		writer.WriteU16(0);
		writer.WriteU16(0);
		writer.WriteU16(4);
		writer.WriteU16(0);
		writer.WriteU32(0);
		writer.WriteU32(sizeOfImage);
		writer.WriteU32(HeadersSize);
		writer.WriteU32(0);
		writer.WriteU16(3);
		writer.WriteU16(0x8540);
		writer.WriteU32(0x100000);
		writer.WriteU32(0x1000);
		writer.WriteU32(0x100000);
		writer.WriteU32(0x1000);
		writer.WriteU32(0);
		writer.WriteU32(16);

		for (uint32_t i = 0; i < 16; ++i)
		{
			if (i == 14)
				WriteDataDirectory(writer, SectionRva, CliHeaderSize);
			else
				WriteDataDirectory(writer, 0, 0);
		}

		const char sectionName[8] = { '.', 't', 'e', 'x', 't', 0, 0, 0 };
		writer.WriteBytes(sectionName, sizeof(sectionName));
		writer.WriteU32(sectionVirtualSize);
		writer.WriteU32(SectionRva);
		writer.WriteU32(sectionRawSize);
		writer.WriteU32(HeadersSize);
		writer.WriteU32(0);
		writer.WriteU32(0);
		writer.WriteU16(0);
		writer.WriteU16(0);
		writer.WriteU32(0x60000020);
		writer.WriteZero(HeadersSize - writer.Size());

		writer.WriteU32(CliHeaderSize);
		writer.WriteU16(2);
		writer.WriteU16(5);
		WriteDataDirectory(writer, metadataRva, static_cast<uint32_t>(metadata.size()));
		writer.WriteU32(0x00000001);
		writer.WriteU32(0);
		WriteDataDirectory(writer, 0, 0);
		WriteDataDirectory(writer, 0, 0);
		WriteDataDirectory(writer, 0, 0);
		WriteDataDirectory(writer, 0, 0);
		WriteDataDirectory(writer, 0, 0);
		WriteDataDirectory(writer, 0, 0);

		writer.WriteBytes(methodBodies);
		writer.WriteZero(methodBodiesSize - static_cast<uint32_t>(methodBodies.size()));
		writer.WriteBytes(metadata);
		writer.WriteZero(sectionRawSize - sectionVirtualSize);

		std::ofstream output(path, std::ios::binary);
		if (!output)
			return false;

		const auto& image = writer.Data();
		output.write(reinterpret_cast<const char*>(image.data()), static_cast<std::streamsize>(image.size()));
		return output.good();
	}
}
