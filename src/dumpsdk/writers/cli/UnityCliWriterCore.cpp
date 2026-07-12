#include <er2/unity2/dumpsdk/writers/cli/UnityCliWriter.h>

#include <algorithm>
#include <cstring>
#include <fstream>

namespace er2::UnityCli
{
	void ByteWriter::WriteU8(uint8_t value)
	{
		data_.push_back(value);
	}

	void ByteWriter::WriteU16(uint16_t value)
	{
		data_.push_back(static_cast<uint8_t>(value));
		data_.push_back(static_cast<uint8_t>(value >> 8));
	}

	void ByteWriter::WriteU32(uint32_t value)
	{
		data_.push_back(static_cast<uint8_t>(value));
		data_.push_back(static_cast<uint8_t>(value >> 8));
		data_.push_back(static_cast<uint8_t>(value >> 16));
		data_.push_back(static_cast<uint8_t>(value >> 24));
	}

	void ByteWriter::WriteU64(uint64_t value)
	{
		WriteU32(static_cast<uint32_t>(value));
		WriteU32(static_cast<uint32_t>(value >> 32));
	}

	void ByteWriter::WriteBytes(const void* data, size_t size)
	{
		const auto* bytes = static_cast<const uint8_t*>(data);
		data_.insert(data_.end(), bytes, bytes + size);
	}

	void ByteWriter::WriteBytes(const std::vector<uint8_t>& data)
	{
		data_.insert(data_.end(), data.begin(), data.end());
	}

	void ByteWriter::WriteZero(size_t size)
	{
		data_.insert(data_.end(), size, 0);
	}

	void ByteWriter::AlignTo(size_t alignment)
	{
		const size_t remainder = data_.size() % alignment;
		if (remainder != 0)
			WriteZero(alignment - remainder);
	}

	size_t ByteWriter::Size() const
	{
		return data_.size();
	}

	const std::vector<uint8_t>& ByteWriter::Data() const
	{
		return data_;
	}

	std::vector<uint8_t>& ByteWriter::Data()
	{
		return data_;
	}

	MetadataHeaps::MetadataHeaps()
	{
		strings_.push_back(0);
		blob_.push_back(0);
	}

	uint32_t MetadataHeaps::GetString(const std::string& value)
	{
		if (value.empty())
			return 0;

		const auto existing = stringOffsets_.find(value);
		if (existing != stringOffsets_.end())
			return existing->second;

		const uint32_t offset = static_cast<uint32_t>(strings_.size());
		strings_.insert(strings_.end(), value.begin(), value.end());
		strings_.push_back(0);
		stringOffsets_.emplace(value, offset);
		return offset;
	}

	uint32_t MetadataHeaps::AddBlob(const std::vector<uint8_t>& payload)
	{
		const uint32_t offset = static_cast<uint32_t>(blob_.size());
		ByteWriter encoded;
		WriteCompressedUInt(encoded, static_cast<uint32_t>(payload.size()));
		blob_.insert(blob_.end(), encoded.Data().begin(), encoded.Data().end());
		blob_.insert(blob_.end(), payload.begin(), payload.end());
		return offset;
	}

	uint32_t MetadataHeaps::AddGuid(const std::array<uint8_t, 16>& value)
	{
		const uint32_t index = static_cast<uint32_t>(guid_.size() / 16) + 1;
		guid_.insert(guid_.end(), value.begin(), value.end());
		return index;
	}

	const std::vector<uint8_t>& MetadataHeaps::Strings() const
	{
		return strings_;
	}

	const std::vector<uint8_t>& MetadataHeaps::Blob() const
	{
		return blob_;
	}

	const std::vector<uint8_t>& MetadataHeaps::Guid() const
	{
		return guid_;
	}

	uint8_t MetadataHeaps::HeapSizeFlags() const
	{
		uint8_t flags = 0;
		if (strings_.size() > 0xFFFF)
			flags |= 0x01;
		if ((guid_.size() / 16) > 0xFFFF)
			flags |= 0x02;
		if (blob_.size() > 0xFFFF)
			flags |= 0x04;
		return flags;
	}

