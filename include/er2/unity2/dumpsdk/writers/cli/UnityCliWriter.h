#pragma once

#include <er2/unity2/dumpsdk/collected_data.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace er2::UnityCli
{
	enum class TableId : uint8_t
	{
		Module = 0x00,
		TypeRef = 0x01,
		TypeDef = 0x02,
		Field = 0x04,
		MethodDef = 0x06,
		Param = 0x08,
		PropertyMap = 0x15,
		Property = 0x17,
		MethodSemantics = 0x18,
		Assembly = 0x20,
		AssemblyRef = 0x23,
	};

	class ByteWriter
	{
	public:
		void WriteU8(uint8_t value);
		void WriteU16(uint16_t value);
		void WriteU32(uint32_t value);
		void WriteU64(uint64_t value);
		void WriteBytes(const void* data, size_t size);
		void WriteBytes(const std::vector<uint8_t>& data);
		void WriteZero(size_t size);
		void AlignTo(size_t alignment);
		size_t Size() const;
		const std::vector<uint8_t>& Data() const;
		std::vector<uint8_t>& Data();

	private:
		std::vector<uint8_t> data_;
	};

	class MetadataHeaps
	{
	public:
		MetadataHeaps();

		uint32_t GetString(const std::string& value);
		uint32_t AddBlob(const std::vector<uint8_t>& payload);
		uint32_t AddGuid(const std::array<uint8_t, 16>& value);

		const std::vector<uint8_t>& Strings() const;
		const std::vector<uint8_t>& Blob() const;
		const std::vector<uint8_t>& Guid() const;

		uint8_t HeapSizeFlags() const;
		uint32_t StringIndexSize() const;
		uint32_t BlobIndexSize() const;
		uint32_t GuidIndexSize() const;

	private:
		std::vector<uint8_t> strings_;
		std::vector<uint8_t> blob_;
		std::vector<uint8_t> guid_;
		std::unordered_map<std::string, uint32_t> stringOffsets_;
	};

	struct TypeRefRow
	{
		uint32_t ResolutionScope = 0;
		uint32_t Name = 0;
		uint32_t Namespace = 0;
	};

	struct TypeDefRow
	{
		uint32_t Flags = 0;
		uint32_t Name = 0;
		uint32_t Namespace = 0;
		uint32_t Extends = 0;
		uint32_t FieldList = 1;
		uint32_t MethodList = 1;
	};

	struct FieldRow
	{
		uint16_t Flags = 0;
		uint32_t Name = 0;
		uint32_t Signature = 0;
	};

	struct MethodDefRow
	{
		uint32_t Rva = 0;
		uint16_t ImplFlags = 0;
		uint16_t Flags = 0;
		uint32_t Name = 0;
		uint32_t Signature = 0;
		uint32_t ParamList = 1;
	};

	struct ParamRow
	{
		uint16_t Flags = 0;
		uint16_t Sequence = 0;
		uint32_t Name = 0;
	};

	struct PropertyMapRow
	{
		uint32_t Parent = 0;
		uint32_t PropertyList = 1;
	};

	struct PropertyRow
	{
		uint16_t Flags = 0;
		uint32_t Name = 0;
		uint32_t Type = 0;
	};

	struct MethodSemanticsRow
	{
		uint16_t Semantics = 0;
		uint32_t Method = 0;
		uint32_t Association = 0;
	};

	struct AssemblyRow
	{
		uint32_t HashAlgId = 0x00008004;
		uint16_t MajorVersion = 1;
		uint16_t MinorVersion = 0;
		uint16_t BuildNumber = 0;
		uint16_t RevisionNumber = 0;
		uint32_t Flags = 0;
		uint32_t PublicKey = 0;
		uint32_t Name = 0;
		uint32_t Culture = 0;
	};

	struct AssemblyRefRow
	{
		uint16_t MajorVersion = 4;
		uint16_t MinorVersion = 0;
		uint16_t BuildNumber = 0;
		uint16_t RevisionNumber = 0;
		uint32_t Flags = 0;
		uint32_t PublicKeyOrToken = 0;
		uint32_t Name = 0;
		uint32_t Culture = 0;
		uint32_t HashValue = 0;
	};

	struct MetadataRows
	{
		uint32_t ModuleName = 0;
		uint32_t ModuleMvid = 1;
		AssemblyRow Assembly;
		std::vector<AssemblyRefRow> AssemblyRefs;
		std::vector<TypeRefRow> TypeRefs;
		std::vector<TypeDefRow> TypeDefs;
		std::vector<FieldRow> Fields;
		std::vector<MethodDefRow> Methods;
		std::vector<ParamRow> Params;
		std::vector<PropertyMapRow> PropertyMaps;
		std::vector<PropertyRow> Properties;
		std::vector<MethodSemanticsRow> MethodSemantics;

		std::array<uint32_t, 64> RowCounts() const;
	};

	void WriteCompressedUInt(ByteWriter& writer, uint32_t value);
	uint32_t EncodeTypeDefOrRef(uint32_t rowIndex, TableId table);
	uint32_t EncodeResolutionScope(uint32_t rowIndex, TableId table);
	uint32_t TableIndexSize(const std::array<uint32_t, 64>& rowCounts, TableId table);
	uint32_t CodedIndexSize(const std::array<uint32_t, 64>& rowCounts, const std::vector<TableId>& tables, uint32_t tagBits);

	bool BuildMetadata(const CollectedAssembly& assembly, std::vector<uint8_t>& metadata, std::vector<uint8_t>& methodBodies);
	std::vector<uint8_t> BuildMetadataRoot(const MetadataRows& rows, const MetadataHeaps& heaps);
	bool WritePeImage(const std::string& path, const std::vector<uint8_t>& metadata, const std::vector<uint8_t>& methodBodies);
	bool WriteAssembly(const std::string& path, const CollectedAssembly& assembly);
}
