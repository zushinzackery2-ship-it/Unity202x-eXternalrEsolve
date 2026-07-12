#include <er2/unity2/dumpsdk/writers/cli/UnityCliWriter.h>

#include <array>

namespace er2::UnityCli
{
	namespace
	{
		void WriteSizedIndex(ByteWriter& writer, uint32_t value, uint32_t size)
		{
			if (size == 2)
				writer.WriteU16(static_cast<uint16_t>(value));
			else
				writer.WriteU32(value);
		}

		void WriteStringIndex(ByteWriter& writer, const MetadataHeaps& heaps, uint32_t value)
		{
			WriteSizedIndex(writer, value, heaps.StringIndexSize());
		}

		void WriteBlobIndex(ByteWriter& writer, const MetadataHeaps& heaps, uint32_t value)
		{
			WriteSizedIndex(writer, value, heaps.BlobIndexSize());
		}

		void WriteGuidIndex(ByteWriter& writer, const MetadataHeaps& heaps, uint32_t value)
		{
			WriteSizedIndex(writer, value, heaps.GuidIndexSize());
		}

		void WriteTableIndex(ByteWriter& writer, const std::array<uint32_t, 64>& rowCounts, TableId table, uint32_t value)
		{
			WriteSizedIndex(writer, value, TableIndexSize(rowCounts, table));
		}

		uint64_t BuildValidMask(const std::array<uint32_t, 64>& rowCounts)
		{
			uint64_t mask = 0;
			for (size_t i = 0; i < rowCounts.size(); ++i)
			{
				if (rowCounts[i] != 0)
					mask |= 1ull << i;
			}
			return mask;
		}

		void WriteModuleRows(ByteWriter& writer, const MetadataRows& rows, const MetadataHeaps& heaps)
		{
			writer.WriteU16(0);
			WriteStringIndex(writer, heaps, rows.ModuleName);
			WriteGuidIndex(writer, heaps, rows.ModuleMvid);
			WriteGuidIndex(writer, heaps, 0);
			WriteGuidIndex(writer, heaps, 0);
		}

		void WriteTypeRefRows(ByteWriter& writer, const MetadataRows& rows, const MetadataHeaps& heaps, const std::array<uint32_t, 64>& rowCounts)
		{
			const uint32_t scopeSize = CodedIndexSize(rowCounts, { TableId::Module, TableId::TypeRef, TableId::AssemblyRef }, 2);
			for (const TypeRefRow& row : rows.TypeRefs)
			{
				WriteSizedIndex(writer, row.ResolutionScope, scopeSize);
				WriteStringIndex(writer, heaps, row.Name);
				WriteStringIndex(writer, heaps, row.Namespace);
			}
		}

		void WriteTypeDefRows(ByteWriter& writer, const MetadataRows& rows, const MetadataHeaps& heaps, const std::array<uint32_t, 64>& rowCounts)
		{
			const uint32_t extendsSize = CodedIndexSize(rowCounts, { TableId::TypeDef, TableId::TypeRef }, 2);
			for (const TypeDefRow& row : rows.TypeDefs)
			{
				writer.WriteU32(row.Flags);
				WriteStringIndex(writer, heaps, row.Name);
				WriteStringIndex(writer, heaps, row.Namespace);
				WriteSizedIndex(writer, row.Extends, extendsSize);
				WriteTableIndex(writer, rowCounts, TableId::Field, row.FieldList);
				WriteTableIndex(writer, rowCounts, TableId::MethodDef, row.MethodList);
			}
		}

		void WriteFieldRows(ByteWriter& writer, const MetadataRows& rows, const MetadataHeaps& heaps)
		{
			for (const FieldRow& row : rows.Fields)
			{
				writer.WriteU16(row.Flags);
				WriteStringIndex(writer, heaps, row.Name);
				WriteBlobIndex(writer, heaps, row.Signature);
			}
		}

		void WriteMethodRows(ByteWriter& writer, const MetadataRows& rows, const MetadataHeaps& heaps, const std::array<uint32_t, 64>& rowCounts)
		{
			for (const MethodDefRow& row : rows.Methods)
			{
				writer.WriteU32(row.Rva);
				writer.WriteU16(row.ImplFlags);
				writer.WriteU16(row.Flags);
				WriteStringIndex(writer, heaps, row.Name);
				WriteBlobIndex(writer, heaps, row.Signature);
				WriteTableIndex(writer, rowCounts, TableId::Param, row.ParamList);
			}
		}

		void WriteParamRows(ByteWriter& writer, const MetadataRows& rows, const MetadataHeaps& heaps)
		{
			for (const ParamRow& row : rows.Params)
			{
				writer.WriteU16(row.Flags);
				writer.WriteU16(row.Sequence);
				WriteStringIndex(writer, heaps, row.Name);
			}
		}

		void WritePropertyMapRows(ByteWriter& writer, const MetadataRows& rows, const std::array<uint32_t, 64>& rowCounts)
		{
			for (const PropertyMapRow& row : rows.PropertyMaps)
			{
				WriteTableIndex(writer, rowCounts, TableId::TypeDef, row.Parent);
				WriteTableIndex(writer, rowCounts, TableId::Property, row.PropertyList);
			}
		}

		void WritePropertyRows(ByteWriter& writer, const MetadataRows& rows, const MetadataHeaps& heaps)
		{
			for (const PropertyRow& row : rows.Properties)
			{
				writer.WriteU16(row.Flags);
				WriteStringIndex(writer, heaps, row.Name);
				WriteBlobIndex(writer, heaps, row.Type);
			}
		}