	uint32_t MetadataHeaps::StringIndexSize() const
	{
		return strings_.size() > 0xFFFF ? 4u : 2u;
	}

	uint32_t MetadataHeaps::BlobIndexSize() const
	{
		return blob_.size() > 0xFFFF ? 4u : 2u;
	}

	uint32_t MetadataHeaps::GuidIndexSize() const
	{
		return (guid_.size() / 16) > 0xFFFF ? 4u : 2u;
	}

	std::array<uint32_t, 64> MetadataRows::RowCounts() const
	{
		std::array<uint32_t, 64> counts{};
		counts[static_cast<size_t>(TableId::Module)] = 1;
		counts[static_cast<size_t>(TableId::TypeRef)] = static_cast<uint32_t>(TypeRefs.size());
		counts[static_cast<size_t>(TableId::TypeDef)] = static_cast<uint32_t>(TypeDefs.size());
		counts[static_cast<size_t>(TableId::Field)] = static_cast<uint32_t>(Fields.size());
		counts[static_cast<size_t>(TableId::MethodDef)] = static_cast<uint32_t>(Methods.size());
		counts[static_cast<size_t>(TableId::Param)] = static_cast<uint32_t>(Params.size());
		counts[static_cast<size_t>(TableId::PropertyMap)] = static_cast<uint32_t>(PropertyMaps.size());
		counts[static_cast<size_t>(TableId::Property)] = static_cast<uint32_t>(Properties.size());
		counts[static_cast<size_t>(TableId::MethodSemantics)] = static_cast<uint32_t>(MethodSemantics.size());
		counts[static_cast<size_t>(TableId::Assembly)] = 1;
		counts[static_cast<size_t>(TableId::AssemblyRef)] = static_cast<uint32_t>(AssemblyRefs.size());
		return counts;
	}

	void WriteCompressedUInt(ByteWriter& writer, uint32_t value)
	{
		if (value <= 0x7F)
		{
			writer.WriteU8(static_cast<uint8_t>(value));
		}
		else if (value <= 0x3FFF)
		{
			writer.WriteU8(static_cast<uint8_t>((value >> 8) | 0x80));
			writer.WriteU8(static_cast<uint8_t>(value));
		}
		else
		{
			writer.WriteU8(static_cast<uint8_t>((value >> 24) | 0xC0));
			writer.WriteU8(static_cast<uint8_t>(value >> 16));
			writer.WriteU8(static_cast<uint8_t>(value >> 8));
			writer.WriteU8(static_cast<uint8_t>(value));
		}
	}

	uint32_t EncodeTypeDefOrRef(uint32_t rowIndex, TableId table)
	{
		uint32_t tag = 0;
		if (table == TableId::TypeRef)
			tag = 1;
		return (rowIndex << 2) | tag;
	}

	uint32_t EncodeResolutionScope(uint32_t rowIndex, TableId table)
	{
		uint32_t tag = 0;
		if (table == TableId::AssemblyRef)
			tag = 2;
		else if (table == TableId::TypeRef)
			tag = 3;
		return (rowIndex << 2) | tag;
	}

	uint32_t TableIndexSize(const std::array<uint32_t, 64>& rowCounts, TableId table)
	{
		return rowCounts[static_cast<size_t>(table)] > 0xFFFF ? 4u : 2u;
	}

	uint32_t CodedIndexSize(const std::array<uint32_t, 64>& rowCounts, const std::vector<TableId>& tables, uint32_t tagBits)
	{
		uint32_t maxRows = 0;
		for (const TableId table : tables)
			maxRows = std::max(maxRows, rowCounts[static_cast<size_t>(table)]);
		return maxRows < (1u << (16 - tagBits)) ? 2u : 4u;
	}

	bool WriteAssembly(const std::string& path, const CollectedAssembly& assembly)
	{
		std::vector<uint8_t> metadata;
		std::vector<uint8_t> methodBodies;
		if (!BuildMetadata(assembly, metadata, methodBodies))
			return false;
		return WritePeImage(path, metadata, methodBodies);
	}
}