		void WriteMethodSemanticsRows(ByteWriter& writer, const MetadataRows& rows, const std::array<uint32_t, 64>& rowCounts)
		{
			const uint32_t associationSize = CodedIndexSize(rowCounts, { TableId::Property }, 1);
			for (const MethodSemanticsRow& row : rows.MethodSemantics)
			{
				writer.WriteU16(row.Semantics);
				WriteTableIndex(writer, rowCounts, TableId::MethodDef, row.Method);
				WriteSizedIndex(writer, (row.Association << 1) | 1u, associationSize);
			}
		}

		void WriteAssemblyRows(ByteWriter& writer, const MetadataRows& rows, const MetadataHeaps& heaps)
		{
			const AssemblyRow& row = rows.Assembly;
			writer.WriteU32(row.HashAlgId);
			writer.WriteU16(row.MajorVersion);
			writer.WriteU16(row.MinorVersion);
			writer.WriteU16(row.BuildNumber);
			writer.WriteU16(row.RevisionNumber);
			writer.WriteU32(row.Flags);
			WriteBlobIndex(writer, heaps, row.PublicKey);
			WriteStringIndex(writer, heaps, row.Name);
			WriteStringIndex(writer, heaps, row.Culture);
		}

		void WriteAssemblyRefRows(ByteWriter& writer, const MetadataRows& rows, const MetadataHeaps& heaps)
		{
			for (const AssemblyRefRow& row : rows.AssemblyRefs)
			{
				writer.WriteU16(row.MajorVersion);
				writer.WriteU16(row.MinorVersion);
				writer.WriteU16(row.BuildNumber);
				writer.WriteU16(row.RevisionNumber);
				writer.WriteU32(row.Flags);
				WriteBlobIndex(writer, heaps, row.PublicKeyOrToken);
				WriteStringIndex(writer, heaps, row.Name);
				WriteStringIndex(writer, heaps, row.Culture);
				WriteBlobIndex(writer, heaps, row.HashValue);
			}
		}

		std::vector<uint8_t> BuildTableStream(const MetadataRows& rows, const MetadataHeaps& heaps)
		{
			const auto rowCounts = rows.RowCounts();
			const uint64_t validMask = BuildValidMask(rowCounts);
			ByteWriter writer;

			writer.WriteU32(0);
			writer.WriteU8(2);
			writer.WriteU8(0);
			writer.WriteU8(heaps.HeapSizeFlags());
			writer.WriteU8(1);
			writer.WriteU64(validMask);
			writer.WriteU64(0);

			for (size_t table = 0; table < rowCounts.size(); ++table)
			{
				if ((validMask & (1ull << table)) != 0)
					writer.WriteU32(rowCounts[table]);
			}

			WriteModuleRows(writer, rows, heaps);
			WriteTypeRefRows(writer, rows, heaps, rowCounts);
			WriteTypeDefRows(writer, rows, heaps, rowCounts);
			WriteFieldRows(writer, rows, heaps);
			WriteMethodRows(writer, rows, heaps, rowCounts);
			WriteParamRows(writer, rows, heaps);
			WritePropertyMapRows(writer, rows, rowCounts);
			WritePropertyRows(writer, rows, heaps);
			WriteMethodSemanticsRows(writer, rows, rowCounts);
			WriteAssemblyRows(writer, rows, heaps);
			WriteAssemblyRefRows(writer, rows, heaps);

			return writer.Data();
		}

		void WriteStreamHeader(ByteWriter& writer, uint32_t offset, uint32_t size, const char* name)
		{
			writer.WriteU32(offset);
			writer.WriteU32(size);
			for (const char* c = name; ; ++c)
			{
				writer.WriteU8(static_cast<uint8_t>(*c));
				if (*c == 0)
					break;
			}
			writer.AlignTo(4);
		}

		uint32_t StreamHeaderSize(const char* name)
		{
			uint32_t length = 0;
			while (name[length] != 0)
				++length;
			return 8 + ((length + 1 + 3) / 4) * 4;
		}
	}

	std::vector<uint8_t> BuildMetadataRoot(const MetadataRows& rows, const MetadataHeaps& heaps)
	{
		std::vector<uint8_t> tableStream = BuildTableStream(rows, heaps);
		const std::vector<uint8_t>* streams[] = { &tableStream, &heaps.Strings(), &heaps.Guid(), &heaps.Blob() };
		const char* names[] = { "#~", "#Strings", "#GUID", "#Blob" };
		constexpr uint16_t streamCount = 4;

		const char version[] = "v4.0.30319";
		const uint32_t versionLength = 12;
		uint32_t metadataOffset = 4 + 2 + 2 + 4 + 4 + versionLength + 2 + 2;
		for (const char* name : names)
			metadataOffset += StreamHeaderSize(name);

		std::array<uint32_t, streamCount> offsets{};
		std::array<uint32_t, streamCount> sizes{};
		uint32_t cursor = metadataOffset;
		for (size_t i = 0; i < streamCount; ++i)
		{
			offsets[i] = cursor;
			sizes[i] = static_cast<uint32_t>(streams[i]->size());
			cursor += sizes[i];
			cursor = (cursor + 3) & ~3u;
		}

		ByteWriter writer;
		writer.WriteU32(0x424A5342);
		writer.WriteU16(1);
		writer.WriteU16(1);
		writer.WriteU32(0);
		writer.WriteU32(versionLength);
		writer.WriteBytes(version, sizeof(version));
		writer.WriteU8(0);
		writer.WriteU16(0);
		writer.WriteU16(streamCount);

		for (size_t i = 0; i < streamCount; ++i)
			WriteStreamHeader(writer, offsets[i], sizes[i], names[i]);

		for (size_t i = 0; i < streamCount; ++i)
		{
			writer.WriteBytes(*streams[i]);
			writer.AlignTo(4);
		}

		return writer.Data();
	}
}
